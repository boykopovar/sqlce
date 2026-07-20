#ifndef SDF_INFRASTRUCTURE_SHA1_HPP
#define SDF_INFRASTRUCTURE_SHA1_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

std::array<std::uint8_t, 20> Sha1(std::span<const std::uint8_t> message);

}

#endif
