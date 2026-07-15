#include "sdf/domain/ColumnValue.hpp"

#include <cstdio>
#include <sstream>

namespace sdf::domain
{

namespace
{

std::string BytesToHex(const std::vector<std::uint8_t>& bytes)
{
    std::string result;
    result.reserve(bytes.size() * 2);
    static constexpr char digits[] = "0123456789abcdef";
    for (const std::uint8_t byte : bytes)
    {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 0x0F]);
    }
    return result;
}

}

ColumnValue::ColumnValue() : _storage(NullValue{})
{
}

ColumnValue::ColumnValue(ColumnValueStorage storage) : _storage(std::move(storage))
{
}

bool ColumnValue::IsNull() const
{
    return std::holds_alternative<NullValue>(_storage);
}

const ColumnValueStorage& ColumnValue::Storage() const
{
    return _storage;
}

std::string ColumnValue::ToDisplayString() const
{
    struct Visitor
    {
        std::string operator()(const NullValue&) const
        {
            return "None";
        }

        std::string operator()(std::int64_t value) const
        {
            return std::to_string(value);
        }

        std::string operator()(std::uint64_t value) const
        {
            return std::to_string(value);
        }

        std::string operator()(double value) const
        {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%g", value);
            return std::string(buffer);
        }

        std::string operator()(bool value) const
        {
            return value ? "True" : "False";
        }

        std::string operator()(const std::string& value) const
        {
            return value;
        }

        std::string operator()(const std::vector<std::uint8_t>& value) const
        {
            return "0x" + BytesToHex(value);
        }

        std::string operator()(const DateTimeValue& value) const
        {
            return value.ToString();
        }

        std::string operator()(const NumericValue& value) const
        {
            return value.ToString();
        }

        std::string operator()(const Guid& value) const
        {
            return value.ToString();
        }

        std::string operator()(const LazyLob& value) const
        {
            return value.IsText() ? value.ReadText() : ("0x" + BytesToHex(value.ReadBytes()));
        }
    };

    return std::visit(Visitor{}, _storage);
}

}
