#include "sdf/infrastructure/BinaryReader.hpp"

#include <cstring>

namespace sdf::infrastructure
{

std::uint16_t ReadUInt16LE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::uint16_t>(bytes[offset]) | (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t ReadUInt32LE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::int32_t ReadInt32LE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::int32_t>(ReadUInt32LE(bytes, offset));
}

std::uint64_t ReadUInt64LE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    std::uint64_t result = 0;
    for (std::size_t i = 0; i < 8; ++i)
    {
        result |= static_cast<std::uint64_t>(bytes[offset + i]) << (8 * i);
    }
    return result;
}

std::int64_t ReadInt64LE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::int64_t>(ReadUInt64LE(bytes, offset));
}

double ReadDoubleLE(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    static_assert(sizeof(double) == 8, "double must be 8 bytes on this platform");
    double result = 0.0;
    std::uint64_t raw = ReadUInt64LE(bytes, offset);
    std::memcpy(&result, &raw, sizeof(result));
    return result;
}

std::array<std::uint8_t, 16> ReadBytes16(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    std::array<std::uint8_t, 16> result{};
    for (std::size_t i = 0; i < 16; ++i)
    {
        result[i] = bytes[offset + i];
    }
    return result;
}

}
