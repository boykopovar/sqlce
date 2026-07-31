#ifndef SDF_PARSING_CATALOG_PAGE_SCANNER_HPP
#define SDF_PARSING_CATALOG_PAGE_SCANNER_HPP

#include <memory>
#include <optional>

#include "sdf/parsing/interfaces/ICatalogPageScanner.hpp"
#include "sdf/parsing/interfaces/ILogicalPageResolver.hpp"

namespace sdf::parsing
{

class CatalogPageScanner final : public ICatalogPageScanner
{
public:
    explicit CatalogPageScanner(std::shared_ptr<ILogicalPageResolver> logicalPageResolver);

    [[nodiscard]] std::vector<std::vector<std::uint8_t>> CollectCatalogRows(const domain::IPageStorage& storage) const override;

    void AssignDataPages(const domain::IPageStorage& storage, const std::vector<domain::TableDef*>& tables) const override;

    [[nodiscard]] std::optional<std::uint32_t> RowCount(const domain::IPageStorage& storage, std::uint32_t rootLogicalPageId) const override;

private:
    std::shared_ptr<ILogicalPageResolver> _logicalPageResolver;

    [[nodiscard]] std::vector<std::size_t> _ResolveHeapPagesFromRoot(
        const domain::IPageStorage& storage, std::uint32_t rootLogicalPageId) const;
};

}

#endif
