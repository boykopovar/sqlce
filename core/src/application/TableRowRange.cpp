#include "sdf/application/TableRowRange.hpp"

namespace sdf::application
{

TableRowRange::Iterator::Iterator()
    : _storage(nullptr), _table(nullptr), _rowDecoder(nullptr), _pageIndex(0), _slotIndex(0)
{
}

TableRowRange::Iterator::Iterator(
    const domain::IPageStorage* storage,
    const domain::TableDef* table,
    parsing::IRowDecoder* rowDecoder,
    std::size_t pageIndex)
    : _storage(storage), _table(table), _rowDecoder(rowDecoder), _pageIndex(pageIndex), _slotIndex(0)
{
    if (_pageIndex < _table->DataPageNumbers().size())
    {
        LoadPageAt(_pageIndex);
        SkipEmptyPages();
    }
    LoadCurrentRow();
}

void TableRowRange::Iterator::LoadPageAt(std::size_t pageIndex)
{
    const std::span<const std::uint8_t> pageBytes = _storage->PageBytes(_table->DataPageNumbers()[pageIndex]);
    _currentPageRows = infrastructure::PageView(pageBytes).Rows();
}

void TableRowRange::Iterator::SkipEmptyPages()
{
    const std::vector<std::size_t>& pageNumbers = _table->DataPageNumbers();
    while (_currentPageRows.empty() && _pageIndex < pageNumbers.size())
    {
        ++_pageIndex;
        if (_pageIndex < pageNumbers.size())
        {
            LoadPageAt(_pageIndex);
        }
    }
}

void TableRowRange::Iterator::LoadCurrentRow()
{
    if (_pageIndex >= _table->DataPageNumbers().size())
    {
        _current = std::nullopt;
        return;
    }

    _current = _rowDecoder->Decode(*_table, _currentPageRows[_slotIndex].bytes);
}

TableRowRange::Iterator::reference TableRowRange::Iterator::operator*() const
{
    return *_current;
}

TableRowRange::Iterator::pointer TableRowRange::Iterator::operator->() const
{
    return &(*_current);
}

TableRowRange::Iterator& TableRowRange::Iterator::operator++()
{
    ++_slotIndex;
    if (_slotIndex >= _currentPageRows.size())
    {
        _slotIndex = 0;
        ++_pageIndex;
        if (_pageIndex < _table->DataPageNumbers().size())
        {
            LoadPageAt(_pageIndex);
        }
        else
        {
            _currentPageRows.clear();
        }
        SkipEmptyPages();
    }

    LoadCurrentRow();
    return *this;
}

TableRowRange::Iterator TableRowRange::Iterator::operator++(int)
{
    Iterator copy = *this;
    ++(*this);
    return copy;
}

bool TableRowRange::Iterator::operator==(const Iterator& other) const
{
    const std::size_t thisPageCount = _table->DataPageNumbers().size();
    const std::size_t otherPageCount = other._table->DataPageNumbers().size();

    const bool thisIsEnd = _pageIndex >= thisPageCount;
    const bool otherIsEnd = other._pageIndex >= otherPageCount;

    if (thisIsEnd || otherIsEnd)
    {
        return thisIsEnd == otherIsEnd;
    }

    return _pageIndex == other._pageIndex && _slotIndex == other._slotIndex;
}

bool TableRowRange::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

TableRowRange::TableRowRange(
    const domain::IPageStorage* storage, const domain::TableDef* table, parsing::IRowDecoder* rowDecoder)
    : _storage(storage), _table(table), _rowDecoder(rowDecoder)
{
}

TableRowRange::Iterator TableRowRange::begin() const
{
    return Iterator(_storage, _table, _rowDecoder, 0);
}

TableRowRange::Iterator TableRowRange::end() const
{
    return Iterator(_storage, _table, _rowDecoder, _table->DataPageNumbers().size());
}

}
