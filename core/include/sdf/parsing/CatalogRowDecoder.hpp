#ifndef SDF_PARSING_CATALOG_ROW_DECODER_HPP
#define SDF_PARSING_CATALOG_ROW_DECODER_HPP

#include "sdf/parsing/interfaces/ICatalogRowDecoder.hpp"

namespace sdf::parsing
{

class CatalogRowDecoder final : public ICatalogRowDecoder
{
public:
    [[nodiscard]] std::optional<CatalogRow> Decode(std::span<const std::uint8_t> rowBytes) const override;

private:
    static bool IsFieldNull(std::span<const std::uint8_t> presence, std::uint8_t columnId);
};

}

#endif
