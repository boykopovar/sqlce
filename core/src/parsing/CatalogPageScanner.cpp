#include "sdf/parsing/CatalogPageScanner.hpp"

#include <algorithm>
#include <array>

#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"

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
        const PageView page(storage.PageBytes(pageNumber));
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

void CatalogPageScanner::AssignDataPages(
    const domain::IPageStorage& storage, const std::map<std::uint8_t, domain::TableDef*>& tableByObjectId) const
{
    const std::set<std::uint8_t> catalogObjectIds = FindCatalogObjectIds(storage);
    const std::size_t pageCount = storage.PageCount();

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const PageView page(storage.PageBytes(pageNumber));
        if (!page.IsDataPage())
        {
            continue;
        }
        if (catalogObjectIds.find(page.OwnerObjectId()) != catalogObjectIds.end())
        {
            continue;
        }

        const auto it = tableByObjectId.find(page.OwnerObjectId());
        if (it != tableByObjectId.end())
        {
            it->second->DataPageNumbers().push_back(pageNumber);
        }
    }
}

std::vector<std::vector<std::uint8_t>> CatalogPageScanner::CollectCatalogRows(
    const domain::IPageStorage& storage) const
{
    const std::set<std::uint8_t> catalogObjectIds = FindCatalogObjectIds(storage);
    const std::size_t pageCount = storage.PageCount();

    auto isCatalogPage = [&](std::size_t pageNumber) -> bool
    {
        const PageView page(storage.PageBytes(pageNumber));
        return page.IsDataPage() && catalogObjectIds.find(page.OwnerObjectId()) != catalogObjectIds.end();
    };

    std::set<std::pair<std::size_t, std::size_t>> continuationTargets;
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        if (!isCatalogPage(pageNumber))
        {
            continue;
        }
        const PageView page(storage.PageBytes(pageNumber));
        for (const ContinuedRowSlice& slice : page.RowsWithContinuation())
        {
            if (slice.hasContinuation && slice.continuationPageNumber < pageCount)
            {
                continuationTargets.emplace(slice.continuationPageNumber, slice.continuationSlotIndex);
            }
        }
    }

    std::vector<std::vector<std::uint8_t>> rows;

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        if (!isCatalogPage(pageNumber))
        {
            continue;
        }
        const PageView page(storage.PageBytes(pageNumber));
        for (const ContinuedRowSlice& slice : page.RowsWithContinuation())
        {
            if (!slice.isFirstFragment)
            {
                continue;
            }
            if (continuationTargets.find({pageNumber, slice.slotIndex}) != continuationTargets.end())
            {
                continue;
            }

            std::vector<std::uint8_t> assembled(slice.bytes.begin(), slice.bytes.end());

            bool continued = slice.hasContinuation;
            std::size_t nextPageNumber = slice.continuationPageNumber;
            std::size_t nextSlotIndex = slice.continuationSlotIndex;
            std::size_t hops = 0;

            while (continued && hops < MaxRowContinuationHops && nextPageNumber < pageCount)
            {
                ++hops;
                const PageView nextPage(storage.PageBytes(nextPageNumber));

                bool matched = false;
                for (const ContinuedRowSlice& nextSlice : nextPage.RowsWithContinuation())
                {
                    if (nextSlice.slotIndex != nextSlotIndex)
                    {
                        continue;
                    }
                    assembled.insert(assembled.end(), nextSlice.bytes.begin(), nextSlice.bytes.end());
                    matched = true;
                    continued = nextSlice.hasContinuation;
                    nextPageNumber = nextSlice.continuationPageNumber;
                    nextSlotIndex = nextSlice.continuationSlotIndex;
                    break;
                }

                if (!matched)
                {
                    break;
                }
            }

            rows.push_back(std::move(assembled));
        }
    }

    return rows;
}

}
