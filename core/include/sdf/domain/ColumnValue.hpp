#ifndef SDF_DOMAIN_COLUMN_VALUE_HPP
#define SDF_DOMAIN_COLUMN_VALUE_HPP

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "sdf/domain/DateTimeValue.hpp"
#include "sdf/domain/Guid.hpp"
#include "sdf/domain/LazyLob.hpp"
#include "sdf/domain/NumericValue.hpp"

namespace sdf::domain
{

struct NullValue
{
};

using ColumnValueStorage = std::variant<
    NullValue,
    std::int64_t,
    std::uint64_t,
    double,
    bool,
    std::string,
    std::vector<std::uint8_t>,
    DateTimeValue,
    NumericValue,
    Guid,
    LazyLob>;

class ColumnValue
{
public:
    ColumnValue();
    explicit ColumnValue(ColumnValueStorage storage);

    bool IsNull() const;

    std::string ToDisplayString() const;

    const ColumnValueStorage& Storage() const;

private:
    ColumnValueStorage _storage;
};

}

#endif
