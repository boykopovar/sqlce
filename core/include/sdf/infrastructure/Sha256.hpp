#ifndef SDF_INFRASTRUCTURE_SHA256_HPP
#define SDF_INFRASTRUCTURE_SHA256_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

std::array<std::uint8_t, 32> Sha256(std::span<const std::uint8_t> message);

}

#endif
