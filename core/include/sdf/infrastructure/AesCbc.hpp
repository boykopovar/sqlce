#ifndef SDF_INFRASTRUCTURE_AES_CBC_HPP
#define SDF_INFRASTRUCTURE_AES_CBC_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sdf::infrastructure
{

constexpr std::size_t Aes128KeyLength = 16;
constexpr std::size_t Aes256KeyLength = 32;
constexpr std::size_t Aes128RoundCount = 10;
constexpr std::size_t Aes256RoundCount = 14;
constexpr std::size_t AesBlockLength = 16;

class AesCbcDecryptor
{
public:
    explicit AesCbcDecryptor(std::span<const std::uint8_t> key, std::span<const std::uint8_t, AesBlockLength> iv);

    [[nodiscard]] std::vector<std::uint8_t> Decrypt(std::span<const std::uint8_t> ciphertext) const;

private:
    std::vector<std::uint32_t> _roundKeys;
    std::size_t _roundCount;
    std::array<std::uint8_t, AesBlockLength> _iv;

    void DecryptBlock(const std::uint8_t* in, std::uint8_t* out) const;
};

}

#endif
