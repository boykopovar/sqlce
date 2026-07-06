#ifndef SDF_DOMAIN_GUID_HPP
#define SDF_DOMAIN_GUID_HPP

#include <array>
#include <cstdint>
#include <string>

namespace sdf::domain
{

class Guid
{
public:
    Guid();
    explicit Guid(const std::array<std::uint8_t, 16>& littleEndianBytes);

    std::string ToString() const;

    const std::array<std::uint8_t, 16>& Bytes() const;

private:
    std::array<std::uint8_t, 16> _bytes;
};

}

#endif
