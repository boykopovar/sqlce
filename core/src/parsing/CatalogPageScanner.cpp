#include "sdf/parsing/CatalogPageScanner.hpp"

#include <optional>
#include <set>
#include <span>
#include <utility>
#include <vector>

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"
#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"

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

bool BitmapBitSet(std::span<const std::uint8_t> bitmapPageBytes, std::uint32_t bitPosition)
{
    const std::size_t byteOffset = BitmapPageBitsOffset + (bitPosition >> 3);
    if (byteOffset >= bitmapPageBytes.size())
    {
        return false;
    }
    return ((bitmapPageBytes[byteOffset] >> (bitPosition & 7)) & 0x01u) != 0;
}

std::vector<std::uint32_t> DecodeIndirectSpaceMapLogicalPageIds(
    const domain::IPageStorage& storage, const ILogicalPageResolver& logicalPageResolver,
    std::span<const std::uint8_t> rootPageBytes, std::size_t slotBase, std::size_t slotRegionEnd)
{
    std::vector<std::uint32_t> logicalPageIds;

    for (std::size_t groupIndex = 0; groupIndex < SpaceMapIndirectGroupCount; ++groupIndex)
    {
        const std::size_t wordIndex = (LvBitsPerSlot * groupIndex) / (LvSlotsPerWord * LvBitsPerSlot);
        const std::size_t wordByteOffset = slotBase + wordIndex * LvWordBytes;
        if (wordByteOffset + LvWordBytes > slotRegionEnd)
        {
            break;
        }

        const std::uint32_t bitmapPageLogicalId = ReadPackedSlot(rootPageBytes, slotBase, groupIndex);
        if (bitmapPageLogicalId == 0)
        {
            continue;
        }

        const std::optional<std::size_t> bitmapPhysicalPage
            = logicalPageResolver.ResolvePhysicalPage(storage, bitmapPageLogicalId);
        if (!bitmapPhysicalPage.has_value())
        {
            continue;
        }

        const std::span<const std::uint8_t> bitmapPageBytes = storage.PageBytes(*bitmapPhysicalPage);
        if (bitmapPageBytes.size() <= BitmapPageLastSetOffset + 1 || bitmapPageBytes[PageTypeOffset] != BitmapPageType)
        {
            continue;
        }

        const std::uint16_t firstSet = infrastructure::ReadUInt16LE(bitmapPageBytes, BitmapPageFirstSetOffset);
        const std::uint16_t lastSet = infrastructure::ReadUInt16LE(bitmapPageBytes, BitmapPageLastSetOffset);

        for (std::uint32_t bitPosition = firstSet; bitPosition <= lastSet; ++bitPosition)
        {
            if (BitmapBitSet(bitmapPageBytes, bitPosition))
            {
                logicalPageIds.push_back(bitPosition + SpaceMapIndirectGroupSpan * static_cast<std::uint32_t>(groupIndex));
            }
        }
    }

    return logicalPageIds;
}

std::vector<std::uint32_t> DecodeSpaceMapLogicalPageIds(
    const domain::IPageStorage& storage, const ILogicalPageResolver& logicalPageResolver,
    std::span<const std::uint8_t> rootPageBytes)
{
    std::vector<std::uint32_t> logicalPageIds;

    const std::size_t mapOffset = SpaceMapBaseOffset;
    if (mapOffset + SpaceMapStride > rootPageBytes.size())
    {
        return logicalPageIds;
    }

    const std::uint32_t count = infrastructure::ReadUInt32LE(rootPageBytes, mapOffset + SpaceMapCountOffset);
    const std::uint32_t indirect = infrastructure::ReadUInt32LE(rootPageBytes, mapOffset + SpaceMapIndirectFlagOffset);
    const std::size_t slotBase = mapOffset + SpaceMapSlotsOffset;
    const std::size_t slotRegionEnd = mapOffset + SpaceMapStride;

    if (indirect != SpaceMapIndirectModeInline)
    {
        return DecodeIndirectSpaceMapLogicalPageIds(storage, logicalPageResolver, rootPageBytes, slotBase, slotRegionEnd);
    }

    for (std::size_t slotIndex = 0; slotIndex < count; ++slotIndex)
    {
        const std::size_t wordIndex = (LvBitsPerSlot * slotIndex) / (LvSlotsPerWord * LvBitsPerSlot);
        const std::size_t wordByteOffset = slotBase + wordIndex * LvWordBytes;
        if (wordByteOffset + LvWordBytes > slotRegionEnd)
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

    for (const std::uint32_t heapLogicalPageId : DecodeSpaceMapLogicalPageIds(storage, *_logicalPageResolver, rootPage.Bytes()))
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
