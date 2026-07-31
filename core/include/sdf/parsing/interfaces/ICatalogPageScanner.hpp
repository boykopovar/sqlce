#ifndef SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_I_CATALOG_PAGE_SCANNER_HPP

#include <cstdint>
#include <optional>
#include <vector>

#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/domain/TableDef.hpp"

namespace sdf::parsing
{

class ICatalogPageScanner
{
public:
    virtual ~ICatalogPageScanner() = default;

    virtual std::vector<std::vector<std::uint8_t>> CollectCatalogRows(const domain::IPageStorage& storage) const = 0;

    virtual void AssignDataPages(const domain::IPageStorage& storage, const std::vector<domain::TableDef*>& tables) const = 0;

    virtual std::optional<std::uint32_t> RowCount(const domain::IPageStorage& storage, std::uint32_t rootLogicalPageId) const = 0;
};

}

#endif
