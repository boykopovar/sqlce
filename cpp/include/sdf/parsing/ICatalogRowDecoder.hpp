#ifndef SDF_PARSING_I_CATALOG_ROW_DECODER_HPP
#define SDF_PARSING_I_CATALOG_ROW_DECODER_HPP

#include <cstdint>
#include <optional>
#include <span>

#include "sdf/parsing/CatalogRow.hpp"

namespace sdf::parsing
{

class ICatalogRowDecoder
{
public:
    virtual ~ICatalogRowDecoder() = default;

    virtual std::optional<CatalogRow> Decode(std::span<const std::uint8_t> rowBytes) const = 0;
};

}

#endif
