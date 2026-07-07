#ifndef SDF_PARSING_CATALOG_SCHEMA_HPP
#define SDF_PARSING_CATALOG_SCHEMA_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace sdf::parsing
{

enum class CatalogFieldKind : std::uint8_t
{
    TinyInt = 0x0,
    SmallInt = 0x2,
    Int = 0x3,
    IntAlt = 0x4,
    BigInt = 0xA,
    VarWideChar = 0x8,
    VarBinaryLike = 0xB,
    VarBinaryLikeAlt = 0xC,
    Bit = 0xF
};

constexpr std::uint8_t CatalogTableIdCommon = 255;
constexpr std::uint8_t CatalogTableIdTable = 1;
constexpr std::uint8_t CatalogTableIdIndex = 2;
constexpr std::uint8_t CatalogTableIdColumn = 4;
constexpr std::uint8_t CatalogTableIdConstraint = 8;

struct CatalogFieldDescriptor
{
    std::string_view name;
    CatalogFieldKind kind;
    std::uint8_t columnId;
    std::uint8_t tableId;
};

constexpr std::array<CatalogFieldDescriptor, 38> CatalogSchema{{
    {"SysObjectType", CatalogFieldKind::SmallInt, 0, CatalogTableIdCommon},
    {"SysObjectOwner", CatalogFieldKind::VarWideChar, 1, CatalogTableIdCommon},
    {"SysObjectName", CatalogFieldKind::VarWideChar, 2, CatalogTableIdCommon},
    {"SysObjectIsSystem", CatalogFieldKind::Bit, 3, CatalogTableIdCommon},
    {"SysObjectVersion", CatalogFieldKind::BigInt, 4, CatalogTableIdCommon},
    {"SysObjectOrdinal", CatalogFieldKind::SmallInt, 5, CatalogTableIdCommon},
    {"SysObjectCedbInfo", CatalogFieldKind::IntAlt, 6, CatalogTableIdCommon},

    {"SysTablePageId", CatalogFieldKind::IntAlt, 7, CatalogTableIdTable},
    {"SysTableNick", CatalogFieldKind::Int, 8, CatalogTableIdTable},
    {"SysTableTrackingType", CatalogFieldKind::SmallInt, 9, CatalogTableIdTable},
    {"SysTableDdlGranted", CatalogFieldKind::IntAlt, 10, CatalogTableIdTable},
    {"SysTableReadOnly", CatalogFieldKind::Bit, 11, CatalogTableIdTable},
    {"SysTableCompressed", CatalogFieldKind::Bit, 12, CatalogTableIdTable},

    {"SysColumnType", CatalogFieldKind::SmallInt, 13, CatalogTableIdColumn},
    {"SysColumnSize", CatalogFieldKind::SmallInt, 14, CatalogTableIdColumn},
    {"SysColumnPrecision", CatalogFieldKind::TinyInt, 15, CatalogTableIdColumn},
    {"SysColumnScale", CatalogFieldKind::TinyInt, 16, CatalogTableIdColumn},
    {"SysColumnFixed", CatalogFieldKind::Bit, 17, CatalogTableIdColumn},
    {"SysColumnNullable", CatalogFieldKind::Bit, 18, CatalogTableIdColumn},
    {"SysColumnWriteable", CatalogFieldKind::Bit, 19, CatalogTableIdColumn},
    {"SysColumnAutoType", CatalogFieldKind::SmallInt, 20, CatalogTableIdColumn},
    {"SysColumnPosition", CatalogFieldKind::SmallInt, 21, CatalogTableIdColumn},
    {"SysColumnDefault", CatalogFieldKind::VarWideChar, 22, CatalogTableIdColumn},
    {"SysColumnCompressed", CatalogFieldKind::Bit, 23, CatalogTableIdColumn},

    {"SysIndexRoot", CatalogFieldKind::IntAlt, 24, CatalogTableIdIndex},
    {"SysIndexKey", CatalogFieldKind::VarBinaryLike, 25, CatalogTableIdIndex},
    {"SysIndexUnique", CatalogFieldKind::Bit, 26, CatalogTableIdIndex},
    {"SysIndexNullOption", CatalogFieldKind::SmallInt, 27, CatalogTableIdIndex},
    {"SysIndexPositional", CatalogFieldKind::Bit, 28, CatalogTableIdIndex},
    {"SysIndexHistogram", CatalogFieldKind::VarBinaryLikeAlt, 29, CatalogTableIdIndex},

    {"SysConstraintType", CatalogFieldKind::IntAlt, 30, CatalogTableIdConstraint},
    {"SysConstraintIndex", CatalogFieldKind::VarWideChar, 31, CatalogTableIdConstraint},
    {"SysConstraintTargetIndex", CatalogFieldKind::VarWideChar, 32, CatalogTableIdConstraint},
    {"SysConstraintTargetTable", CatalogFieldKind::VarWideChar, 33, CatalogTableIdConstraint},
    {"SysConstraintOnDelete", CatalogFieldKind::IntAlt, 34, CatalogTableIdConstraint},
    {"SysConstraintOnUpdate", CatalogFieldKind::IntAlt, 35, CatalogTableIdConstraint},
    {"SysConstraintKey", CatalogFieldKind::VarBinaryLike, 36, CatalogTableIdConstraint},
    {"SysConstraintColumn", CatalogFieldKind::VarWideChar, 37, CatalogTableIdConstraint},
}};

constexpr std::size_t CatalogTotalColumnCount = CatalogSchema.size();
constexpr std::size_t CatalogPresenceBytes = (CatalogTotalColumnCount + 7) / 8;

bool IsVarLikeKind(CatalogFieldKind kind);
bool IsBitKind(CatalogFieldKind kind);
std::size_t FixedKindSize(CatalogFieldKind kind);

}

#endif
