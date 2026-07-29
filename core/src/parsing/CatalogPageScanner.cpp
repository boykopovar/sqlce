#include "sdf/parsing/CatalogPageScanner.hpp"

#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::uint32_t ReadPackedSlot(std::span<const std::uint8_t> pageBytes, std::size_t baseOffset, std::size_t slotIndex)
{
    const std::size_t wordIndex = (LvBitsPerSlot * slotIndex) / (LvSlotsPerWord * LvBitsPerSlot);
    const std::size_t bitOffset = (LvBitsPerSlot * slotIndex) % (LvSlotsPerWord * LvBitsPerSlot);
    const std::size_t wordByteOffset = baseOffset + wordIndex * LvWordBytes;

    if (wordByteOffset + LvWordBytes > pageBytes.size())
    {
        return 0;
    }

    const std::uint64_t word = infrastructure::ReadUInt64LE(pageBytes, wordByteOffset);
    return static_cast<std::uint32_t>((word >> bitOffset) & LvSlotValueMask);
}

std::vector<std::uint32_t> DecodeSpaceMapLogicalPageIds(std::span<const std::uint8_t> rootPageBytes)
{
    std::vector<std::uint32_t> logicalPageIds;

    const std::size_t mapOffset = SpaceMapBaseOffset;
    if (mapOffset + SpaceMapStride > rootPageBytes.size())
    {
        return logicalPageIds;
    }

    const std::uint32_t count = infrastructure::ReadUInt32LE(rootPageBytes, mapOffset + SpaceMapCountOffset);
    const std::uint32_t indirect = infrastructure::ReadUInt32LE(rootPageBytes, mapOffset + SpaceMapIndirectFlagOffset);
    if (indirect != SpaceMapIndirectModeInline)
    {
        return logicalPageIds;
    }

    const std::size_t slotBase = mapOffset + SpaceMapSlotsOffset;
    for (std::size_t slotIndex = 0; slotIndex < count; ++slotIndex)
    {
        const std::size_t wordIndex = (LvBitsPerSlot * slotIndex) / (LvSlotsPerWord * LvBitsPerSlot);
        const std::size_t wordByteOffset = slotBase + wordIndex * LvWordBytes;
        if (wordByteOffset + LvWordBytes > mapOffset + SpaceMapStride)
        {
            break;
        }

        const std::uint32_t logicalPageId = ReadPackedSlot(rootPageBytes, slotBase, slotIndex);
        if (logicalPageId != 0)
        {
            logicalPageIds.push_back(logicalPageId);
        }
    }

    return logicalPageIds;
}

}

CatalogPageScanner::CatalogPageScanner(std::shared_ptr<ILogicalPageResolver> logicalPageResolver)
    : _logicalPageResolver(std::move(logicalPageResolver))
{
}

std::vector<std::size_t> CatalogPageScanner::_ResolveHeapPagesFromRoot(
    const domain::IPageStorage& storage, std::uint32_t rootLogicalPageId) const
{
    std::vector<std::size_t> heapPageNumbers;

    const std::optional<std::size_t> rootPhysicalPage
        = _logicalPageResolver->ResolvePhysicalPage(storage, rootLogicalPageId);
    if (!rootPhysicalPage.has_value())
    {
        return heapPageNumbers;
    }

    const PageView rootPage(storage.PageBytes(*rootPhysicalPage));
    if (rootPage.PageType() != TableRootPageType)
    {
        return heapPageNumbers;
    }

    for (const std::uint32_t heapLogicalPageId : DecodeSpaceMapLogicalPageIds(rootPage.Bytes()))
    {
        const std::optional<std::size_t> heapPhysicalPage
            = _logicalPageResolver->ResolvePhysicalPage(storage, heapLogicalPageId);
        if (heapPhysicalPage.has_value())
        {
            heapPageNumbers.push_back(*heapPhysicalPage);
        }
    }

    return heapPageNumbers;
}

void CatalogPageScanner::AssignDataPages(
    const domain::IPageStorage& storage, const std::vector<domain::TableDef*>& tables) const
{
    for (domain::TableDef* table : tables)
    {
        for (const std::size_t heapPageNumber : _ResolveHeapPagesFromRoot(storage, table->RootLogicalPageId()))
        {
            table->DataPageNumbers().push_back(heapPageNumber);
        }
    }
}

std::vector<std::vector<std::uint8_t>> CatalogPageScanner::CollectCatalogRows(
    const domain::IPageStorage& storage) const
{
    const std::vector<std::size_t> catalogPageNumbers
        = _ResolveHeapPagesFromRoot(storage, SystemCatalogRootLogicalPageId);
    const std::set<std::size_t> catalogPageSet(catalogPageNumbers.begin(), catalogPageNumbers.end());

    auto isCatalogPage = [&](std::size_t pageNumber) -> bool
    {
        return catalogPageSet.find(pageNumber) != catalogPageSet.end();
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
    for (const std::size_t pageNumber : catalogPageNumbers)
    {
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

    for (const std::size_t pageNumber : catalogPageNumbers)
    {
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
