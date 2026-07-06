#ifndef SDF_PARSING_ROW_DECODER_HPP
#define SDF_PARSING_ROW_DECODER_HPP

#include <memory>
#include <vector>

#include "sdf/domain/ColumnValue.hpp"
#include "sdf/parsing/ILobChainRegistry.hpp"
#include "sdf/parsing/IRowDecoder.hpp"

namespace sdf::parsing
{

class RowDecoder final : public IRowDecoder
{
public:
    explicit RowDecoder(std::shared_ptr<ILobChainRegistry> lobChainRegistry);

    domain::Row Decode(const domain::TableDef& table, std::span<const std::uint8_t> rowBytes) override;

private:
    std::shared_ptr<ILobChainRegistry> _lobChainRegistry;

    static bool IsNull(std::span<const std::uint8_t> presence, std::size_t ordinalZeroBased);
    domain::ColumnValue DecodeFixed(const domain::ColumnDef& column, std::span<const std::uint8_t> chunk) const;
    domain::ColumnValue DecodeVarLength(
        const domain::ColumnDef& column, std::span<const std::uint8_t> chunk, bool compressed) const;
    domain::ColumnValue DecodeLob(
        const domain::ColumnDef& column, std::span<const std::uint8_t> chunk, bool compressed) const;
};

}

#endif
