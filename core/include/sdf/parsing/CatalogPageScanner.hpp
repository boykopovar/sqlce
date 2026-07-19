#ifndef SDF_PARSING_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_CATALOG_PAGE_SCANNER_HPP

#include "sdf/parsing/interfaces/ICatalogPageScanner.hpp"

namespace sdf::parsing
{

class CatalogPageScanner final : public ICatalogPageScanner
{
public:
    [[nodiscard]] std::set<std::uint8_t> FindCatalogObjectIds(const domain::IPageStorage& storage) const override;
    [[nodiscard]] std::vector<std::vector<std::uint8_t>> CollectCatalogRows(const domain::IPageStorage& storage) const override;
};

}

#endif
