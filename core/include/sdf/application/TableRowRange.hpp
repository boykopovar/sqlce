#ifndef SDF_APPLICATION_TABLE_ROW_RANGE_HPP
#define SDF_APPLICATION_TABLE_ROW_RANGE_HPP

#include <cstddef>
#include <iterator>
#include <optional>
#include <vector>

#include "sdf/domain/IPageStorage.hpp"
#include "sdf/domain/Row.hpp"
#include "sdf/domain/TableDef.hpp"
#include "sdf/infrastructure/PageView.hpp"
#include "sdf/parsing/IRowDecoder.hpp"

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
            std::size_t pageIndex);

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
        std::size_t _pageIndex;
        std::size_t _slotIndex;
        std::vector<infrastructure::RowSlice> _currentPageRows;
        std::optional<domain::Row> _current;

        void LoadPageAt(std::size_t pageIndex);
        void SkipEmptyPages();
        void LoadCurrentRow();
    };

    TableRowRange(const domain::IPageStorage* storage, const domain::TableDef* table, parsing::IRowDecoder* rowDecoder);

    Iterator begin() const;
    Iterator end() const;

private:
    const domain::IPageStorage* _storage;
    const domain::TableDef* _table;
    parsing::IRowDecoder* _rowDecoder;
};

}

#endif
