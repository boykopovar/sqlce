#ifndef SDF_DOMAIN_COLUMN_TYPE_HPP
#define SDF_DOMAIN_COLUMN_TYPE_HPP

#include <cstdint>
#include <string_view>

namespace sdf::domain
{

enum class ColumnType : std::uint16_t
{
    TinyInt = 0,
    SmallInt = 1,
    Int = 3,
    BigInt = 5,
    NVarChar = 8,
    NText = 9,
    Binary = 10,
    VarBinary = 11,
    Image = 12,
    DateTime = 13,
    UniqueIdentifier = 14,
    Bit = 15,
    Float = 17,
    Money = 18,
    Numeric = 19,
    Unknown = 0xFFFF
};

bool IsBitColumnType(ColumnType type);

bool IsVarLengthColumnType(ColumnType type);

bool IsLobColumnType(ColumnType type);

std::size_t FixedSizeOf(ColumnType type, std::uint16_t declaredSize);

std::string_view NameOf(ColumnType type);

}

#endif
