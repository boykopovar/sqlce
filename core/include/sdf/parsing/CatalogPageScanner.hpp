#ifndef SDF_PARSING_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_CATALOG_PAGE_SCANNER_HPP

#include <memory>

#include "sdf/parsing/interfaces/ICatalogPageScanner.hpp"
#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"

namespace sdf::parsing
{

class CatalogPageScanner final : public ICatalogPageScanner
{
public:
    explicit CatalogPageScanner(std::shared_ptr<ILogicalPageResolver> logicalPageResolver);

    [[nodiscard]] std::set<std::uint8_t> FindCatalogObjectIds(const domain::IPageStorage& storage) const override;
    [[nodiscard]] std::vector<std::vector<std::uint8_t>> CollectCatalogRows(const domain::IPageStorage& storage) const override;

    void AssignDataPages(
        const domain::IPageStorage& storage,
        const std::map<std::uint8_t, domain::TableDef*>& tableByObjectId) const override;

private:
    std::shared_ptr<ILogicalPageResolver> _logicalPageResolver;
};

}

#endif
