#ifndef SDF_PARSING_SDF_PAGE_CIPHER_HPP
#define SDF_PARSING_SDF_PAGE_CIPHER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/FormatVersion.hpp"
#include "sdf/domain/interfaces/IPageCipher.hpp"

namespace sdf::parsing
{

class SdfPageCipher final : public domain::IPageCipher
{
public:
    SdfPageCipher(std::span<const std::uint8_t> page0, std::string password);

    static domain::EncryptionMode ReadMode(std::span<const std::uint8_t> page0);
    static domain::FormatVersion ReadFormatVersion(std::span<const std::uint8_t> page0);

    [[nodiscard]] bool VerifyPassword() const override;
    [[nodiscard]] std::vector<std::uint8_t> DecryptPage(std::size_t pageNumber, std::span<const std::uint8_t> page) const override;
    [[nodiscard]] domain::EncryptionMode ResolvedEncryptionMode() const override;

private:
    std::string _password;
    std::vector<std::uint8_t> _passwordUtf16Le;
    std::vector<std::uint8_t> _page0Ciphertext;
    std::array<std::uint8_t, 4> _page0KeyParam;
    mutable domain::EncryptionMode _algorithm;
    mutable bool _algorithmResolved;

    [[nodiscard]] bool ResolveAlgorithm() const;
    [[nodiscard]] bool VerifyWith(domain::EncryptionMode algorithm) const;
    [[nodiscard]] std::vector<std::uint8_t> DecryptPage0(std::span<const std::uint8_t> page0) const;
    [[nodiscard]] std::vector<std::uint8_t> DecryptTail(domain::EncryptionMode algorithm, std::span<const std::uint8_t> keyParam, std::span<const std::uint8_t> ciphertext) const;
    [[nodiscard]] std::vector<std::uint8_t> DeriveKey(domain::EncryptionMode algorithm, std::span<const std::uint8_t> keyParam) const;
    [[nodiscard]] std::vector<std::uint8_t> HashPasswordWith(domain::EncryptionMode algorithm, std::span<const std::uint8_t> keyParam) const;
};

}

#endif
