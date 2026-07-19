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
    return _bytes[domain::PageTypeOffset];
}

std::uint8_t PageView::OwnerObjectId() const
{
    return _bytes[domain::OwnerObjectIdOffset];
}

bool PageView::IsDataPage() const
{
    return PageType() == domain::DataPageType;
}

std::size_t PageView::SlotCount() const
{
    return ReadU32(_bytes, domain::SlotCountFieldOffset) & domain::SlotCountFieldMask;
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
        const std::size_t slotOffset = domain::SlotEntryLength * slotPosition;
        if (slotOffset + domain::RowHeaderLength > domain::PageSize)
        {
            continue;
        }

        const std::uint8_t flags = bytes[slotOffset + domain::SlotEntryFlagsOffset];
        if ((flags & domain::SlotFlagGhost) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(bytes, slotOffset + domain::SlotEntryDwordOffset);
        const std::size_t recordOffset = slotDword & domain::RecordOffsetMask;
        const std::uint32_t recordLenField = (slotDword >> domain::RecordLengthFieldShift) & domain::RecordLengthFieldMask;

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
            if (recordLenField < domain::RecordLengthFieldBias)
            {
                return std::nullopt;
            }
            const std::size_t recordLength = recordLenField - domain::RecordLengthFieldBias;
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
        const std::size_t slotOffset = domain::SlotEntryLength * slotPosition;
        if (slotOffset + domain::RowHeaderLength > domain::PageSize)
        {
            continue;
        }

        const std::uint8_t flags = _bytes[slotOffset + domain::SlotEntryFlagsOffset];
        if ((flags & domain::SlotFlagGhost) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(_bytes, slotOffset + domain::SlotEntryDwordOffset);
        const std::size_t recordOffset = slotDword & domain::RecordOffsetMask;
        const std::uint32_t recordLenField = (slotDword >> domain::RecordLengthFieldShift) & domain::RecordLengthFieldMask;
        if (recordLenField < domain::RecordLengthFieldBias)
        {
            continue;
        }
        const std::size_t recordLength = recordLenField - domain::RecordLengthFieldBias;
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

        if (recordOffset + domain::RowContinuationDwordOffset + domain::SlotEntryLength > domain::PageSize)
        {
            continue;
        }
        const std::uint32_t continuationDword = ReadU32(_bytes, recordOffset + domain::RowContinuationDwordOffset);

        ContinuedRowSlice slice{
            slotIndex, recordOffset, _bytes.subspan(start, recordLength), (flags & domain::SlotFlagFirstFragment) != 0,
            false, 0, 0};
        if (continuationDword != 0)
        {
            const std::size_t nextSlotIndex = continuationDword & domain::ContinuationSlotIndexMask;
            const std::size_t marker = continuationDword >> domain::ContinuationMarkerShift;

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
