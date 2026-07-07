#include "sdf/application/SdfDatabase.hpp"

#include <stdexcept>

#include "sdf/infrastructure/FileStorage.hpp"
#include "sdf/infrastructure/PageView.hpp"
#include "sdf/parsing/CatalogPageScanner.hpp"
#include "sdf/parsing/CatalogRowDecoder.hpp"
#include "sdf/parsing/LobChainRegistry.hpp"
#include "sdf/parsing/RowDecoder.hpp"
#include "sdf/parsing/SdfPageCipher.hpp"
#include "sdf/parsing/TableCatalogBuilder.hpp"

namespace sdf::application
{

SdfDatabase::SdfDatabase(const std::string& path) : SdfDatabase(OpenStorage(path, std::string()))
{
}

SdfDatabase::SdfDatabase(const std::string& path, const std::string& password) : SdfDatabase(OpenStorage(path, password))
{
}

std::unique_ptr<domain::IPageStorage> SdfDatabase::OpenStorage(const std::string& path, const std::string& password)
{
    if (password.empty())
    {
        return std::make_unique<infrastructure::FileStorage>(path);
    }

    const std::vector<std::uint8_t> firstPage = infrastructure::ReadFirstPageRaw(path);
    const parsing::SdfPageCipher cipher(firstPage, password);
    return std::make_unique<infrastructure::FileStorage>(path, cipher);
}

SdfDatabase::SdfDatabase(std::unique_ptr<domain::IPageStorage> storage)
    : _storage(std::move(storage))
    , _pageScanner(std::make_shared<parsing::CatalogPageScanner>())
    , _tableCatalogBuilder(
          std::make_shared<parsing::TableCatalogBuilder>(_pageScanner, std::make_shared<parsing::CatalogRowDecoder>()))
    , _lobChainRegistry(std::make_shared<parsing::LobChainRegistry>(*_storage))
    , _rowDecoder(std::make_shared<parsing::RowDecoder>(_lobChainRegistry))
{
    _tables = _tableCatalogBuilder->BuildTables(*_storage);
    AssignDataPages();
}

void SdfDatabase::AssignDataPages()
{
    const std::set<std::uint8_t> catalogObjectIds = _pageScanner->FindCatalogObjectIds(*_storage);

    std::map<std::uint8_t, domain::TableDef*> tableByObjectId;
    for (auto& [name, table] : _tables)
    {
        tableByObjectId[table.ObjectId()] = &table;
    }

    const std::size_t pageCount = _storage->PageCount();
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const infrastructure::PageView page(_storage->PageBytes(pageNumber));
        if (!page.IsDataPage())
        {
            continue;
        }
        if (catalogObjectIds.find(page.OwnerObjectId()) != catalogObjectIds.end())
        {
            continue;
        }

        const auto it = tableByObjectId.find(page.OwnerObjectId());
        if (it != tableByObjectId.end())
        {
            it->second->DataPageNumbers().push_back(pageNumber);
        }
    }
}

std::vector<std::string> SdfDatabase::ListTables() const
{
    std::vector<std::string> names;
    names.reserve(_tables.size());
    for (const auto& [name, table] : _tables)
    {
        names.push_back(name);
    }
    return names;
}

const domain::TableDef& SdfDatabase::RequireTable(const std::string& tableName) const
{
    const auto it = _tables.find(tableName);
    if (it == _tables.end())
    {
        throw std::out_of_range("no such table: " + tableName);
    }
    return it->second;
}

std::vector<ColumnSchema> SdfDatabase::TableSchema(const std::string& tableName) const
{
    const domain::TableDef& table = RequireTable(tableName);

    std::vector<ColumnSchema> result;
    result.reserve(table.Columns().size());

    for (const domain::ColumnDef& column : table.Columns())
    {
        ColumnSchema schema;
        schema.ordinal = column.Ordinal();
        schema.name = column.Name();
        schema.typeName = domain::NameOf(column.Type());
        schema.declaredSize = column.DeclaredSize();
        if (column.Type() == domain::ColumnType::Numeric)
        {
            schema.precision = column.Precision();
            schema.scale = column.Scale();
        }
        result.push_back(std::move(schema));
    }

    return result;
}

std::vector<domain::Row> SdfDatabase::ReadTable(const std::string& tableName) const
{
    const domain::TableDef& table = RequireTable(tableName);

    std::vector<domain::Row> rows;
    for (const std::size_t pageNumber : table.DataPageNumbers())
    {
        const infrastructure::PageView page(_storage->PageBytes(pageNumber));
        for (const infrastructure::RowSlice& slice : page.Rows())
        {
            rows.push_back(_rowDecoder->Decode(table, slice.bytes));
        }
    }

    return rows;
}

}
