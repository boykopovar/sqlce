#ifndef SDF_APPLICATION_COLUMN_SCHEMA_HPP
#define SDF_APPLICATION_COLUMN_SCHEMA_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sdf::application
{

struct ColumnSchema
{
    std::uint16_t ordinal;
    std::string name;
    std::string_view typeName;
    std::uint16_t declaredSize;
    std::optional<std::uint8_t> precision;
    std::optional<std::uint8_t> scale;
};

}

#endif
