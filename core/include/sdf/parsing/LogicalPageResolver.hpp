#ifndef SDF_PARSING_LOGICAL_PAGE_RESOLVER_HPP
#define SDF_PARSING_LOGICAL_PAGE_RESOLVER_HPP

#include <cstddef>
#include <mutex>
#include <unordered_map>

#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"

namespace sdf::parsing
{

class LogicalPageResolver final : public ILogicalPageResolver
{
public:
    [[nodiscard]] std::optional<std::size_t> ResolvePhysicalPage(
const domain::IPageStorage& storage, std::uint32_t logicalPageId) const override;

private:
    struct LogicalPageMap
    {
        std::unordered_map<std::uint32_t, std::size_t> physicalPageByLogicalId;
    };

    const LogicalPageMap& _mapFor(const domain::IPageStorage& storage) const;

    mutable std::mutex _mutex;
    mutable const domain::IPageStorage* _cachedStorage = nullptr;
    mutable LogicalPageMap _cachedMap;
};

}

#endif
