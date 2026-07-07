#ifndef SDF_APPLICATION_SDF_DATABASE_HPP
#define SDF_APPLICATION_SDF_DATABASE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/domain/IPageCipher.hpp"
#include "sdf/domain/IPageStorage.hpp"
#include "sdf/domain/Row.hpp"
#include "sdf/domain/TableDef.hpp"
#include "sdf/parsing/ICatalogPageScanner.hpp"
#include "sdf/parsing/ILobChainRegistry.hpp"
#include "sdf/parsing/IRowDecoder.hpp"
#include "sdf/parsing/ITableCatalogBuilder.hpp"

namespace sdf::application
{

class SdfDatabase
{
public:
    explicit SdfDatabase(const std::string& path);
    SdfDatabase(const std::string& path, const std::string& password);

    std::vector<std::string> ListTables() const;
    std::vector<ColumnSchema> TableSchema(const std::string& tableName) const;
    std::vector<domain::Row> ReadTable(const std::string& tableName) const;

private:
    std::unique_ptr<domain::IPageStorage> _storage;
    std::shared_ptr<parsing::ICatalogPageScanner> _pageScanner;
    std::shared_ptr<parsing::ITableCatalogBuilder> _tableCatalogBuilder;
    std::shared_ptr<parsing::ILobChainRegistry> _lobChainRegistry;
    std::shared_ptr<parsing::IRowDecoder> _rowDecoder;

    std::map<std::string, domain::TableDef> _tables;

    explicit SdfDatabase(std::unique_ptr<domain::IPageStorage> storage);

    static std::unique_ptr<domain::IPageStorage> OpenStorage(const std::string& path, const std::string& password);

    void AssignDataPages();
    const domain::TableDef& RequireTable(const std::string& tableName) const;
};

}

#endif
