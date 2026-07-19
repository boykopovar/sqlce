#ifndef SDF_APPLICATION_SDF_DATABASE_HPP
#define SDF_APPLICATION_SDF_DATABASE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/application/TableRowRange.hpp"
#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/interfaces/IPageCipher.hpp"
#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/domain/Row.hpp"
#include "sdf/domain/TableDef.hpp"
#include "sdf/parsing/interfaces/ICatalogPageScanner.hpp"
#include "sdf/parsing/interfaces/ILobChainRegistry.hpp"
#include "sdf/parsing/interfaces/IRowDecoder.hpp"
#include "sdf/parsing/interfaces/ITableCatalogBuilder.hpp"

namespace sdf::application
{

class SqlceDatabase
{
public:
    explicit SqlceDatabase(const std::string& path);
    SqlceDatabase(const std::string& path, const std::string& password);

    [[nodiscard]] std::vector<std::string> ListTables() const;
    [[nodiscard]] std::vector<ColumnSchema> TableSchema(const std::string& tableName) const;
    [[nodiscard]] TableRowRange IterateTable(const std::string& tableName) const;
    [[nodiscard]] std::vector<domain::Row> ReadTable(const std::string& tableName) const;
    [[nodiscard]] domain::EncryptionMode GetEncryptionMode() const;
    [[nodiscard]] static domain::EncryptionMode GetEncryptionMode(const std::string& path);

private:
    domain::EncryptionMode _encryptionMode;
    std::unique_ptr<domain::IPageStorage> _storage;
    std::shared_ptr<parsing::ICatalogPageScanner> _pageScanner;
    std::shared_ptr<parsing::ITableCatalogBuilder> _tableCatalogBuilder;
    std::shared_ptr<parsing::ILobChainRegistry> _lobChainRegistry;
    std::shared_ptr<parsing::IRowDecoder> _rowDecoder;

    std::map<std::string, domain::TableDef> _tables;

    struct OpenResult
    {
        std::unique_ptr<domain::IPageStorage> storage;
        domain::EncryptionMode encryptionMode;
    };

    static OpenResult Open(const std::string& path, const std::string& password);
    explicit SqlceDatabase(OpenResult opened);

    void AssignDataPages();
    [[nodiscard]] const domain::TableDef& RequireTable(const std::string& tableName) const;
};

}

#endif
