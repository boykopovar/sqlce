#ifndef SDF_PARSING_CATALOG_ROW_HPP
#define SDF_PARSING_CATALOG_ROW_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace sdf::parsing
{

enum class CatalogRowKind
{
    Table,
    Column,
    Index,
    Constraint,
    Unknown
};

struct CatalogRow
{
    CatalogRowKind kind = CatalogRowKind::Unknown;

    std::optional<std::string> objectOwner;
    std::optional<std::string> objectName;
    std::optional<std::uint16_t> cedbInfoDbType;
    std::optional<std::uint16_t> cedbInfoOrdinal;

    std::optional<std::uint32_t> tablePageId;

    std::optional<std::uint16_t> columnType;
    std::optional<std::uint16_t> columnSize;
    std::optional<std::uint8_t> columnPrecision;
    std::optional<std::uint8_t> columnScale;
};

CatalogRowKind CatalogRowKindFromSysObjectType(std::uint16_t sysObjectType);

}

#endif
