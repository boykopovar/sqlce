#ifndef SDF_PARSING_LOGICAL_PAGE_RESOLVER_HPP
#define SDF_PARSING_LOGICAL_PAGE_RESOLVER_HPP

#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"

namespace sdf::parsing
{

class LogicalPageResolver final : public ILogicalPageResolver
{
public:
    [[nodiscard]] std::optional<std::size_t> ResolvePhysicalPage(
        const domain::IPageStorage& storage, std::uint32_t logicalPageId) const override;
};

}

#endif
