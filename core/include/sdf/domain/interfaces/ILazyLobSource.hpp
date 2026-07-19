#ifndef SDF_DOMAIN_I_LAZY_LOB_SOURCE_HPP
#define SDF_DOMAIN_I_LAZY_LOB_SOURCE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdf::domain
{

class ILazyLobSource
{
public:
    virtual ~ILazyLobSource() = default;

    virtual std::size_t TotalLength() const = 0;
    virtual std::size_t ChunkCount() const = 0;
    virtual std::vector<std::uint8_t> ReadChunk(std::size_t chunkIndex) const = 0;
};

}

#endif
