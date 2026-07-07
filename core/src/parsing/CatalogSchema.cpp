#include "sdf/parsing/CatalogSchema.hpp"

namespace sdf::parsing
{

bool IsVarLikeKind(CatalogFieldKind kind)
{
    return kind == CatalogFieldKind::VarWideChar || kind == CatalogFieldKind::VarBinaryLike
        || kind == CatalogFieldKind::VarBinaryLikeAlt;
}

bool IsBitKind(CatalogFieldKind kind)
{
    return kind == CatalogFieldKind::Bit;
}

std::size_t FixedKindSize(CatalogFieldKind kind)
{
    switch (kind)
    {
        case CatalogFieldKind::TinyInt:
            return 1;
        case CatalogFieldKind::SmallInt:
            return 2;
        case CatalogFieldKind::Int:
        case CatalogFieldKind::IntAlt:
            return 4;
        case CatalogFieldKind::BigInt:
            return 8;
        default:
            return 0;
    }
}

}
