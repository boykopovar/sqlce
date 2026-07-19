#ifndef SDF_APPLICATION_TABLE_ROW_RANGE_HPP
#define SDF_APPLICATION_TABLE_ROW_RANGE_HPP

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <vector>

#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/domain/Row.hpp"
#include "sdf/domain/TableDef.hpp"
#include "sdf/parsing/interfaces/IRowDecoder.hpp"
#include "sdf/parsing/interfaces/IRowFragmentReassembler.hpp"

namespace sdf::application
{

class TableRowRange
{
public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = domain::Row;
        using difference_type = std::ptrdiff_t;
        using pointer = const domain::Row*;
        using reference = const domain::Row&;

        Iterator();
        Iterator(
            const domain::IPageStorage* storage,
            const domain::TableDef* table,
            parsing::IRowDecoder* rowDecoder,
            const parsing::IRowFragmentReassembler* reassembler,
            bool atEnd);

        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int);

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        const domain::IPageStorage* _storage;
        const domain::TableDef* _table;
        parsing::IRowDecoder* _rowDecoder;
        const parsing::IRowFragmentReassembler* _reassembler;
        std::optional<parsing::RowCursor> _cursor;
        std::optional<domain::Row> _current;

        void LoadCurrentRow();
    };

    TableRowRange(
        const domain::IPageStorage* storage,
        const domain::TableDef* table,
        parsing::IRowDecoder* rowDecoder,
        const parsing::IRowFragmentReassembler* reassembler);

    Iterator begin() const;
    Iterator end() const;

private:
    const domain::IPageStorage* _storage;
    const domain::TableDef* _table;
    parsing::IRowDecoder* _rowDecoder;
    const parsing::IRowFragmentReassembler* _reassembler;
};

}

#endif
