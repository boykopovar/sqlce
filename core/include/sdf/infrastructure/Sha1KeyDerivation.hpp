#ifndef SDF_INFRASTRUCTURE_SHA1_KEY_DERIVATION_HPP
#define SDF_INFRASTRUCTURE_SHA1_KEY_DERIVATION_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

constexpr std::size_t Sha1KeyDerivationLength = 40;

std::array<std::uint8_t, Sha1KeyDerivationLength> Sha1KeyDerivation(
    std::span<const std::uint8_t> baseHash, std::span<const std::uint8_t> extra);

}

#endif
