#ifndef SDF_PARSING_LOB_CHAIN_REGISTRY_HPP
#define SDF_PARSING_LOB_CHAIN_REGISTRY_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "sdf/domain/ILazyLobSource.hpp"
#include "sdf/domain/IPageStorage.hpp"
#include "sdf/parsing/ILobChainRegistry.hpp"

namespace sdf::parsing
{

    class LobChainRegistry final : public ILobChainRegistry
    {
    public:
        explicit LobChainRegistry(const domain::IPageStorage& storage);

        std::shared_ptr<domain::ILazyLobSource> ResolveLob(
            std::span<const std::uint8_t> inlineTail, std::size_t totalLength) override;

    private:
        class InlineLobSource;
        class ChainLobSource;

        struct PageEntry
        {
            std::size_t pageNumber;
            std::uint8_t pageType;
        };

        const domain::IPageStorage* _storage;

        std::map<std::uint16_t, PageEntry> _pageByLogicalId;
    };

}

#endif