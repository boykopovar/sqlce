#include "sdf/parsing/CatalogPageScanner.hpp"

#include <algorithm>
#include <array>

#include "sdf/infrastructure/PageView.hpp"

namespace sdf::parsing
{

namespace
{

constexpr std::array<std::uint8_t, 5> CatalogMarker{'_', '_', 'S', 'y', 's'};

bool ContainsCatalogMarker(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() < CatalogMarker.size())
    {
        return false;
    }
    const auto it = std::search(bytes.begin(), bytes.end(), CatalogMarker.begin(), CatalogMarker.end());
    return it != bytes.end();
}

}

std::set<std::uint8_t> CatalogPageScanner::FindCatalogObjectIds(const domain::IPageStorage& storage) const
{
    std::set<std::uint8_t> objectIds;
    const std::size_t pageCount = storage.PageCount();

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const infrastructure::PageView page(storage.PageBytes(pageNumber));
        if (!page.IsDataPage())
        {
            continue;
        }
        if (ContainsCatalogMarker(page.Bytes()))
        {
            objectIds.insert(page.OwnerObjectId());
        }
    }

    return objectIds;
}

std::vector<std::vector<std::uint8_t>> CatalogPageScanner::CollectCatalogRows(
    const domain::IPageStorage& storage) const
{
    const std::set<std::uint8_t> catalogObjectIds = FindCatalogObjectIds(storage);
    std::vector<std::vector<std::uint8_t>> rows;
    const std::size_t pageCount = storage.PageCount();

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const infrastructure::PageView page(storage.PageBytes(pageNumber));
        if (!page.IsDataPage())
        {
            continue;
        }
        if (catalogObjectIds.find(page.OwnerObjectId()) == catalogObjectIds.end())
        {
            continue;
        }
        for (const infrastructure::RowSlice& slice : page.Rows())
        {
            rows.emplace_back(slice.bytes.begin(), slice.bytes.end());
        }
    }

    return rows;
}

}
