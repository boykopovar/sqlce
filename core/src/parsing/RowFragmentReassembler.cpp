#include "sdf/parsing/RowFragmentReassembler.hpp"

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

std::vector<std::uint8_t> RowFragmentReassembler::AssembleRowBytes(
    const domain::IPageStorage& storage, std::size_t slotIndex, const std::vector<ContinuedRowSlice>& pageRows) const
{
    const ContinuedRowSlice& first = pageRows[slotIndex];
    std::vector<std::uint8_t> assembled(first.bytes.begin(), first.bytes.end());

    bool continued = first.hasContinuation;
    std::size_t nextPageNumber = first.continuationPageNumber;
    std::size_t nextSlotIndex = first.continuationSlotIndex;
    std::size_t hops = 0;

    while (continued && hops < MaxRowContinuationHops)
    {
        ++hops;
        const std::vector<ContinuedRowSlice> nextPageRows = RowsOnPage(storage, nextPageNumber);

        bool matched = false;
        for (const ContinuedRowSlice& nextSlice : nextPageRows)
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

    return assembled;
}

std::optional<AssembledRow> RowFragmentReassembler::FindAtOrAfter(
    const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor cursor) const
{
    std::size_t pageIndex = cursor.pageIndex;
    std::size_t slotIndex = cursor.slotIndex;

    while (pageIndex < dataPageNumbers.size())
    {
        const std::vector<ContinuedRowSlice> pageRows = RowsOnPage(storage, dataPageNumbers[pageIndex]);

        while (slotIndex < pageRows.size() && !pageRows[slotIndex].isFirstFragment)
        {
            ++slotIndex;
        }

        if (slotIndex < pageRows.size())
        {
            AssembledRow result;
            result.cursor = RowCursor{pageIndex, slotIndex};
            result.bytes = AssembleRowBytes(storage, slotIndex, pageRows);
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
