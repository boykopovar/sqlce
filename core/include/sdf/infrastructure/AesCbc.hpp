#ifndef SDF_INFRASTRUCTURE_AES_CBC_HPP
#define SDF_INFRASTRUCTURE_AES_CBC_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sdf::infrastructure
{

class AesCbcDecryptor
{
public:
    explicit AesCbcDecryptor(std::span<const std::uint8_t> key, std::span<const std::uint8_t, 16> iv);

    std::vector<std::uint8_t> Decrypt(std::span<const std::uint8_t> ciphertext) const;

private:
    std::vector<std::uint32_t> _roundKeys;
    std::size_t _roundCount;
    std::array<std::uint8_t, 16> _iv;

    void DecryptBlock(const std::uint8_t* in, std::uint8_t* out) const;
};

}

#endif
