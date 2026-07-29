#ifndef SDF_PARSING_I_LOGICAL_PAGE_RESOLVER_HPP
#define SDF_PARSING_I_LOGICAL_PAGE_RESOLVER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>

#include "sdf/domain/interfaces/IPageStorage.hpp"

namespace sdf::parsing
{

class ILogicalPageResolver
{
public:
    virtual ~ILogicalPageResolver() = default;

    virtual std::optional<std::size_t> ResolvePhysicalPage(
        const domain::IPageStorage& storage, std::uint32_t logicalPageId) const = 0;
};

}

#endif
