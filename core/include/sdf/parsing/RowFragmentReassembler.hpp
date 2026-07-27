#ifndef SDF_PARSING_ROW_FRAGMENT_REASSEMBLER_HPP
#define SDF_PARSING_ROW_FRAGMENT_REASSEMBLER_HPP

#include <memory>

#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"
#include "sdf/parsing/interfaces/IRowFragmentReassembler.hpp"
#include "sdf/parsing/PageView.hpp"

namespace sdf::parsing
{

class RowFragmentReassembler final : public IRowFragmentReassembler
{
public:
    explicit RowFragmentReassembler(std::shared_ptr<ILogicalPageResolver> logicalPageResolver);

    [[nodiscard]] std::optional<AssembledRow> FindFirst(
        const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor from)
        const override;

    [[nodiscard]] std::optional<AssembledRow> FindNext(
        const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor after)
        const override;

private:
    [[nodiscard]] std::optional<AssembledRow> FindAtOrAfter(
        const domain::IPageStorage& storage, const std::vector<std::size_t>& dataPageNumbers, RowCursor cursor)
        const;

    [[nodiscard]] std::vector<std::uint8_t> AssembleRowBytes(
        const domain::IPageStorage& storage, std::uint8_t expectedOwnerObjectId, std::size_t slotIndex,
        const std::vector<ContinuedRowSlice>& pageRows) const;

    std::shared_ptr<ILogicalPageResolver> _logicalPageResolver;
};

}

#endif
