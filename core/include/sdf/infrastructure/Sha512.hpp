#ifndef SDF_INFRASTRUCTURE_SHA512_HPP
#define SDF_INFRASTRUCTURE_SHA512_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

std::array<std::uint8_t, 64> Sha512(std::span<const std::uint8_t> message);

}

#endif
