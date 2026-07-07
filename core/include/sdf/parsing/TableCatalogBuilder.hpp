#ifndef SDF_PARSING_TABLE_CATALOG_BUILDER_HPP
#define SDF_PARSING_TABLE_CATALOG_BUILDER_HPP

#include <memory>

#include "sdf/parsing/ICatalogPageScanner.hpp"
#include "sdf/parsing/ICatalogRowDecoder.hpp"
#include "sdf/parsing/ITableCatalogBuilder.hpp"

namespace sdf::parsing
{

class TableCatalogBuilder final : public ITableCatalogBuilder
{
public:
    TableCatalogBuilder(
        std::shared_ptr<ICatalogPageScanner> pageScanner, std::shared_ptr<ICatalogRowDecoder> rowDecoder);

    std::map<std::string, domain::TableDef> BuildTables(const domain::IPageStorage& storage) const override;

private:
    std::shared_ptr<ICatalogPageScanner> _pageScanner;
    std::shared_ptr<ICatalogRowDecoder> _rowDecoder;
};

}

#endif
