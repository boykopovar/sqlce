#ifndef SDF_INFRASTRUCTURE_TRIPLE_DES_CBC_HPP
#define SDF_INFRASTRUCTURE_TRIPLE_DES_CBC_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sdf::infrastructure
{

constexpr std::size_t TripleDesKeyLength = 24;
constexpr std::size_t DesBlockLength = 8;

class TripleDesCbcDecryptor
{
public:
    explicit TripleDesCbcDecryptor(std::span<const std::uint8_t> key, std::span<const std::uint8_t, DesBlockLength> iv);

    [[nodiscard]] std::vector<std::uint8_t> Decrypt(std::span<const std::uint8_t> ciphertext) const;

private:
    std::array<std::uint8_t, TripleDesKeyLength> _key;
    std::array<std::uint8_t, DesBlockLength> _iv;

    void DecryptBlock(const std::uint8_t* in, std::uint8_t* out) const;
};

}

#endif
