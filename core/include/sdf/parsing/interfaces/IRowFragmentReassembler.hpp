#ifndef SDF_PARSING_I_ROW_FRAGMENT_REASSEMBLER_HPP
#define SDF_PARSING_I_ROW_FRAGMENT_REASSEMBLER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "sdf/domain/interfaces/IPageStorage.hpp"

namespace sdf::parsing
{

struct RowCursor
{
    std::size_t pageIndex;
    std::size_t slotIndex;
};

struct AssembledRow
{
    RowCursor cursor;
    std::vector<std::uint8_t> bytes;
};

class IRowFragmentReassembler
{
public:
    virtual ~IRowFragmentReassembler() = default;

    virtual std::optional<AssembledRow> FindFirst(
        const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor from) const
        = 0;

    virtual std::optional<AssembledRow> FindNext(
        const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor after) const
        = 0;
};

}

#endif
