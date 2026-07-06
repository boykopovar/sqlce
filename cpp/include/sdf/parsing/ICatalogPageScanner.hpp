#ifndef SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP

#include <cstdint>
#include <set>
#include <vector>

#include "sdf/domain/IPageStorage.hpp"
#include "sdf/infrastructure/PageView.hpp"

namespace sdf::parsing
{

class ICatalogPageScanner
{
public:
    virtual ~ICatalogPageScanner() = default;

    virtual std::set<std::uint8_t> FindCatalogObjectIds(const domain::IPageStorage& storage) const = 0;

    virtual std::vector<infrastructure::RowSlice> CollectCatalogRows(const domain::IPageStorage& storage) const = 0;
};

}

#endif
