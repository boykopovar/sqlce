#include "sdf/domain/Row.hpp"

#include <algorithm>

namespace sdf::domain
{

void Row::Append(std::string columnName, ColumnValue value)
{
    _values.emplace_back(std::move(columnName), std::move(value));
}

const std::vector<std::pair<std::string, ColumnValue>>& Row::Values() const
{
    return _values;
}

const ColumnValue* Row::Find(const std::string& columnName) const
{
    const auto it = std::find_if(
        _values.begin(), _values.end(), [&columnName](const auto& entry) { return entry.first == columnName; });
    if (it == _values.end())
    {
        return nullptr;
    }
    return &it->second;
}

}
