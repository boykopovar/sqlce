#include "sdf/parsing/RowDecoder.hpp"

#include <utility>
#include <vector>

#include "sdf/domain/LazyLob.hpp"
#include "sdf/domain/PageLayout.hpp"
#include "sdf/domain/TextDecoder.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"

namespace sdf::parsing
{

namespace
{

std::vector<const domain::ColumnDef*> FixedColumnsOf(const domain::TableDef& table)
{
    std::vector<const domain::ColumnDef*> result;
    for (const domain::ColumnDef& column : table.Columns())
    {
        if (!column.IsBit() && !column.IsVarLength())
        {
            result.push_back(&column);
        }
    }
    return result;
}

std::vector<const domain::ColumnDef*> BitColumnsOf(const domain::TableDef& table)
{
    std::vector<const domain::ColumnDef*> result;
    for (const domain::ColumnDef& column : table.Columns())
    {
        if (column.IsBit())
        {
            result.push_back(&column);
        }
    }
    return result;
}

std::vector<const domain::ColumnDef*> VarColumnsOf(const domain::TableDef& table)
{
    std::vector<const domain::ColumnDef*> result;
    for (const domain::ColumnDef& column : table.Columns())
    {
        if (column.IsVarLength())
        {
            result.push_back(&column);
        }
    }
    return result;
}

}

RowDecoder::RowDecoder(std::shared_ptr<ILobChainRegistry> lobChainRegistry)
    : _lobChainRegistry(std::move(lobChainRegistry))
{
}

bool RowDecoder::IsNull(std::span<const std::uint8_t> presence, std::size_t ordinalZeroBased)
{
    const std::size_t byteIndex = ordinalZeroBased / 8;
    const std::size_t bitIndex = ordinalZeroBased % 8;
    if (byteIndex >= presence.size())
    {
        return false;
    }
    return ((presence[byteIndex] >> bitIndex) & domain::SingleBitMask) != 0;
}

domain::Row RowDecoder::Decode(const domain::TableDef& table, std::span<const std::uint8_t> rowBytes)
{
    const std::vector<domain::ColumnDef>& columns = table.Columns();
    const std::size_t columnCount = columns.size();

    const std::size_t presenceBytes = (columnCount + 7) / 8;
    const std::vector<const domain::ColumnDef*> bitColumns = BitColumnsOf(table);
    const std::size_t bitRegionBytes = (bitColumns.size() + 7) / 8;

    std::size_t offset = domain::RowInternalHeaderLength;
    const std::span<const std::uint8_t> presence
        = offset + presenceBytes <= rowBytes.size() ? rowBytes.subspan(offset, presenceBytes)
                                                      : std::span<const std::uint8_t>{};
    offset += presenceBytes;

    const std::span<const std::uint8_t> bitRegion
        = offset + bitRegionBytes <= rowBytes.size() ? rowBytes.subspan(offset, bitRegionBytes)
                                                       : std::span<const std::uint8_t>{};
    offset += bitRegionBytes;

    domain::Row row;
    std::vector<domain::ColumnValue> valuesByOrdinalIndex(columnCount);

    const std::vector<const domain::ColumnDef*> fixedColumns = FixedColumnsOf(table);
    for (const domain::ColumnDef* column : fixedColumns)
    {
        const std::size_t size = column->FixedSize();
        const bool isNull = IsNull(presence, column->Ordinal() - 1);

        domain::ColumnValue value;
        if (!isNull && offset + size <= rowBytes.size())
        {
            value = DecodeFixed(*column, rowBytes.subspan(offset, size));
        }
        offset += size;

        for (std::size_t i = 0; i < columnCount; ++i)
        {
            if (columns[i].Ordinal() == column->Ordinal())
            {
                valuesByOrdinalIndex[i] = value;
                break;
            }
        }
    }

    for (std::size_t bitIndex = 0; bitIndex < bitColumns.size(); ++bitIndex)
    {
        const domain::ColumnDef* column = bitColumns[bitIndex];
        const bool isNull = IsNull(presence, column->Ordinal() - 1);

        domain::ColumnValue value;
        if (!isNull)
        {
            const std::size_t byteIndex = bitIndex / 8;
            const std::size_t bitInByte = bitIndex % 8;
            if (byteIndex < bitRegion.size())
            {
                const bool bitValue = ((bitRegion[byteIndex] >> bitInByte) & domain::SingleBitMask) != 0;
                value = domain::ColumnValue(domain::ColumnValueStorage(bitValue));
            }
        }

        for (std::size_t i = 0; i < columnCount; ++i)
        {
            if (columns[i].Ordinal() == column->Ordinal())
            {
                valuesByOrdinalIndex[i] = value;
                break;
            }
        }
    }

    const std::vector<const domain::ColumnDef*> varColumns = VarColumnsOf(table);
    if (!varColumns.empty())
    {
        bool allNull = true;
        for (const domain::ColumnDef* column : varColumns)
        {
            if (!IsNull(presence, column->Ordinal() - 1))
            {
                allNull = false;
                break;
            }
        }

        if (!allNull)
        {
            const std::size_t directoryLength = domain::VarDirectoryEntryLength * varColumns.size();
            if (offset + directoryLength <= rowBytes.size())
            {
                const std::span<const std::uint8_t> directoryBytes = rowBytes.subspan(offset, directoryLength);
                offset += directoryLength;

                const std::span<const std::uint8_t> varData = rowBytes.subspan(offset);

                std::vector<std::size_t> starts(varColumns.size());
                std::vector<bool> compressedFlags(varColumns.size());
                for (std::size_t i = 0; i < varColumns.size(); ++i)
                {
                    const std::uint16_t entry
                        = infrastructure::ReadUInt16LE(directoryBytes, domain::VarDirectoryEntryLength * i);
                    starts[i] = entry & domain::VarDirectoryOffsetMask;
                    compressedFlags[i] = (entry & domain::VarDirectoryCompressedFlag) != 0;
                }

                for (std::size_t i = 0; i < varColumns.size(); ++i)
                {
                    const domain::ColumnDef* column = varColumns[i];
                    const bool isNull = IsNull(presence, column->Ordinal() - 1);

                    domain::ColumnValue value;
                    if (!isNull)
                    {
                        const std::size_t start = starts[i];
                        const std::size_t end = (i + 1 < starts.size()) ? starts[i + 1] : varData.size();
                        if (start <= end && end <= varData.size())
                        {
                            const std::span<const std::uint8_t> chunk = varData.subspan(start, end - start);
                            if (column->IsLob())
                            {
                                value = DecodeLob(*column, chunk, compressedFlags[i]);
                            }
                            else
                            {
                                value = DecodeVarLength(*column, chunk, compressedFlags[i]);
                            }
                        }
                    }

                    for (std::size_t j = 0; j < columnCount; ++j)
                    {
                        if (columns[j].Ordinal() == column->Ordinal())
                        {
                            valuesByOrdinalIndex[j] = value;
                            break;
                        }
                    }
                }
            }
        }
    }

    for (std::size_t i = 0; i < columnCount; ++i)
    {
        row.Append(columns[i].Name(), valuesByOrdinalIndex[i]);
    }

    return row;
}

domain::ColumnValue RowDecoder::DecodeFixed(const domain::ColumnDef& column, std::span<const std::uint8_t> chunk)
    const
{
    switch (column.Type())
    {
        case domain::ColumnType::Int:
            return domain::ColumnValue(
                domain::ColumnValueStorage(static_cast<std::int64_t>(infrastructure::ReadInt32LE(chunk, 0))));

        case domain::ColumnType::DateTime:
        {
            const std::uint32_t ticks = infrastructure::ReadUInt32LE(chunk, 0);
            const std::int32_t days = infrastructure::ReadInt32LE(chunk, 4);
            return domain::ColumnValue(domain::ColumnValueStorage(domain::DateTimeValue(days, ticks)));
        }

        case domain::ColumnType::Money:
        {
            const std::int64_t raw = infrastructure::ReadInt64LE(chunk, 0);
            return domain::ColumnValue(domain::ColumnValueStorage(static_cast<double>(raw) / 10000.0));
        }

        case domain::ColumnType::Float:
            return domain::ColumnValue(domain::ColumnValueStorage(infrastructure::ReadDoubleLE(chunk, 0)));

        case domain::ColumnType::UniqueIdentifier:
            return domain::ColumnValue(
                domain::ColumnValueStorage(domain::Guid(infrastructure::ReadBytes16(chunk, 0))));

        case domain::ColumnType::Numeric:
        {
            const std::uint8_t precision = chunk[0];
            const std::uint8_t scale = chunk[1];
            const std::uint8_t sign = chunk[2];
            const std::array<std::uint8_t, 16> unscaled = infrastructure::ReadBytes16(chunk, 3);
            return domain::ColumnValue(
                domain::ColumnValueStorage(domain::NumericValue(precision, scale, sign == 1, unscaled)));
        }

        case domain::ColumnType::TinyInt:
            return domain::ColumnValue(domain::ColumnValueStorage(static_cast<std::uint64_t>(chunk[0])));

        case domain::ColumnType::SmallInt:
        {
            const std::int16_t value = static_cast<std::int16_t>(infrastructure::ReadUInt16LE(chunk, 0));
            return domain::ColumnValue(domain::ColumnValueStorage(static_cast<std::int64_t>(value)));
        }

        case domain::ColumnType::BigInt:
            return domain::ColumnValue(domain::ColumnValueStorage(infrastructure::ReadInt64LE(chunk, 0)));

        case domain::ColumnType::Binary:
            return domain::ColumnValue(
                domain::ColumnValueStorage(std::vector<std::uint8_t>(chunk.begin(), chunk.end())));

        default:
            return domain::ColumnValue(
                domain::ColumnValueStorage(std::vector<std::uint8_t>(chunk.begin(), chunk.end())));
    }
}

domain::ColumnValue RowDecoder::DecodeVarLength(
    const domain::ColumnDef& column, std::span<const std::uint8_t> chunk, bool compressed) const
{
    if (column.Type() == domain::ColumnType::NVarChar)
    {
        return domain::ColumnValue(domain::ColumnValueStorage(domain::DecodeText(chunk, compressed)));
    }
    return domain::ColumnValue(domain::ColumnValueStorage(std::vector<std::uint8_t>(chunk.begin(), chunk.end())));
}

domain::ColumnValue RowDecoder::DecodeLob(
    const domain::ColumnDef& column, std::span<const std::uint8_t> chunk, bool compressed) const
{
    const bool isText = column.Type() == domain::ColumnType::NText;

    if (chunk.size() < domain::LobValueHeaderLength)
    {
        std::shared_ptr<domain::ILazyLobSource> emptySource = _lobChainRegistry->ResolveLob({}, 0);
        return domain::ColumnValue(domain::ColumnValueStorage(domain::LazyLob(emptySource, isText, compressed)));
    }

    const std::uint32_t length = infrastructure::ReadUInt32LE(chunk, 0);
    const std::span<const std::uint8_t> inlineTail = chunk.subspan(domain::LobValueHeaderLength);

    std::shared_ptr<domain::ILazyLobSource> source = _lobChainRegistry->ResolveLob(inlineTail, length);

    return domain::ColumnValue(domain::ColumnValueStorage(domain::LazyLob(source, isText, compressed)));
}

}