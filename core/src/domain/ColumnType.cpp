#include "sdf/domain/ColumnType.hpp"

namespace sdf::domain
{

bool IsBitColumnType(ColumnType type)
{
    return type == ColumnType::Bit;
}

bool IsVarLengthColumnType(ColumnType type)
{
    switch (type)
    {
        case ColumnType::NVarChar:
        case ColumnType::VarBinary:
        case ColumnType::NText:
        case ColumnType::Image:
            return true;
        default:
            return false;
    }
}

bool IsLobColumnType(ColumnType type)
{
    return type == ColumnType::NText || type == ColumnType::Image;
}

std::size_t FixedSizeOf(ColumnType type, std::uint16_t declaredSize)
{
    if (IsBitColumnType(type) || IsVarLengthColumnType(type))
    {
        return 0;
    }

    switch (type)
    {
        case ColumnType::TinyInt:
            return 1;
        case ColumnType::SmallInt:
            return 2;
        case ColumnType::Int:
            return 4;
        case ColumnType::BigInt:
            return 8;
        case ColumnType::Binary:
            return declaredSize != 0 ? declaredSize : 1;
        case ColumnType::DateTime:
            return 8;
        case ColumnType::UniqueIdentifier:
            return 16;
        case ColumnType::Float:
            return 8;
        case ColumnType::Money:
            return 8;
        case ColumnType::Numeric:
            return 19;
        default:
            return declaredSize != 0 ? declaredSize : 4;
    }
}

std::string_view NameOf(ColumnType type)
{
    switch (type)
    {
        case ColumnType::TinyInt:
            return "tinyint";
        case ColumnType::SmallInt:
            return "smallint";
        case ColumnType::Int:
            return "int";
        case ColumnType::BigInt:
            return "bigint";
        case ColumnType::NVarChar:
            return "nvarchar/nchar";
        case ColumnType::NText:
            return "ntext";
        case ColumnType::Binary:
            return "binary";
        case ColumnType::VarBinary:
            return "varbinary";
        case ColumnType::Image:
            return "image";
        case ColumnType::DateTime:
            return "datetime";
        case ColumnType::UniqueIdentifier:
            return "uniqueidentifier";
        case ColumnType::Bit:
            return "bit";
        case ColumnType::Float:
            return "float/real";
        case ColumnType::Money:
            return "money";
        case ColumnType::Numeric:
            return "numeric/decimal";
        default:
            return "unknown";
    }
}

}
