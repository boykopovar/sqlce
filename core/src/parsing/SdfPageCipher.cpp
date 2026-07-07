#include "sdf/parsing/SdfPageCipher.hpp"

#include <stdexcept>

#include "sdf/domain/PageLayout.hpp"
#include "sdf/infrastructure/AesCbc.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/infrastructure/Sha256.hpp"
#include "sdf/infrastructure/Sha512.hpp"

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

std::vector<std::uint8_t> Concat(std::span<const std::uint8_t> first, std::span<const std::uint8_t> second)
{
    std::vector<std::uint8_t> result;
    result.reserve(first.size() + second.size());
    result.insert(result.end(), first.begin(), first.end());
    result.insert(result.end(), second.begin(), second.end());
    return result;
}

std::array<std::uint8_t, 16> ZeroIv()
{
    std::array<std::uint8_t, 16> iv{};
    iv.fill(0);
    return iv;
}

}

domain::EncryptionMode SdfPageCipher::ReadMode(std::span<const std::uint8_t> page0)
{
    const std::uint32_t rawMode = infrastructure::ReadUInt32LE(page0, domain::Page0ModeOffset);
    return static_cast<domain::EncryptionMode>(rawMode);
}

SdfPageCipher::SdfPageCipher(std::span<const std::uint8_t> page0, std::string password) : _password(std::move(password))
{
    if (_password.size() > domain::MaxPasswordLength)
    {
        throw std::invalid_argument("password exceeds maximum supported length");
    }

    _mode = ReadMode(page0);
    if (_mode != domain::EncryptionMode::Aes128Sha256 && _mode != domain::EncryptionMode::Aes256Sha512)
    {
        throw domain::UnsupportedEncryptionModeException(_mode);
    }

    _passwordUtf16Le = ToUtf16Le(_password);

    const auto ciphertext = page0.subspan(domain::Page0VerifierOffset, domain::Page0VerifierLength);
    _page0Ciphertext.assign(ciphertext.begin(), ciphertext.end());

    const auto keyParam = page0.subspan(domain::Page0KeyParamOffset, domain::Page0KeyParamLength);
    for (std::size_t i = 0; i < domain::Page0KeyParamLength; ++i)
    {
        _page0KeyParam[i] = keyParam[i];
    }
}

std::vector<std::uint8_t> SdfPageCipher::HashPasswordWith(std::span<const std::uint8_t> keyParam) const
{
    const std::vector<std::uint8_t> hashInput = Concat(_passwordUtf16Le, keyParam);

    if (_mode == domain::EncryptionMode::Aes128Sha256)
    {
        const std::array<std::uint8_t, 32> digest = infrastructure::Sha256(hashInput);
        return std::vector<std::uint8_t>(digest.begin(), digest.end());
    }

    const std::array<std::uint8_t, 64> digest = infrastructure::Sha512(hashInput);
    return std::vector<std::uint8_t>(digest.begin(), digest.end());
}

std::vector<std::uint8_t> SdfPageCipher::DeriveKey(std::span<const std::uint8_t> keyParam) const
{
    const std::vector<std::uint8_t> hash = HashPasswordWith(keyParam);
    const std::size_t keyLength = (_mode == domain::EncryptionMode::Aes128Sha256) ? 16 : 32;
    return std::vector<std::uint8_t>(hash.begin(), hash.begin() + static_cast<std::ptrdiff_t>(keyLength));
}

bool SdfPageCipher::VerifyPassword() const
{
    const std::vector<std::uint8_t> key = DeriveKey(_page0KeyParam);
    const infrastructure::AesCbcDecryptor decryptor(key, ZeroIv());
    const std::vector<std::uint8_t> plaintext = decryptor.Decrypt(_page0Ciphertext);

    std::vector<std::uint8_t> expected = _passwordUtf16Le;
    expected.resize(domain::Page0VerifierLength, 0x00);

    return plaintext == expected;
}

std::vector<std::uint8_t> SdfPageCipher::DecryptPage0(std::span<const std::uint8_t> page0) const
{
    const std::vector<std::uint8_t> key = DeriveKey(_page0KeyParam);
    const infrastructure::AesCbcDecryptor decryptor(key, ZeroIv());
    const std::vector<std::uint8_t> plaintext = decryptor.Decrypt(_page0Ciphertext);

    std::vector<std::uint8_t> result(page0.begin(), page0.end());
    for (std::size_t i = 0; i < plaintext.size(); ++i)
    {
        result[domain::Page0VerifierOffset + i] = plaintext[i];
    }
    return result;
}

std::vector<std::uint8_t> SdfPageCipher::DecryptPage(std::size_t pageNumber, std::span<const std::uint8_t> page) const
{
    if (pageNumber == 0)
    {
        return DecryptPage0(page);
    }

    const std::uint32_t pageTypeWord = infrastructure::ReadUInt32LE(page, 4);
    const std::uint32_t pageType = (pageTypeWord >> 20) & 0xF;
    if (pageType <= 2)
    {
        return std::vector<std::uint8_t>(page.begin(), page.end());
    }

    const auto keyParam = page.subspan(0, domain::Page0KeyParamLength);
    const std::vector<std::uint8_t> key = DeriveKey(keyParam);
    const infrastructure::AesCbcDecryptor decryptor(key, ZeroIv());

    const auto tailCiphertext = page.subspan(domain::PlaintextHeaderLength);
    const std::vector<std::uint8_t> tailPlaintext = decryptor.Decrypt(tailCiphertext);

    std::vector<std::uint8_t> result(page.begin(), page.begin() + domain::PlaintextHeaderLength);
    result.insert(result.end(), tailPlaintext.begin(), tailPlaintext.end());
    return result;
}

}
