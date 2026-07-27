#include "sdf/parsing/LogicalPageResolver.hpp"

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

std::optional<std::size_t> LogicalPageResolver::ResolvePhysicalPage(
    const domain::IPageStorage& storage, std::uint32_t logicalPageId) const
{
    const std::size_t pageCount = storage.PageCount();

    std::optional<std::size_t> latestPhysicalPage;
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::uint32_t candidateLogicalId =
            infrastructure::ReadUInt32LE(storage.PageBytes(pageNumber), LogicalPageIdOffset) & LogicalPageIdMask;
        if (candidateLogicalId != logicalPageId)
        {
            continue;
        }
        if (!latestPhysicalPage.has_value() || pageNumber > *latestPhysicalPage)
        {
            latestPhysicalPage = pageNumber;
        }
    }

    return latestPhysicalPage;
}

}
