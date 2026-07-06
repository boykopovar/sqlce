#ifndef SDF_DOMAIN_ROW_HPP
#define SDF_DOMAIN_ROW_HPP

#include <string>
#include <utility>
#include <vector>

#include "sdf/domain/ColumnValue.hpp"

namespace sdf::domain
{

class Row
{
public:
    void Append(std::string columnName, ColumnValue value);

    const std::vector<std::pair<std::string, ColumnValue>>& Values() const;
    const ColumnValue* Find(const std::string& columnName) const;

private:
    std::vector<std::pair<std::string, ColumnValue>> _values;
};

}

#endif
