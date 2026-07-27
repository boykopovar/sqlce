#include "sdf/parsing/RowFragmentReassembler.hpp"

#include <utility>

#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::vector<ContinuedRowSlice> RowsOnPage(const domain::IPageStorage& storage, std::size_t pageNumber)
{
    const std::span<const std::uint8_t> pageBytes = storage.PageBytes(pageNumber);
    return PageView(pageBytes).RowsWithContinuation();
}

}

RowFragmentReassembler::RowFragmentReassembler(std::shared_ptr<ILogicalPageResolver> logicalPageResolver)
    : _logicalPageResolver(std::move(logicalPageResolver))
{
}

std::vector<std::uint8_t> RowFragmentReassembler::AssembleRowBytes(
    const domain::IPageStorage& storage, std::uint8_t expectedOwnerObjectId, std::size_t slotIndex,
    const std::vector<ContinuedRowSlice>& pageRows) const
{
    const ContinuedRowSlice& first = pageRows[slotIndex];
    std::vector<std::uint8_t> assembled(first.bytes.begin(), first.bytes.end());

    auto resolveContinuationTarget = [&](const ContinuedRowSlice& slice) -> std::optional<std::size_t>
    {
        if (!slice.hasContinuation)
        {
            return std::nullopt;
        }
        const std::optional<std::size_t> physicalPage
            = _logicalPageResolver->ResolvePhysicalPage(storage, slice.continuationLogicalPageId);
        if (!physicalPage.has_value())
        {
            return std::nullopt;
        }
        const PageView targetPage(storage.PageBytes(*physicalPage));
        if (!targetPage.IsDataPage() || targetPage.OwnerObjectId() != expectedOwnerObjectId)
        {
            return std::nullopt;
        }
        return physicalPage;
    };

    std::optional<std::size_t> nextPageNumber = resolveContinuationTarget(first);
    std::size_t nextSlotIndex = first.continuationSlotIndex;
    std::size_t hops = 0;

    while (nextPageNumber.has_value() && hops < MaxRowContinuationHops)
    {
        ++hops;
        const std::vector<ContinuedRowSlice> nextPageRows = RowsOnPage(storage, *nextPageNumber);

        bool matched = false;
        for (const ContinuedRowSlice& nextSlice : nextPageRows)
        {
            if (nextSlice.slotIndex != nextSlotIndex)
            {
                continue;
            }
            assembled.insert(assembled.end(), nextSlice.bytes.begin(), nextSlice.bytes.end());
            matched = true;
            nextSlotIndex = nextSlice.continuationSlotIndex;
            nextPageNumber = resolveContinuationTarget(nextSlice);
            break;
        }

        if (!matched)
        {
            break;
        }
    }

    return assembled;
}

std::optional<AssembledRow> RowFragmentReassembler::FindAtOrAfter(
    const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor cursor) const
{
    std::size_t pageIndex = cursor.pageIndex;
    std::size_t slotIndex = cursor.slotIndex;

    while (pageIndex < dataPageNumbers.size())
    {
        const std::size_t physicalPageNumber = dataPageNumbers[pageIndex];
        const PageView currentPage(storage.PageBytes(physicalPageNumber));
        const std::uint8_t expectedOwnerObjectId = currentPage.OwnerObjectId();
        const std::vector<ContinuedRowSlice> pageRows = RowsOnPage(storage, physicalPageNumber);

        while (slotIndex < pageRows.size() && !pageRows[slotIndex].isFirstFragment)
        {
            ++slotIndex;
        }

        if (slotIndex < pageRows.size())
        {
            AssembledRow result;
            result.cursor = RowCursor{pageIndex, slotIndex};
            result.bytes = AssembleRowBytes(storage, expectedOwnerObjectId, slotIndex, pageRows);
            return result;
        }

        slotIndex = 0;
        ++pageIndex;
    }

    return std::nullopt;
}

std::optional<AssembledRow> RowFragmentReassembler::FindFirst(
    const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor from) const
{
    return FindAtOrAfter(storage, dataPageNumbers, from);
}

std::optional<AssembledRow> RowFragmentReassembler::FindNext(
    const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor after) const
{
    return FindAtOrAfter(storage, dataPageNumbers, RowCursor{after.pageIndex, after.slotIndex + 1});
}

}
