#ifndef SDF_PARSING_I_LOB_CHAIN_REGISTRY_HPP
#define SDF_PARSING_I_LOB_CHAIN_REGISTRY_HPP

#include <cstdint>
#include <span>
#include <vector>

namespace sdf::parsing
{

class ILobChainRegistry
{
public:
    virtual ~ILobChainRegistry() = default;

    virtual std::vector<std::uint8_t> ResolvePayload(
        std::span<const std::uint8_t> inlineTail, std::size_t totalLength) = 0;
};

}

#endif
