#include "sdf/domain/LazyLob.hpp"

#include <utility>

#include "sdf/domain/TextDecoder.hpp"

namespace sdf::domain
{

LazyLobChunkRange::Iterator::Iterator() :
    _source(nullptr),
    _chunkIndex(0),
    _current(),
    _loaded(false)
{
}

LazyLobChunkRange::Iterator::Iterator(std::shared_ptr<const ILazyLobSource> source, std::size_t chunkIndex)
    : _source(std::move(source)), _chunkIndex(chunkIndex), _current(), _loaded(false)
{
}

void LazyLobChunkRange::Iterator::EnsureLoaded() const
{
    if (!_loaded)
    {
        _current = _source->ReadChunk(_chunkIndex);
        _loaded = true;
    }
}

LazyLobChunkRange::Iterator::reference LazyLobChunkRange::Iterator::operator*() const
{
    EnsureLoaded();
    return _current;
}

LazyLobChunkRange::Iterator::pointer LazyLobChunkRange::Iterator::operator->() const
{
    EnsureLoaded();
    return &_current;
}

LazyLobChunkRange::Iterator& LazyLobChunkRange::Iterator::operator++()
{
    ++_chunkIndex;
    _loaded = false;
    _current = value_type();
    return *this;
}

LazyLobChunkRange::Iterator LazyLobChunkRange::Iterator::operator++(int)
{
    Iterator copy = *this;
    ++(*this);
    return copy;
}

bool LazyLobChunkRange::Iterator::operator==(const Iterator& other) const
{
    return _chunkIndex == other._chunkIndex;
}

bool LazyLobChunkRange::Iterator::operator!=(const Iterator& other) const
{
    return !(*this == other);
}

LazyLobChunkRange::LazyLobChunkRange(std::shared_ptr<const ILazyLobSource> source) : _source(std::move(source))
{
}

LazyLobChunkRange::Iterator LazyLobChunkRange::begin() const
{
    return Iterator(_source, 0);
}

LazyLobChunkRange::Iterator LazyLobChunkRange::end() const
{
    return Iterator(_source, _source->ChunkCount());
}

LazyLob::LazyLob(std::shared_ptr<const ILazyLobSource> source, bool isText, bool compressed)
    : _source(std::move(source)), _isText(isText), _compressed(compressed)
{
}

std::size_t LazyLob::TotalLength() const
{
    return _source->TotalLength();
}

bool LazyLob::IsText() const
{
    return _isText;
}

std::vector<std::uint8_t> LazyLob::ReadBytes() const
{
    std::vector<std::uint8_t> buffer;
    buffer.reserve(_source->TotalLength());

    const std::size_t chunkCount = _source->ChunkCount();
    for (std::size_t i = 0; i < chunkCount; ++i)
    {
        const std::vector<std::uint8_t> chunk = _source->ReadChunk(i);
        buffer.insert(buffer.end(), chunk.begin(), chunk.end());
    }

    return buffer;
}

std::string LazyLob::ReadText() const
{
    const std::vector<std::uint8_t> bytes = ReadBytes();
    return DecodeText(std::span<const std::uint8_t>(bytes), _compressed);
}

LazyLobChunkRange LazyLob::ReadChunks() const
{
    return LazyLobChunkRange(_source);
}

}
