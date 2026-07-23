#ifndef SDF_INFRASTRUCTURE_PAGE_CHECKSUM_HPP
#define SDF_INFRASTRUCTURE_PAGE_CHECKSUM_HPP

#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

[[nodiscard]] std::uint32_t ComputePageChecksum(std::span<const std::uint8_t> page);

}

#endif
