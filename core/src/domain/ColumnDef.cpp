#include "sdf/domain/ColumnDef.hpp"

namespace sdf::domain
{

ColumnDef::ColumnDef(
    std::string name,
    std::uint16_t ordinal,
    std::uint8_t dbType,
    ColumnType type,
    std::uint16_t declaredSize,
    std::uint8_t precision,
    std::uint8_t scale)
    : _name(std::move(name))
    , _ordinal(ordinal)
    , _dbType(dbType)
    , _type(type)
    , _declaredSize(declaredSize)
    , _precision(precision)
    , _scale(scale)
{
}

const std::string& ColumnDef::Name() const
{
    return _name;
}

std::uint16_t ColumnDef::Ordinal() const
{
    return _ordinal;
}

std::uint8_t ColumnDef::DbType() const
{
    return _dbType;
}

ColumnType ColumnDef::Type() const
{
    return _type;
}

std::uint16_t ColumnDef::DeclaredSize() const
{
    if (IsBit())
    {
        return 1;
    }
    return _declaredSize;
}

std::uint8_t ColumnDef::Precision() const
{
    return _precision;
}

std::uint8_t ColumnDef::Scale() const
{
    return _scale;
}

bool ColumnDef::IsBit() const
{
    return IsBitColumnType(_type);
}

bool ColumnDef::IsVarLength() const
{
    return IsVarLengthColumnType(_type);
}

bool ColumnDef::IsLob() const
{
    return IsLobColumnType(_type);
}

std::size_t ColumnDef::FixedSize() const
{
    return FixedSizeOf(_type, _declaredSize);
}

}
