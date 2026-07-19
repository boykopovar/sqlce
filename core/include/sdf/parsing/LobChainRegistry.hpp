#ifndef SDF_PARSING_LOB_CHAIN_REGISTRY_HPP
#define SDF_PARSING_LOB_CHAIN_REGISTRY_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "sdf/domain/interfaces/ILazyLobSource.hpp"
#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/parsing/interfaces/ILobChainRegistry.hpp"

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

        static std::vector<std::uint32_t> ReadPackedSlots(
            std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t maxSlots);

        std::optional<std::size_t> PhysicalPageOf(std::uint32_t logicalId) const;

        const domain::IPageStorage* _storage;

        std::map<std::uint32_t, PageEntry> _pageByLogicalId;
    };

}

#endif