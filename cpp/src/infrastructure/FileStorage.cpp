#include "sdf/infrastructure/FileStorage.hpp"

#include <fstream>
#include <stdexcept>

#include "sdf/domain/PageLayout.hpp"

namespace sdf::infrastructure
{

FileStorage::FileStorage(const std::string& path) : _pageCount(0)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open file: " + path);
    }

    const std::streamsize size = file.tellg();
    if (size < 0)
    {
        throw std::runtime_error("cannot determine file size: " + path);
    }

    if (static_cast<std::size_t>(size) % domain::PageSize != 0)
    {
        throw std::runtime_error("file size is not a multiple of page size, not a valid .sdf: " + path);
    }

    _fileBytes.resize(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!_fileBytes.empty())
    {
        file.read(reinterpret_cast<char*>(_fileBytes.data()), size);
        if (!file)
        {
            throw std::runtime_error("failed to read file: " + path);
        }
    }

    _pageCount = _fileBytes.size() / domain::PageSize;
}

std::size_t FileStorage::PageCount() const
{
    return _pageCount;
}

std::span<const std::uint8_t> FileStorage::PageBytes(std::size_t pageNumber) const
{
    if (pageNumber >= _pageCount)
    {
        throw std::out_of_range("page number out of range");
    }
    const std::size_t offset = pageNumber * domain::PageSize;
    return std::span<const std::uint8_t>(_fileBytes.data() + offset, domain::PageSize);
}

}
