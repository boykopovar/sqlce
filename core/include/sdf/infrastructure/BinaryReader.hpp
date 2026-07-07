#ifndef SDF_INFRASTRUCTURE_BINARY_READER_HPP
#define SDF_INFRASTRUCTURE_BINARY_READER_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::infrastructure
{

std::uint16_t ReadUInt16LE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::uint32_t ReadUInt32LE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::int32_t ReadInt32LE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::uint64_t ReadUInt64LE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::int64_t ReadInt64LE(std::span<const std::uint8_t> bytes, std::size_t offset);
double ReadDoubleLE(std::span<const std::uint8_t> bytes, std::size_t offset);
std::array<std::uint8_t, 16> ReadBytes16(std::span<const std::uint8_t> bytes, std::size_t offset);

}

#endif
