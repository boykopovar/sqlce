#ifndef SDF_INFRASTRUCTURE_PAGE_VIEW_HPP
#define SDF_INFRASTRUCTURE_PAGE_VIEW_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sdf::infrastructure
{

struct RowSlice
{
    std::size_t slotIndex;
    std::span<const std::uint8_t> bytes;
};

struct ContinuedRowSlice
{
    std::size_t slotIndex;
    std::size_t recordOffset;
    std::span<const std::uint8_t> bytes;
    bool isFirstFragment;
    bool hasContinuation;
    std::size_t continuationPageNumber;
    std::size_t continuationSlotIndex;
};

class PageView
{
public:
    explicit PageView(std::span<const std::uint8_t> bytes);

    [[nodiscard]] std::uint8_t PageType() const;
    [[nodiscard]] std::uint8_t OwnerObjectId() const;
    [[nodiscard]] bool IsDataPage() const;
    [[nodiscard]] std::size_t SlotCount() const;

    [[nodiscard]] std::vector<RowSlice> Rows() const;
    [[nodiscard]] std::vector<ContinuedRowSlice> RowsWithContinuation() const;

    [[nodiscard]] std::span<const std::uint8_t> Bytes() const;

private:
    std::span<const std::uint8_t> _bytes;
};

}

#endif
