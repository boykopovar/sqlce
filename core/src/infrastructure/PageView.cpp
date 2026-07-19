#include "sdf/infrastructure/PageView.hpp"

#include <optional>

#include "sdf/domain/PageLayout.hpp"

namespace sdf::infrastructure
{

namespace
{

std::uint32_t ReadU32(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

}

PageView::PageView(std::span<const std::uint8_t> bytes) : _bytes(bytes)
{
}

std::uint8_t PageView::PageType() const
{
    return _bytes[6];
}

std::uint8_t PageView::OwnerObjectId() const
{
    return _bytes[16];
}

bool PageView::IsDataPage() const
{
    return PageType() == domain::DataPageType;
}

std::size_t PageView::SlotCount() const
{
    return ReadU32(_bytes, 20) & 0xFFFu;
}

std::span<const std::uint8_t> PageView::Bytes() const
{
    return _bytes;
}

namespace
{

template <typename BaseFn, typename LengthFn>
std::vector<RowSlice> CollectRows(
    std::span<const std::uint8_t> bytes, std::size_t slotCount, BaseFn&& computeBase, LengthFn&& computeLength)
{
    std::vector<RowSlice> result;
    result.reserve(slotCount);

    for (std::size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        const std::size_t slotPosition = domain::SlotArrayBaseIndex - slotIndex;
        const std::size_t slotOffset = 4 * slotPosition;
        if (slotOffset + 28 > domain::PageSize)
        {
            continue;
        }

        const std::uint8_t flags = bytes[slotOffset + 27];
        if ((flags & 0x01u) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(bytes, slotOffset + 24);
        const std::size_t recordOffset = slotDword & 0xFFFu;
        const std::uint32_t recordLenField = (slotDword >> 12) & 0xFFFu;

        const auto lengthResult = computeLength(recordLenField);
        if (!lengthResult.has_value())
        {
            continue;
        }
        const std::size_t recordLength = *lengthResult;

        const std::size_t start = computeBase(recordOffset);
        const std::size_t end = start + recordLength;
        if (end > domain::PageSize || start > end)
        {
            continue;
        }

        result.push_back(RowSlice{slotIndex, bytes.subspan(start, recordLength)});
    }

    return result;
}

}

std::vector<RowSlice> PageView::Rows() const
{
    return CollectRows(
        _bytes, SlotCount(),
        [](std::size_t recordOffset) { return recordOffset + domain::RowHeaderLength; },
        [](std::uint32_t recordLenField) -> std::optional<std::size_t>
        {
            if (recordLenField < 4)
            {
                return std::nullopt;
            }
            const std::size_t recordLength = recordLenField - 4;
            if (recordLength == 0)
            {
                return std::nullopt;
            }
            return recordLength;
        });
}

std::vector<ContinuedRowSlice> PageView::RowsWithContinuation() const
{
    std::vector<ContinuedRowSlice> result;
    const std::size_t slotCount = SlotCount();
    result.reserve(slotCount);

    for (std::size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        const std::size_t slotPosition = domain::SlotArrayBaseIndex - slotIndex;
        const std::size_t slotOffset = 4 * slotPosition;
        if (slotOffset + 28 > domain::PageSize)
        {
            continue;
        }

        const std::uint8_t flags = _bytes[slotOffset + 27];
        if ((flags & 0x01u) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(_bytes, slotOffset + 24);
        const std::size_t recordOffset = slotDword & 0xFFFu;
        const std::uint32_t recordLenField = (slotDword >> 12) & 0xFFFu;
        if (recordLenField < 4)
        {
            continue;
        }
        const std::size_t recordLength = recordLenField - 4;
        if (recordLength == 0)
        {
            continue;
        }

        const std::size_t start = recordOffset + domain::RowHeaderLength;
        const std::size_t end = start + recordLength;
        if (end > domain::PageSize || start > end)
        {
            continue;
        }

        if (recordOffset + domain::RowContinuationDwordOffset + 4 > domain::PageSize)
        {
            continue;
        }
        const std::uint32_t continuationDword = ReadU32(_bytes, recordOffset + domain::RowContinuationDwordOffset);

        ContinuedRowSlice slice{slotIndex, recordOffset, _bytes.subspan(start, recordLength), (flags & 0x02u) != 0, false, 0, 0};
        if (continuationDword != 0)
        {
            const std::size_t nextSlotIndex = continuationDword & 0xFFFu;
            const std::size_t marker = continuationDword >> 12;

            if (marker > domain::ContinuationPageNumberBias)
            {
                slice.hasContinuation = true;
                slice.continuationPageNumber = marker - domain::ContinuationPageNumberBias;
                slice.continuationSlotIndex = nextSlotIndex;
            }
        }

        result.push_back(slice);
    }

    return result;
}

}
