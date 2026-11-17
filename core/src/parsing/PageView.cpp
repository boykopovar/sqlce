#include "sdf/parsing/PageView.hpp"

#include <optional>

#include "sdf/domain/InvalidSdfException.hpp"
#include "sdf/domain/PageSize.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::uint32_t ReadU32(const std::span<const std::uint8_t> bytes, const std::size_t offset)
{
    return static_cast<std::uint32_t>(bytes[offset]) | (static_cast<std::uint32_t>(bytes[offset + 1]) << 8)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 16)
        | (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

}

PageView::PageView(std::span<const std::uint8_t> bytes) : _bytes(bytes)
{
    if (bytes.size() != domain::PageSize)
    {
        throw domain::InvalidSdfException("page length is not 4096 bytes");
    }
}

std::uint8_t PageView::PageType() const
{
    return _bytes[PageTypeOffset];
}

std::uint8_t PageView::OwnerObjectId() const
{
    return _bytes[OwnerObjectIdOffset];
}

std::uint8_t PageView::OwnerObjectGeneration() const
{
    return _bytes[OwnerObjectGenerationOffset];
}

bool PageView::IsDataPage() const
{
    return PageType() == DataPageType;
}

std::size_t PageView::SlotCount() const
{
    const std::size_t count = ReadU32(_bytes, SlotCountFieldOffset) & SlotCountFieldMask;
    // Validate before subtracting slot indices from SlotArrayBaseIndex.
    if (count > SlotArrayBaseIndex + 1)
    {
        throw domain::InvalidSdfException("slot count exceeds page directory bounds");
    }
    return count;
}

std::span<const std::uint8_t> PageView::Bytes() const
{
    return _bytes;
}

namespace
{

template <typename TBaseFn, typename TLengthFn>
std::vector<RowSlice> CollectRows(
    std::span<const std::uint8_t> bytes, std::size_t slotCount, TBaseFn&& computeBase, TLengthFn&& computeLength)
{
    std::vector<RowSlice> result;
    result.reserve(slotCount);

    for (std::size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex)
    {
        const std::size_t slotPosition = SlotArrayBaseIndex - slotIndex;
        const std::size_t slotOffset = SlotEntryLength * slotPosition;
        if (slotOffset > bytes.size() || RowHeaderLength > bytes.size() - slotOffset)
        {
            throw domain::InvalidSdfException("slot entry outside page");
        }

        const std::uint8_t flags = bytes[slotOffset + SlotEntryFlagsOffset];
        if ((flags & SlotFlagGhost) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(bytes, slotOffset + SlotEntryDwordOffset);
        const std::size_t recordOffset = slotDword & RecordOffsetMask;
        const std::uint32_t recordLenField = (slotDword >> RecordLengthFieldShift) & RecordLengthFieldMask;

        const auto lengthResult = computeLength(recordLenField);
        if (!lengthResult.has_value())
        {
            continue;
        }
        const std::size_t recordLength = *lengthResult;

        const std::size_t start = computeBase(recordOffset);
        if (start > bytes.size() || recordLength > bytes.size() - start)
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
        [](std::size_t recordOffset) { return recordOffset + RowHeaderLength; },
        [](std::uint32_t recordLenField) -> std::optional<std::size_t>
        {
            if (recordLenField < RecordLengthFieldBias)
            {
                return std::nullopt;
            }
            const std::size_t recordLength = recordLenField - RecordLengthFieldBias;
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
        const std::size_t slotPosition = SlotArrayBaseIndex - slotIndex;
        const std::size_t slotOffset = SlotEntryLength * slotPosition;
        if (slotOffset > _bytes.size() || RowHeaderLength > _bytes.size() - slotOffset)
        {
            throw domain::InvalidSdfException("slot entry outside page");
        }

        const std::uint8_t flags = _bytes[slotOffset + SlotEntryFlagsOffset];
        if ((flags & SlotFlagGhost) != 0)
        {
            continue;
        }

        const std::uint32_t slotDword = ReadU32(_bytes, slotOffset + SlotEntryDwordOffset);
        const std::size_t recordOffset = slotDword & RecordOffsetMask;
        const std::uint32_t recordLenField = (slotDword >> RecordLengthFieldShift) & RecordLengthFieldMask;
        if (recordLenField < RecordLengthFieldBias)
        {
            continue;
        }
        const std::size_t recordLength = recordLenField - RecordLengthFieldBias;
        if (recordLength == 0)
        {
            continue;
        }

        const std::size_t start = recordOffset + RowHeaderLength;
        if (start > _bytes.size() || recordLength > _bytes.size() - start)
        {
            continue;
        }

        if (recordOffset + RowContinuationDwordOffset + SlotEntryLength > domain::PageSize)
        {
            continue;
        }
        const std::uint32_t continuationDword = ReadU32(_bytes, recordOffset + RowContinuationDwordOffset);

        ContinuedRowSlice slice{
            slotIndex, recordOffset, _bytes.subspan(start, recordLength), (flags & SlotFlagFirstFragment) != 0,
            false, 0, 0};
        if (continuationDword != 0)
        {
            slice.hasContinuation = true;
            slice.continuationSlotIndex = continuationDword & ContinuationSlotIndexMask;
            slice.continuationLogicalPageId = continuationDword >> ContinuationLogicalPageIdShift;
        }

        result.push_back(slice);
    }

    return result;
}

}
