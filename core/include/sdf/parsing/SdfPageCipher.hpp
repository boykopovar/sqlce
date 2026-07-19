#ifndef SDF_PARSING_SDF_PAGE_CIPHER_HPP
#define SDF_PARSING_SDF_PAGE_CIPHER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/interfaces/IPageCipher.hpp"

namespace sdf::parsing
{

class SdfPageCipher final : public domain::IPageCipher
{
public:
    SdfPageCipher(std::span<const std::uint8_t> page0, std::string password);

    static domain::EncryptionMode ReadMode(std::span<const std::uint8_t> page0);

    [[nodiscard]] bool VerifyPassword() const override;
    [[nodiscard]] std::vector<std::uint8_t> DecryptPage(std::size_t pageNumber, std::span<const std::uint8_t> page) const override;

private:
    domain::EncryptionMode _mode;
    std::string _password;
    std::vector<std::uint8_t> _passwordUtf16Le;
    std::vector<std::uint8_t> _page0Ciphertext;
    std::array<std::uint8_t, 4> _page0KeyParam;

    [[nodiscard]] std::vector<std::uint8_t> DecryptPage0(std::span<const std::uint8_t> page0) const;
    [[nodiscard]] std::vector<std::uint8_t> DeriveKey(std::span<const std::uint8_t> keyParam) const;
    [[nodiscard]] std::vector<std::uint8_t> HashPasswordWith(std::span<const std::uint8_t> keyParam) const;
};

}

#endif
