#include "sdf/application/SqlceDatabase.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>

#include "sdf/domain/PageSize.hpp"
#include "sdf/infrastructure/PageChecksum.hpp"
#include "sdf/parsing/SdfFormat.hpp"

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

std::vector<std::uint8_t> SqlceDatabase::ExportDecrypted() const
{
    const std::size_t pageCount = _storage->PageCount();

    std::vector<std::uint8_t> result;
    result.reserve(pageCount * domain::PageSize);

    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::span<const std::uint8_t> page = _storage->PageBytes(pageNumber);
        result.insert(result.end(), page.begin(), page.end());
    }

    if (_resolvedEncryptionMode != domain::EncryptionMode::None)
    {
        ClearPage0EncryptionFields(result);
        RecomputePage0Checksum(result);
    }

    return result;
}

void SqlceDatabase::ClearPage0EncryptionFields(std::vector<std::uint8_t>& pages)
{
    std::fill_n(pages.begin() + parsing::Page0ModeOffset, sizeof(std::uint32_t), std::uint8_t(0));
    std::fill_n(pages.begin() + parsing::Page0VerifierOffset, parsing::Page0VerifierLength, std::uint8_t(0));
    std::fill_n(pages.begin() + parsing::Page0KeyParamOffset, parsing::Page0KeyParamLength, std::uint8_t(0));
    std::fill_n(pages.begin() + parsing::Page0ProviderInfoOffset, parsing::Page0ProviderInfoLength, std::uint8_t(0));
    std::fill_n(pages.begin() + parsing::Page0WasEncryptedFlagOffset, parsing::Page0WasEncryptedFlagLength, std::uint8_t(0));
}

void SqlceDatabase::RecomputePage0Checksum(std::vector<std::uint8_t>& pages)
{
    const std::span<std::uint8_t> page0(pages.data(), domain::PageSize);
    std::fill_n(page0.begin(), sizeof(std::uint32_t), std::uint8_t(0));

    const std::uint32_t checksum = infrastructure::ComputePageChecksum(page0);

    page0[0] = static_cast<std::uint8_t>(checksum & 0xFF);
    page0[1] = static_cast<std::uint8_t>((checksum >> 8) & 0xFF);
    page0[2] = static_cast<std::uint8_t>((checksum >> 16) & 0xFF);
    page0[3] = static_cast<std::uint8_t>((checksum >> 24) & 0xFF);
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
