#include "sdf/application/SqlceDatabase.hpp"

#include <stdexcept>

namespace sdf::application
{

SqlceDatabase::SqlceDatabase(const std::string& path)
    : SqlceDatabase(parsing::OpenSdfFile(path, std::string()))
{
}

SqlceDatabase::SqlceDatabase(const std::string& path, const std::string& password)
    : SqlceDatabase(parsing::OpenSdfFile(path, password))
{
}

SqlceDatabase::SqlceDatabase(parsing::SdfDatabaseComponents components)
    : _encryptionMode(components.encryptionMode)
    , _resolvedEncryptionMode(components.resolvedEncryptionMode)
    , _formatVersion(components.formatVersion)
    , _storage(std::move(components.storage))
    , _pageScanner(std::move(components.pageScanner))
    , _tableCatalogBuilder(std::move(components.tableCatalogBuilder))
    , _lobChainRegistry(std::move(components.lobChainRegistry))
    , _rowDecoder(std::move(components.rowDecoder))
    , _rowFragmentReassembler(std::move(components.rowFragmentReassembler))
{
    _tables = _tableCatalogBuilder->BuildTables(*_storage);
    AssignDataPages();
}

void SqlceDatabase::AssignDataPages()
{
    std::map<std::uint8_t, domain::TableDef*> tableByObjectId;
    for (auto& [name, table] : _tables)
    {
        tableByObjectId[table.ObjectId()] = &table;
    }

    _pageScanner->AssignDataPages(*_storage, tableByObjectId);
}

std::vector<std::string> SqlceDatabase::ListTables() const
{
    std::vector<std::string> names;
    names.reserve(_tables.size());
    for (const auto& [name, table] : _tables)
    {
        names.push_back(name);
    }
    return names;
}

const domain::TableDef& SqlceDatabase::RequireTable(const std::string& tableName) const
{
    const auto it = _tables.find(tableName);
    if (it == _tables.end())
    {
        throw std::out_of_range("no such table: " + tableName);
    }
    return it->second;
}

std::vector<ColumnSchema> SqlceDatabase::TableSchema(const std::string& tableName) const
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

TableRowRange SqlceDatabase::IterateTable(const std::string& tableName) const
{
    const domain::TableDef& table = RequireTable(tableName);
    return TableRowRange(_storage.get(), &table, _rowDecoder.get(), _rowFragmentReassembler.get());
}

std::vector<domain::Row> SqlceDatabase::ReadTable(const std::string& tableName) const
{
    const TableRowRange range = IterateTable(tableName);
    return std::vector<domain::Row>(range.begin(), range.end());
}

domain::EncryptionMode SqlceDatabase::GetEncryptionMode() const
{
    return _encryptionMode;
}

domain::EncryptionMode SqlceDatabase::GetEncryptionMode(const std::string& path)
{
    return parsing::ReadSdfEncryptionMode(path);
}

domain::EncryptionMode SqlceDatabase::ResolvedEncryptionMode() const
{
    return _resolvedEncryptionMode;
}

domain::FormatVersion SqlceDatabase::GetFormatVersion() const
{
    return _formatVersion;
}

domain::FormatVersion SqlceDatabase::GetFormatVersion(const std::string& path)
{
    return parsing::ReadSdfFormatVersion(path);
}

}
