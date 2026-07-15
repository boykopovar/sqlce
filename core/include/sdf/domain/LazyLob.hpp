#ifndef SDF_DOMAIN_LAZY_LOB_HPP
#define SDF_DOMAIN_LAZY_LOB_HPP

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "sdf/domain/ILazyLobSource.hpp"

namespace sdf::domain
{

class LazyLobChunkRange
{
public:
    class Iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::vector<std::uint8_t>;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        Iterator();
        Iterator(std::shared_ptr<const ILazyLobSource> source, std::size_t chunkIndex);

        reference operator*() const;
        pointer operator->() const;
        Iterator& operator++();
        Iterator operator++(int);

        bool operator==(const Iterator& other) const;
        bool operator!=(const Iterator& other) const;

    private:
        std::shared_ptr<const ILazyLobSource> _source;
        std::size_t _chunkIndex;
        mutable value_type _current;
        mutable bool _loaded;

        void EnsureLoaded() const;
    };

    explicit LazyLobChunkRange(std::shared_ptr<const ILazyLobSource> source);

    Iterator begin() const;
    Iterator end() const;

private:
    std::shared_ptr<const ILazyLobSource> _source;
};

class LazyLob
{
public:
    LazyLob(std::shared_ptr<const ILazyLobSource> source, bool isText, bool compressed);

    std::size_t TotalLength() const;
    bool IsText() const;

    std::vector<std::uint8_t> ReadBytes() const;
    std::string ReadText() const;

    LazyLobChunkRange ReadChunks() const;

private:
    std::shared_ptr<const ILazyLobSource> _source;
    bool _isText;
    bool _compressed;
};

}

#endif
