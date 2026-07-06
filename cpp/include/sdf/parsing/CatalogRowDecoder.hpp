#ifndef SDF_PARSING_CATALOG_ROW_DECODER_HPP
#define SDF_PARSING_CATALOG_ROW_DECODER_HPP

#include "sdf/parsing/ICatalogRowDecoder.hpp"

namespace sdf::parsing
{

class CatalogRowDecoder final : public ICatalogRowDecoder
{
public:
    std::optional<CatalogRow> Decode(std::span<const std::uint8_t> rowBytes) const override;

private:
    static bool IsFieldPresent(std::span<const std::uint8_t> presence, std::uint8_t columnId);
};

}

#endif
