#ifndef SDF_DOMAIN_TABLE_DEF_HPP
#define SDF_DOMAIN_TABLE_DEF_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "sdf/domain/ColumnDef.hpp"

namespace sdf::domain
{

class TableDef
{
public:
    TableDef(std::string name, std::uint8_t objectId, std::uint8_t objectGeneration, std::uint32_t rootLogicalPageId);

    [[nodiscard]] const std::string& Name() const;
    [[nodiscard]] std::uint8_t ObjectId() const;
    [[nodiscard]] std::uint8_t ObjectGeneration() const;
    [[nodiscard]] std::uint32_t RootLogicalPageId() const;

    std::vector<ColumnDef>& Columns();
    [[nodiscard]] const std::vector<ColumnDef>& Columns() const;

    std::vector<std::size_t>& DataPageNumbers();
    [[nodiscard]] const std::vector<std::size_t>& DataPageNumbers() const;

private:
    std::string _name;
    std::uint8_t _objectId;
    std::uint8_t _objectGeneration;
    std::uint32_t _rootLogicalPageId;
    std::vector<ColumnDef> _columns;
    std::vector<std::size_t> _dataPageNumbers;
};

}

#endif
