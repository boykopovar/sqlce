#ifndef SDF_PARSING_I_LOB_CHAIN_REGISTRY_HPP
#define SDF_PARSING_I_LOB_CHAIN_REGISTRY_HPP

#include <cstdint>
#include <memory>
#include <span>

#include "sdf/domain/interfaces/ILazyLobSource.hpp"

namespace sdf::parsing
{

class ILobChainRegistry
{
public:
    virtual ~ILobChainRegistry() = default;

    virtual std::shared_ptr<domain::ILazyLobSource> ResolveLob(
        std::span<const std::uint8_t> inlineTail, std::size_t totalLength) = 0;
};

}

#endif
