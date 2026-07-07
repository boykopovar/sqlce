#include "sdf/domain/Guid.hpp"

#include <cstdio>

namespace sdf::domain
{

Guid::Guid() : _bytes{}
{
}

Guid::Guid(const std::array<std::uint8_t, 16>& littleEndianBytes) : _bytes(littleEndianBytes)
{
}

std::string Guid::ToString() const
{
    const std::uint32_t data1 = static_cast<std::uint32_t>(_bytes[0])
        | (static_cast<std::uint32_t>(_bytes[1]) << 8)
        | (static_cast<std::uint32_t>(_bytes[2]) << 16)
        | (static_cast<std::uint32_t>(_bytes[3]) << 24);
    const std::uint16_t data2 = static_cast<std::uint16_t>(_bytes[4] | (_bytes[5] << 8));
    const std::uint16_t data3 = static_cast<std::uint16_t>(_bytes[6] | (_bytes[7] << 8));

    char buffer[37];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        data1,
        data2,
        data3,
        _bytes[8],
        _bytes[9],
        _bytes[10],
        _bytes[11],
        _bytes[12],
        _bytes[13],
        _bytes[14],
        _bytes[15]);
    return std::string(buffer);
}

const std::array<std::uint8_t, 16>& Guid::Bytes() const
{
    return _bytes;
}

}
