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

class PageView
{
public:
    explicit PageView(std::span<const std::uint8_t> bytes);

    std::uint8_t PageType() const;
    std::uint8_t OwnerObjectId() const;
    bool IsDataPage() const;
    std::size_t SlotCount() const;

    std::vector<RowSlice> Rows() const;

    std::span<const std::uint8_t> Bytes() const;

private:
    std::span<const std::uint8_t> _bytes;
};

}

#endif
