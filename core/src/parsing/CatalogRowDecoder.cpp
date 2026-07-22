#include "sdf/parsing/CatalogRowDecoder.hpp"

#include <array>
#include <vector>

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/CatalogSchema.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::size_t FixedRegionEndOffset()
{
    std::size_t total = CatalogRowHeaderLength + CatalogPresenceBytes + CatalogBitRegionBytes;
    for (const CatalogFieldDescriptor& field : CatalogSchema)
    {
        if (!IsVarLikeKind(field.kind) && !IsBitKind(field.kind))
        {
            total += FixedKindSize(field.kind);
        }
    }
    return total;
}

std::size_t VarColumnCountInSchema()
{
    std::size_t count = 0;
    for (const CatalogFieldDescriptor& field : CatalogSchema)
    {
        if (IsVarLikeKind(field.kind))
        {
            ++count;
        }
    }
    return count;
}

void AssignField(CatalogRow& row, std::string_view name, std::uint64_t rawValue)
{
    if (name == "SysColumnType")
    {
        row.columnType = static_cast<std::uint16_t>(rawValue);
    }
    else if (name == "SysColumnSize")
    {
        row.columnSize = static_cast<std::uint16_t>(rawValue);
    }
    else if (name == "SysColumnPrecision")
    {
        row.columnPrecision = static_cast<std::uint8_t>(rawValue);
    }
    else if (name == "SysColumnScale")
    {
        row.columnScale = static_cast<std::uint8_t>(rawValue);
    }
    else if (name == "SysTablePageId")
    {
        row.tablePageId = static_cast<std::uint32_t>(rawValue);
    }
    else if (name == "SysObjectOrdinal")
    {
        row.sysObjectOrdinal = static_cast<std::uint16_t>(rawValue);
    }
}

std::string DecodeNarrowFromWide(std::span<const std::uint8_t> chunk)
{
    std::string result;
    result.reserve(chunk.size());
    for (const std::uint8_t byte : chunk)
    {
        if (byte == 0)
        {
            break;
        }
        result.push_back(static_cast<char>(byte));
    }
    return result;
}

}

bool CatalogRowDecoder::IsFieldNull(std::span<const std::uint8_t> presence, std::uint8_t columnId)
{
    const std::size_t byteIndex = columnId / 8;
    const std::size_t bitIndex = columnId % 8;
    if (byteIndex >= presence.size())
    {
        return true;
    }
    return ((presence[byteIndex] >> bitIndex) & SingleBitMask) != 0;
}

std::optional<CatalogRow> CatalogRowDecoder::Decode(std::span<const std::uint8_t> rowBytes) const
{
    if (rowBytes.size() < CatalogRowHeaderLength + CatalogPresenceBytes)
    {
        return std::nullopt;
    }

    const std::span<const std::uint8_t> presence
        = rowBytes.subspan(CatalogRowHeaderLength, CatalogPresenceBytes);
    const bool sysObjectTypeIsNull = IsFieldNull(presence, 0);
    if (sysObjectTypeIsNull)
    {
        return std::nullopt;
    }

    std::size_t offset = CatalogRowHeaderLength + CatalogPresenceBytes;
    if (offset + CatalogBitRegionBytes > rowBytes.size())
    {
        return std::nullopt;
    }

    const std::uint16_t bitRegionValue = infrastructure::ReadUInt16LE(rowBytes, offset);
    offset += CatalogBitRegionBytes;

    (void)bitRegionValue;

    if (offset + CedbInfoOrdinalOffset > rowBytes.size())
    {
        return std::nullopt;
    }
    const std::uint16_t sysObjectType = infrastructure::ReadUInt16LE(rowBytes, offset);

    const std::uint8_t applicableTableId = [&]() -> std::uint8_t {
        switch (sysObjectType)
        {
            case CatalogTableIdTable:
            case CatalogTableIdIndex:
            case CatalogTableIdColumn:
            case CatalogTableIdConstraint:
                return static_cast<std::uint8_t>(sysObjectType);
            default:
                return 0;
        }
    }();

    CatalogRow row;
    row.kind = CatalogRowKindFromSysObjectType(sysObjectType);

    for (const CatalogFieldDescriptor& field : CatalogSchema)
    {
        if (IsVarLikeKind(field.kind) || IsBitKind(field.kind))
        {
            continue;
        }

        const std::size_t fieldSize = FixedKindSize(field.kind);
        const bool applicable = field.tableId == CatalogTableIdCommon || field.tableId == applicableTableId;

        if (!applicable)
        {
            offset += fieldSize;
            continue;
        }

        if (offset + fieldSize > rowBytes.size())
        {
            return std::nullopt;
        }

        if (IsFieldNull(presence, field.columnId))
        {
            offset += fieldSize;
            continue;
        }

        if (field.name == "SysObjectCedbInfo")
        {
            row.cedbInfoDbType = infrastructure::ReadUInt16LE(rowBytes, offset);
            row.cedbInfoOrdinal = infrastructure::ReadUInt16LE(rowBytes, offset + CedbInfoOrdinalOffset);
        }
        else
        {
            std::uint64_t rawValue = 0;
            switch (fieldSize)
            {
                case 1:
                    rawValue = rowBytes[offset];
                    break;
                case 2:
                    rawValue = infrastructure::ReadUInt16LE(rowBytes, offset);
                    break;
                case 4:
                    rawValue = infrastructure::ReadUInt32LE(rowBytes, offset);
                    break;
                case 8:
                    rawValue = infrastructure::ReadUInt64LE(rowBytes, offset);
                    break;
                default:
                    break;
            }
            AssignField(row, field.name, rawValue);
        }

        offset += fieldSize;
    }

    const std::size_t fixedRegionEnd = FixedRegionEndOffset();
    const std::size_t varColumnCount = VarColumnCountInSchema();
    const std::size_t directoryLength = VarDirectoryEntryLength * varColumnCount;

    if (fixedRegionEnd + directoryLength > rowBytes.size())
    {
        return std::nullopt;
    }

    const std::span<const std::uint8_t> directoryBytes = rowBytes.subspan(fixedRegionEnd, directoryLength);
    std::vector<std::uint16_t> entries(varColumnCount);
    for (std::size_t i = 0; i < varColumnCount; ++i)
    {
        entries[i] = infrastructure::ReadUInt16LE(directoryBytes, VarDirectoryEntryLength * i);
    }

    const std::size_t varDataStart = fixedRegionEnd + directoryLength;
    const std::span<const std::uint8_t> varData = rowBytes.subspan(varDataStart);

    std::size_t varColumnIndex = 0;
    std::size_t prevEnd = 0;
    const std::size_t lastVarIndex = varColumnCount - 1;

    for (const CatalogFieldDescriptor& field : CatalogSchema)
    {
        if (!IsVarLikeKind(field.kind))
        {
            continue;
        }

        const bool isLast = varColumnIndex == lastVarIndex;
        std::size_t cumulativeEnd;
        if (isLast)
        {
            cumulativeEnd = varData.size();
        }
        else
        {
            cumulativeEnd = entries[1 + varColumnIndex] & VarDirectoryOffsetMask;
        }

        const bool applicable = field.tableId == CatalogTableIdCommon || field.tableId == applicableTableId;
        const bool present = applicable && !IsFieldNull(presence, field.columnId);

        if (!present)
        {
            prevEnd = cumulativeEnd;
            ++varColumnIndex;
            continue;
        }

        if (prevEnd > cumulativeEnd || cumulativeEnd > varData.size())
        {
            prevEnd = cumulativeEnd;
            ++varColumnIndex;
            continue;
        }

        const std::span<const std::uint8_t> chunk = varData.subspan(prevEnd, cumulativeEnd - prevEnd);
        prevEnd = cumulativeEnd;

        if (field.name == "SysObjectOwner")
        {
            row.objectOwner = DecodeNarrowFromWide(chunk);
        }
        else if (field.name == "SysObjectName")
        {
            row.objectName = DecodeNarrowFromWide(chunk);
        }

        ++varColumnIndex;
    }

    return row;
}

}
