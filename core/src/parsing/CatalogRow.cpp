#include "sdf/parsing/CatalogRow.hpp"

namespace sdf::parsing
{

CatalogRowKind CatalogRowKindFromSysObjectType(std::uint16_t sysObjectType)
{
    switch (sysObjectType)
    {
        case 1:
            return CatalogRowKind::Table;
        case 2:
            return CatalogRowKind::Index;
        case 4:
            return CatalogRowKind::Column;
        case 8:
            return CatalogRowKind::Constraint;
        default:
            return CatalogRowKind::Unknown;
    }
}

}
