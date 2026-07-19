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

    [[nodiscard]] const std::string& Name() const;
    [[nodiscard]] std::uint16_t Ordinal() const;
    [[nodiscard]] std::uint8_t DbType() const;
    [[nodiscard]] ColumnType Type() const;
    [[nodiscard]] std::uint16_t DeclaredSize() const;
    [[nodiscard]] std::uint8_t Precision() const;
    [[nodiscard]] std::uint8_t Scale() const;

    [[nodiscard]] bool IsBit() const;
    [[nodiscard]] bool IsNumeric() const;
    [[nodiscard]] bool IsVarLength() const;
    [[nodiscard]] bool IsLob() const;
    [[nodiscard]] std::size_t FixedSize() const;

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
