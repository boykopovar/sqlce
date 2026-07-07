#include "sdf/domain/TableDef.hpp"

namespace sdf::domain
{

TableDef::TableDef(std::string name, std::uint8_t objectId) : _name(std::move(name)), _objectId(objectId)
{
}

const std::string& TableDef::Name() const
{
    return _name;
}

std::uint8_t TableDef::ObjectId() const
{
    return _objectId;
}

std::vector<ColumnDef>& TableDef::Columns()
{
    return _columns;
}

const std::vector<ColumnDef>& TableDef::Columns() const
{
    return _columns;
}

std::vector<std::size_t>& TableDef::DataPageNumbers()
{
    return _dataPageNumbers;
}

const std::vector<std::size_t>& TableDef::DataPageNumbers() const
{
    return _dataPageNumbers;
}

}
