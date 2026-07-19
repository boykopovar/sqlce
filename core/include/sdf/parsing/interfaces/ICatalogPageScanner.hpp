#ifndef SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP

#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/domain/TableDef.hpp"

namespace sdf::parsing
{

class ICatalogPageScanner
{
public:
    virtual ~ICatalogPageScanner() = default;

    virtual std::set<std::uint8_t> FindCatalogObjectIds(const domain::IPageStorage& storage) const = 0;

    virtual std::vector<std::vector<std::uint8_t>> CollectCatalogRows(const domain::IPageStorage& storage) const = 0;

    virtual void AssignDataPages(
        const domain::IPageStorage& storage, const std::map<std::uint8_t, domain::TableDef*>& tableByObjectId) const
        = 0;
};

}

#endif
