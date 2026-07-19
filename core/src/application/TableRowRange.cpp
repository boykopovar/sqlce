#include "sdf/application/TableRowRange.hpp"

namespace sdf::application
{

TableRowRange::Iterator::Iterator()
    : _storage(nullptr), _table(nullptr), _rowDecoder(nullptr), _reassembler(nullptr), _cursor(std::nullopt)
{
}

TableRowRange::Iterator::Iterator(
    const domain::IPageStorage* storage,
    const domain::TableDef* table,
    parsing::IRowDecoder* rowDecoder,
    const parsing::IRowFragmentReassembler* reassembler,
    bool atEnd)
    : _storage(storage), _table(table), _rowDecoder(rowDecoder), _reassembler(reassembler), _cursor(std::nullopt)
{
    if (!atEnd)
    {
        LoadCurrentRow();
    }
}

void TableRowRange::Iterator::LoadCurrentRow()
{
    const parsing::RowCursor searchFrom = _cursor.has_value() ? *_cursor : parsing::RowCursor{0, 0};
    const std::optional<parsing::AssembledRow> found = _cursor.has_value()
        ? _reassembler->FindNext(*_storage, _table->DataPageNumbers(), searchFrom)
        : _reassembler->FindFirst(*_storage, _table->DataPageNumbers(), searchFrom);

    if (!found.has_value())
    {
        _cursor = std::nullopt;
        _current = std::nullopt;
        return;
    }

    _cursor = found->cursor;
    _current = _rowDecoder->Decode(*_table, std::span<const std::uint8_t>(found->bytes.data(), found->bytes.size()));
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
    if (!_current.has_value() || !other._current.has_value())
    {
        return _current.has_value() == other._current.has_value();
    }
    return _cursor->pageIndex == other._cursor->pageIndex && _cursor->slotIndex == other._cursor->slotIndex;
}

bool TableRowRange::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

TableRowRange::TableRowRange(
    const domain::IPageStorage* storage,
    const domain::TableDef* table,
    parsing::IRowDecoder* rowDecoder,
    const parsing::IRowFragmentReassembler* reassembler)
    : _storage(storage), _table(table), _rowDecoder(rowDecoder), _reassembler(reassembler)
{
}

TableRowRange::Iterator TableRowRange::begin() const
{
    return Iterator(_storage, _table, _rowDecoder, _reassembler, false);
}

TableRowRange::Iterator TableRowRange::end() const
{
    return Iterator(_storage, _table, _rowDecoder, _reassembler, true);
}

}
