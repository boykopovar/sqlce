#ifndef SDF_PARSING_I_TABLE_CATALOG_BUILDER_HPP
#define SDF_PARSING_I_TABLE_CATALOG_BUILDER_HPP

#include <map>
#include <string>

#include "sdf/domain/IPageStorage.hpp"
#include "sdf/domain/TableDef.hpp"

namespace sdf::parsing
{

class ITableCatalogBuilder
{
public:
    virtual ~ITableCatalogBuilder() = default;

    virtual std::map<std::string, domain::TableDef> BuildTables(const domain::IPageStorage& storage) const = 0;
};

}

#endif
