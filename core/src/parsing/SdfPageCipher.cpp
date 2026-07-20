#include "sdf/parsing/SdfPageCipher.hpp"

#include <stdexcept>

#include "sdf/infrastructure/AesCbc.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/infrastructure/Sha1KeyDerivation.hpp"
#include "sdf/infrastructure/Sha256.hpp"
#include "sdf/infrastructure/Sha512.hpp"
#include "sdf/infrastructure/TripleDesCbc.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::vector<std::uint8_t> ToUtf16Le(const std::string& password)
{
    std::vector<std::uint8_t> encoded;
    encoded.reserve(password.size() * 2);
    for (const char character : password)
    {
        encoded.push_back(static_cast<std::uint8_t>(character));
        encoded.push_back(0x00);
    }
    return encoded;
}

std::array<std::uint8_t, infrastructure::AesBlockLength> ZeroAesIv()
{
    std::array<std::uint8_t, infrastructure::AesBlockLength> iv{};
    iv.fill(0);
    return iv;
}

std::array<std::uint8_t, infrastructure::DesBlockLength> ZeroDesIv()
{
    std::array<std::uint8_t, infrastructure::DesBlockLength> iv{};
    iv.fill(0);
    return iv;
}

constexpr std::array<CipherAlgorithm, 4> AllCipherAlgorithms
    = {CipherAlgorithm::TripleDesSha1, CipherAlgorithm::Aes128Sha1, CipherAlgorithm::Aes128Sha256, CipherAlgorithm::Aes256Sha512};

}

domain::EncryptionMode SdfPageCipher::ReadMode(std::span<const std::uint8_t> page0)
{
    const std::uint32_t rawMode = infrastructure::ReadUInt32LE(page0, Page0ModeOffset);
    return static_cast<domain::EncryptionMode>(rawMode);
}

domain::FormatVersion SdfPageCipher::ReadFormatVersion(std::span<const std::uint8_t> page0)
{
    const std::uint32_t rawVersion = infrastructure::ReadUInt32LE(page0, Page0FormatVersionOffset);

    switch (static_cast<domain::FormatVersion>(rawVersion))
    {
    case domain::FormatVersion::SqlCe30:
    case domain::FormatVersion::SqlCe35:
    case domain::FormatVersion::SqlCe35Sp2:
    case domain::FormatVersion::SqlCe40:
        return static_cast<domain::FormatVersion>(rawVersion);
    default:
        return domain::FormatVersion::Unknown;
    }
}

SdfPageCipher::SdfPageCipher(std::span<const std::uint8_t> page0, std::string password)
    : _password(std::move(password)), _algorithm(CipherAlgorithm::TripleDesSha1), _algorithmResolved(false)
{
    if (_password.size() > MaxPasswordLength)
    {
        throw std::invalid_argument("password exceeds maximum supported length");
    }

    _passwordUtf16Le = ToUtf16Le(_password);

    const auto ciphertext = page0.subspan(Page0VerifierOffset, Page0VerifierLength);
    _page0Ciphertext.assign(ciphertext.begin(), ciphertext.end());

    const auto keyParam = page0.subspan(Page0KeyParamOffset, Page0KeyParamLength);
    for (std::size_t i = 0; i < Page0KeyParamLength; ++i)
    {
        _page0KeyParam[i] = keyParam[i];
    }
}

std::vector<std::uint8_t> SdfPageCipher::HashPasswordWith(CipherAlgorithm algorithm, std::span<const std::uint8_t> keyParam) const
{
    if (algorithm == CipherAlgorithm::TripleDesSha1 || algorithm == CipherAlgorithm::Aes128Sha1)
    {
        const std::array<std::uint8_t, infrastructure::Sha1KeyDerivationLength> digest
            = infrastructure::Sha1KeyDerivation(_passwordUtf16Le, keyParam);
        return std::vector<std::uint8_t>(digest.begin(), digest.end());
    }

    std::vector<std::uint8_t> hashInput;
    hashInput.reserve(_passwordUtf16Le.size() + keyParam.size());
    hashInput.insert(hashInput.end(), _passwordUtf16Le.begin(), _passwordUtf16Le.end());
    hashInput.insert(hashInput.end(), keyParam.begin(), keyParam.end());

    if (algorithm == CipherAlgorithm::Aes128Sha256)
    {
        const std::array<std::uint8_t, 32> digest = infrastructure::Sha256(hashInput);
        return std::vector<std::uint8_t>(digest.begin(), digest.end());
    }

    const std::array<std::uint8_t, 64> digest = infrastructure::Sha512(hashInput);
    return std::vector<std::uint8_t>(digest.begin(), digest.end());
}

std::vector<std::uint8_t> SdfPageCipher::DeriveKey(CipherAlgorithm algorithm, std::span<const std::uint8_t> keyParam) const
{
    const std::vector<std::uint8_t> hash = HashPasswordWith(algorithm, keyParam);

    std::size_t keyLength = infrastructure::Aes256KeyLength;
    if (algorithm == CipherAlgorithm::TripleDesSha1)
    {
        keyLength = infrastructure::TripleDesKeyLength;
    }
    else if (algorithm == CipherAlgorithm::Aes128Sha1 || algorithm == CipherAlgorithm::Aes128Sha256)
    {
        keyLength = infrastructure::Aes128KeyLength;
    }

    return std::vector<std::uint8_t>(hash.begin(), hash.begin() + static_cast<std::ptrdiff_t>(keyLength));
}

std::vector<std::uint8_t> SdfPageCipher::DecryptTail(
    CipherAlgorithm algorithm, std::span<const std::uint8_t> keyParam, std::span<const std::uint8_t> ciphertext) const
{
    const std::vector<std::uint8_t> key = DeriveKey(algorithm, keyParam);

    if (algorithm == CipherAlgorithm::TripleDesSha1)
    {
        const infrastructure::TripleDesCbcDecryptor decryptor(key, ZeroDesIv());
        return decryptor.Decrypt(ciphertext);
    }

    const infrastructure::AesCbcDecryptor decryptor(key, ZeroAesIv());
    return decryptor.Decrypt(ciphertext);
}

bool SdfPageCipher::VerifyWith(CipherAlgorithm algorithm) const
{
    const std::vector<std::uint8_t> plaintext = DecryptTail(algorithm, _page0KeyParam, _page0Ciphertext);

    std::vector<std::uint8_t> expected = _passwordUtf16Le;
    expected.resize(Page0VerifierLength, 0x00);

    return plaintext == expected;
}

bool SdfPageCipher::ResolveAlgorithm() const
{
    if (_algorithmResolved)
    {
        return true;
    }

    for (const CipherAlgorithm candidate : AllCipherAlgorithms)
    {
        if (VerifyWith(candidate))
        {
            _algorithm = candidate;
            _algorithmResolved = true;
            return true;
        }
    }

    return false;
}

bool SdfPageCipher::VerifyPassword() const
{
    return ResolveAlgorithm();
}

std::vector<std::uint8_t> SdfPageCipher::DecryptPage0(std::span<const std::uint8_t> page0) const
{
    if (!ResolveAlgorithm())
    {
        throw domain::InvalidPasswordException();
    }

    const std::vector<std::uint8_t> plaintext = DecryptTail(_algorithm, _page0KeyParam, _page0Ciphertext);

    std::vector<std::uint8_t> result(page0.begin(), page0.end());
    for (std::size_t i = 0; i < plaintext.size(); ++i)
    {
        result[Page0VerifierOffset + i] = plaintext[i];
    }
    return result;
}

std::vector<std::uint8_t> SdfPageCipher::DecryptPage(std::size_t pageNumber, std::span<const std::uint8_t> page) const
{
    if (pageNumber == 0)
    {
        return DecryptPage0(page);
    }

    const std::uint32_t pageTypeWord = infrastructure::ReadUInt32LE(page, EncryptedPageTypeWordOffset);
    const std::uint32_t pageType = (pageTypeWord >> EncryptedPageTypeShift) & EncryptedPageTypeMask;
    if (pageType <= EncryptedPageTypeMaxPlaintext)
    {
        return std::vector<std::uint8_t>(page.begin(), page.end());
    }

    if (!ResolveAlgorithm())
    {
        throw domain::InvalidPasswordException();
    }

    const auto keyParam = page.subspan(0, Page0KeyParamLength);
    const std::vector<std::uint8_t> tailPlaintext = DecryptTail(_algorithm, keyParam, page.subspan(PlaintextHeaderLength));

    std::vector<std::uint8_t> result(page.begin(), page.begin() + PlaintextHeaderLength);
    result.insert(result.end(), tailPlaintext.begin(), tailPlaintext.end());
    return result;
}

}
