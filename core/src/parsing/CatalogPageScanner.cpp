#include "sdf/parsing/CatalogPageScanner.hpp"

#include <optional>
#include <unordered_map>
#include <utility>

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::uint32_t LogicalPageIdOf(std::span<const std::uint8_t> pageBytes)
{
    return infrastructure::ReadUInt32LE(pageBytes, LogicalPageIdOffset) & LogicalPageIdMask;
}

std::set<std::size_t> FindCurrentPhysicalPages(const domain::IPageStorage& storage)
{
    const std::size_t pageCount = storage.PageCount();

    std::unordered_map<std::uint32_t, std::size_t> latestPhysicalPageByLogicalId;
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::uint32_t logicalId = LogicalPageIdOf(storage.PageBytes(pageNumber));
        auto [it, inserted] = latestPhysicalPageByLogicalId.try_emplace(logicalId, pageNumber);
        if (!inserted && pageNumber > it->second)
        {
            it->second = pageNumber;
        }
    }

    std::set<std::size_t> currentPages;
    for (const auto& [logicalId, physicalPage] : latestPhysicalPageByLogicalId)
    {
        currentPages.insert(physicalPage);
    }
    return currentPages;
}

}

CatalogPageScanner::CatalogPageScanner(std::shared_ptr<ILogicalPageResolver> logicalPageResolver)
    : _logicalPageResolver(std::move(logicalPageResolver))
{
}

std::set<std::uint8_t> CatalogPageScanner::FindCatalogObjectIds(const domain::IPageStorage& storage) const
{
    (void)storage;
    return {SystemCatalogObjectId};
}

void CatalogPageScanner::AssignDataPages(
    const domain::IPageStorage& storage,
    const std::map<std::pair<std::uint8_t, std::uint8_t>, domain::TableDef*>& tableByObjectKey) const
{
    const std::set<std::uint8_t> catalogObjectIds = FindCatalogObjectIds(storage);
    const std::size_t pageCount = storage.PageCount();
    const std::set<std::size_t> currentPages = FindCurrentPhysicalPages(storage);

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        if (currentPages.find(pageNumber) == currentPages.end())
        {
            continue;
        }

        const PageView page(storage.PageBytes(pageNumber));
        if (!page.IsDataPage())
        {
            continue;
        }
        if (catalogObjectIds.find(page.OwnerObjectId()) != catalogObjectIds.end())
        {
            continue;
        }

        const auto it = tableByObjectKey.find({page.OwnerObjectId(), page.OwnerObjectGeneration()});
        if (it != tableByObjectKey.end())
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
    const std::set<std::size_t> currentPages = FindCurrentPhysicalPages(storage);

    auto isCatalogPage = [&](std::size_t pageNumber) -> bool
    {
        if (currentPages.find(pageNumber) == currentPages.end())
        {
            return false;
        }
        const PageView page(storage.PageBytes(pageNumber));
        return page.IsDataPage() && catalogObjectIds.find(page.OwnerObjectId()) != catalogObjectIds.end();
    };

    auto resolveCatalogContinuationTarget = [&](const ContinuedRowSlice& slice) -> std::optional<std::size_t>
    {
        if (!slice.hasContinuation)
        {
            return std::nullopt;
        }
        const std::optional<std::size_t> physicalPage = _logicalPageResolver->ResolvePhysicalPage(storage, slice.continuationLogicalPageId);
        if (!physicalPage.has_value() || !isCatalogPage(*physicalPage))
        {
            return std::nullopt;
        }
        return physicalPage;
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
            const std::optional<std::size_t> target = resolveCatalogContinuationTarget(slice);
            if (target.has_value())
            {
                continuationTargets.emplace(*target, slice.continuationSlotIndex);
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

            std::optional<std::size_t> nextPageNumber = resolveCatalogContinuationTarget(slice);
            std::size_t nextSlotIndex = slice.continuationSlotIndex;
            std::size_t hops = 0;

            while (nextPageNumber.has_value() && hops < MaxRowContinuationHops)
            {
                ++hops;
                const PageView nextPage(storage.PageBytes(*nextPageNumber));

                bool matched = false;
                for (const ContinuedRowSlice& nextSlice : nextPage.RowsWithContinuation())
                {
                    if (nextSlice.slotIndex != nextSlotIndex)
                    {
                        continue;
                    }
                    assembled.insert(assembled.end(), nextSlice.bytes.begin(), nextSlice.bytes.end());
                    matched = true;
                    nextSlotIndex = nextSlice.continuationSlotIndex;
                    nextPageNumber = resolveCatalogContinuationTarget(nextSlice);
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
