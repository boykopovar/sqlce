#ifndef SDF_PARSING_I_ROW_DECODER_HPP
#define SDF_PARSING_I_ROW_DECODER_HPP

#include <span>

#include "sdf/domain/Row.hpp"
#include "sdf/domain/TableDef.hpp"

namespace sdf::parsing
{

class IRowDecoder
{
public:
    virtual ~IRowDecoder() = default;

    virtual domain::Row Decode(const domain::TableDef& table, std::span<const std::uint8_t> rowBytes) = 0;
};

}

#endif
