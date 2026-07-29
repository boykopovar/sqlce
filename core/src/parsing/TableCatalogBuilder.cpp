#include "sdf/parsing/TableCatalogBuilder.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/PageView.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

domain::ColumnType ToColumnType(std::uint16_t intType)
{
    switch (intType)
    {
        case 0:
            return domain::ColumnType::TinyInt;
        case 1:
            return domain::ColumnType::SmallInt;
        case 3:
            return domain::ColumnType::Int;
        case 5:
            return domain::ColumnType::BigInt;
        case 8:
            return domain::ColumnType::NVarChar;
        case 9:
            return domain::ColumnType::NText;
        case 10:
            return domain::ColumnType::Binary;
        case 11:
            return domain::ColumnType::VarBinary;
        case 12:
            return domain::ColumnType::Image;
        case 13:
            return domain::ColumnType::DateTime;
        case 14:
            return domain::ColumnType::UniqueIdentifier;
        case 15:
            return domain::ColumnType::Bit;
        case 17:
            return domain::ColumnType::Float;
        case 18:
            return domain::ColumnType::Money;
        case 19:
            return domain::ColumnType::Numeric;
        default:
            return domain::ColumnType::Unknown;
    }
}

struct PendingColumn
{
    std::string ownerTableName;
    domain::ColumnDef columnDef;
};

struct TableObjectKey
{
    std::uint8_t objectId;
    std::uint8_t objectGeneration;
};

TableObjectKey ResolveTableObjectKey(
    const domain::IPageStorage& storage, const ILogicalPageResolver& logicalPageResolver,
    std::uint32_t tablePageId)
{
    const std::optional<std::size_t> rootPhysicalPage = logicalPageResolver.ResolvePhysicalPage(storage, tablePageId);
    if (!rootPhysicalPage.has_value())
    {
        return TableObjectKey{0, 0};
    }

    const PageView rootPage(storage.PageBytes(*rootPhysicalPage));
    if (rootPage.PageType() != TableRootPageType)
    {
        return TableObjectKey{rootPage.OwnerObjectId(), rootPage.OwnerObjectGeneration()};
    }

    const std::uint32_t ownerPageId
        = infrastructure::ReadUInt32LE(rootPage.Bytes(), TableRootOwnerPageIdOffset) & TableRootOwnerPageIdMask;
    if (ownerPageId == 0)
    {
        return TableObjectKey{0, 0};
    }

    const std::optional<std::size_t> ownerPhysicalPage
        = logicalPageResolver.ResolvePhysicalPage(storage, ownerPageId);
    if (!ownerPhysicalPage.has_value())
    {
        return TableObjectKey{0, 0};
    }

    const PageView ownerPage(storage.PageBytes(*ownerPhysicalPage));
    return TableObjectKey{ownerPage.OwnerObjectId(), ownerPage.OwnerObjectGeneration()};
}

}

TableCatalogBuilder::TableCatalogBuilder(
    std::shared_ptr<ICatalogPageScanner> pageScanner, std::shared_ptr<ICatalogRowDecoder> rowDecoder,
    std::shared_ptr<ILogicalPageResolver> logicalPageResolver)
    : _pageScanner(std::move(pageScanner)), _rowDecoder(std::move(rowDecoder)),
      _logicalPageResolver(std::move(logicalPageResolver))
{
}

std::map<std::string, domain::TableDef> TableCatalogBuilder::BuildTables(const domain::IPageStorage& storage) const
{
    const std::vector<std::vector<std::uint8_t>> catalogRows = _pageScanner->CollectCatalogRows(storage);

    std::map<std::string, domain::TableDef> tablesByName;
    std::vector<PendingColumn> pendingColumns;

    for (const std::vector<std::uint8_t>& rowBytes : catalogRows)
    {
        const std::optional<CatalogRow> decoded = _rowDecoder->Decode(rowBytes);
        if (!decoded.has_value())
        {
            continue;
        }

        const CatalogRow& row = *decoded;

        if (row.kind == CatalogRowKind::Table)
        {
            if (!row.objectName.has_value() || !row.tablePageId.has_value())
            {
                continue;
            }
            const TableObjectKey objectKey = ResolveTableObjectKey(storage, *_logicalPageResolver, row.tablePageId.value());
            tablesByName.emplace(
                *row.objectName,
                domain::TableDef(*row.objectName, objectKey.objectId, objectKey.objectGeneration, row.tablePageId.value())
            );
        }
        else if (row.kind == CatalogRowKind::Column)
        {
            if (!row.objectOwner.has_value() || !row.objectName.has_value() || !row.sysObjectOrdinal.has_value()
                || !row.columnType.has_value() || !row.columnSize.has_value())
            {
                continue;
            }

            const std::uint8_t dbType = row.cedbInfoDbType.has_value()
                ? static_cast<std::uint8_t>(row.cedbInfoDbType.value() & 0xFFu)
                : 0;

            domain::ColumnDef columnDef(
                *row.objectName,
                row.sysObjectOrdinal.value(),
                dbType,
                ToColumnType(row.columnType.value()),
                row.columnSize.value(),
                row.columnPrecision.value_or(0),
                row.columnScale.value_or(0));

            pendingColumns.push_back(PendingColumn{*row.objectOwner, std::move(columnDef)});
        }
    }

    for (PendingColumn& pending : pendingColumns)
    {
        const auto it = tablesByName.find(pending.ownerTableName);
        if (it != tablesByName.end())
        {
            it->second.Columns().push_back(std::move(pending.columnDef));
        }
    }

    for (auto& [name, table] : tablesByName)
    {
        std::sort(
            table.Columns().begin(),
            table.Columns().end(),
            [](const domain::ColumnDef& a, const domain::ColumnDef& b) { return a.Ordinal() < b.Ordinal(); });
    }

    return tablesByName;
}

}
