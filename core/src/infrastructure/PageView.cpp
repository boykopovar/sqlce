#include "sdf/infrastructure/PageView.hpp"

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

std::vector<RowSlice> PageView::Rows() const
{
    std::vector<RowSlice> result;
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

        result.push_back(RowSlice{slotIndex, _bytes.subspan(start, recordLength)});
    }

    return result;
}

}
