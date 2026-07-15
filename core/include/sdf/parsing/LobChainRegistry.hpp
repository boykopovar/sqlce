#ifndef SDF_PARSING_LOB_CHAIN_REGISTRY_HPP
#define SDF_PARSING_LOB_CHAIN_REGISTRY_HPP

#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>
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
    using ChainKey = std::variant<std::uint8_t, std::string>;

    struct Chain
    {
        std::vector<std::size_t> pageNumbers;
    };

    class InlineLobSource;
    class ChainLobSource;

    const domain::IPageStorage* _storage;
    std::map<ChainKey, Chain> _chains;
    std::set<ChainKey> _usedChains;
};

}

#endif
