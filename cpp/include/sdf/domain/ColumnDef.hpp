#ifndef SDF_DOMAIN_COLUMN_DEF_HPP
#define SDF_DOMAIN_COLUMN_DEF_HPP

#include <cstdint>
#include <string>

#include "sdf/domain/ColumnType.hpp"

namespace sdf::domain
{

class ColumnDef
{
public:
    ColumnDef(
        std::string name,
        std::uint16_t ordinal,
        std::uint8_t dbType,
        ColumnType type,
        std::uint16_t declaredSize,
        std::uint8_t precision,
        std::uint8_t scale);

    const std::string& Name() const;
    std::uint16_t Ordinal() const;
    std::uint8_t DbType() const;
    ColumnType Type() const;
    std::uint16_t DeclaredSize() const;
    std::uint8_t Precision() const;
    std::uint8_t Scale() const;

    bool IsBit() const;
    bool IsVarLength() const;
    bool IsLob() const;
    std::size_t FixedSize() const;

private:
    std::string _name;
    std::uint16_t _ordinal;
    std::uint8_t _dbType;
    ColumnType _type;
    std::uint16_t _declaredSize;
    std::uint8_t _precision;
    std::uint8_t _scale;
};

}

#endif
