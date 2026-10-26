#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>

#include "sdf/domain/InvalidSdfException.hpp"
#include "sdf/domain/PageSize.hpp"
#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace
{

void WriteUInt32(std::span<std::uint8_t> bytes, std::size_t offset, std::uint32_t value)
{
    for (std::size_t i = 0; i < 4; ++i)
    {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
    }
}

}

int main()
{
    using namespace sdf::parsing;
    using sdf::domain::InvalidSdfException;
    using sdf::domain::PageSize;

    std::array<std::uint8_t, PageSize + 1> buffer{};
    for (std::size_t size = 0; size < PageSize; ++size)
    {
        try
        {
            PageView page{std::span(buffer).first(size)};
            std::cerr << "short page was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const InvalidSdfException&)
        {
        }
    }
    try
    {
        PageView page(buffer);
        std::cerr << "oversized page was accepted\n";
        return EXIT_FAILURE;
    }
    catch (const InvalidSdfException&)
    {
    }

    auto bytes = std::span(buffer).first(PageSize);
    // Exercise the entire encoded count range, including the subtraction boundary.
    for (std::uint32_t count = 0; count <= SlotCountFieldMask; ++count)
    {
        WriteUInt32(bytes, SlotCountFieldOffset, count);
        PageView page(bytes);
        if (count > SlotArrayBaseIndex + 1)
        {
            try
            {
                (void)page.SlotCount();
                std::cerr << "excessive slot count was accepted\n";
                return EXIT_FAILURE;
            }
            catch (const InvalidSdfException&)
            {
            }
            try
            {
                (void)page.Rows();
                std::cerr << "row scan accepted excessive slot count\n";
                return EXIT_FAILURE;
            }
            catch (const InvalidSdfException&)
            {
            }
            try
            {
                (void)page.RowsWithContinuation();
                std::cerr << "continuation scan accepted excessive slot count\n";
                return EXIT_FAILURE;
            }
            catch (const InvalidSdfException&)
            {
            }
        }
        else
        {
            if (page.SlotCount() != count)
            {
                std::cerr << "bounded slot count changed\n";
                return EXIT_FAILURE;
            }
            (void)page.Rows();
            (void)page.RowsWithContinuation();
        }
    }

    buffer.fill(0);
    bytes[PageTypeOffset] = DataPageType;
    WriteUInt32(bytes, SlotCountFieldOffset, 1);
    constexpr std::size_t slotDwordOffset = SlotArrayBaseIndex * SlotEntryLength + SlotEntryDwordOffset;
    WriteUInt32(bytes, slotDwordOffset, 100 | (7 << RecordLengthFieldShift) | (2u << 24));
    bytes[128] = 7;
    bytes[129] = 8;
    bytes[130] = 9;
    PageView page(bytes);
    if (!page.IsDataPage())
    {
        std::cerr << "data page type changed\n";
        return EXIT_FAILURE;
    }
    const auto rows = page.Rows();
    if (rows.size() != 1 || rows[0].slotIndex != 0 || rows[0].bytes.size() != 3
        || rows[0].bytes[2] != 9)
    {
        std::cerr << "valid row changed\n";
        return EXIT_FAILURE;
    }
    WriteUInt32(bytes, 100 + RowContinuationDwordOffset, (1234 << ContinuationLogicalPageIdShift) | 5);
    const auto continued = page.RowsWithContinuation();
    if (continued.size() != 1 || !continued[0].isFirstFragment || !continued[0].hasContinuation
        || continued[0].continuationLogicalPageId != 1234 || continued[0].continuationSlotIndex != 5)
    {
        std::cerr << "valid continuation changed\n";
        return EXIT_FAILURE;
    }

    WriteUInt32(bytes, slotDwordOffset, 0x01ffffff);
    if (!page.Rows().empty() || !page.RowsWithContinuation().empty())
    {
        std::cerr << "ghost slot was not skipped\n";
        return EXIT_FAILURE;
    }
    WriteUInt32(bytes, slotDwordOffset, 4090 | (64 << RecordLengthFieldShift));
    if (!page.Rows().empty() || !page.RowsWithContinuation().empty())
    {
        std::cerr << "row recovery behavior changed\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
