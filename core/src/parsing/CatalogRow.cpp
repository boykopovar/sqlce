#include "sdf/parsing/CatalogRow.hpp"

#include "sdf/parsing/CatalogSchema.hpp"

namespace sdf::parsing
{

CatalogRowKind CatalogRowKindFromSysObjectType(std::uint16_t sysObjectType)
{
    switch (sysObjectType)
    {
        case CatalogTableIdTable:
            return CatalogRowKind::Table;
        case CatalogTableIdIndex:
            return CatalogRowKind::Index;
        case CatalogTableIdColumn:
            return CatalogRowKind::Column;
        case CatalogTableIdConstraint:
            return CatalogRowKind::Constraint;
        default:
            return CatalogRowKind::Unknown;
    }
}

}
