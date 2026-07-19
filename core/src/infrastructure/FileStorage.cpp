#include "sdf/infrastructure/FileStorage.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/PageSize.hpp"

namespace sdf::infrastructure
{

std::vector<std::uint8_t> ReadFirstPageRaw(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::vector<std::uint8_t> page(domain::PageSize);
    file.read(reinterpret_cast<char*>(page.data()), static_cast<std::streamsize>(domain::PageSize));
    if (!file)
    {
        throw std::runtime_error("file is smaller than one page, not a valid .sdf: " + path);
    }

    return page;
}

FileStorage::FileStorage(const std::string& path) : _pageCount(0), _cipher(nullptr), _pageBuffer(domain::PageSize)
{
    Open(path);
}

FileStorage::FileStorage(const std::string& path, std::shared_ptr<const domain::IPageCipher> cipher)
    : _pageCount(0), _cipher(std::move(cipher)), _pageBuffer(domain::PageSize)
{
    Open(path);

    if (!_cipher->VerifyPassword())
    {
        throw domain::InvalidPasswordException();
    }
}

void FileStorage::Open(const std::string& path)
{
    _file.open(path, std::ios::binary | std::ios::ate);
    if (!_file.is_open())
    {
        throw std::runtime_error("cannot open file: " + path);
    }

    const std::streamsize size = _file.tellg();
    if (size < 0)
    {
        throw std::runtime_error("cannot determine file size: " + path);
    }

    if (static_cast<std::size_t>(size) % domain::PageSize != 0)
    {
        throw std::runtime_error("file size is not a multiple of page size, not a valid .sdf: " + path);
    }

    _pageCount = static_cast<std::size_t>(size) / domain::PageSize;
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

    const std::streamoff offset = static_cast<std::streamoff>(pageNumber * domain::PageSize);
    _file.seekg(offset, std::ios::beg);
    _file.read(reinterpret_cast<char*>(_pageBuffer.data()), static_cast<std::streamsize>(domain::PageSize));
    if (!_file)
    {
        throw std::runtime_error("failed to read page " + std::to_string(pageNumber));
    }

    if (_cipher)
    {
        const std::vector<std::uint8_t> decrypted = _cipher->DecryptPage(pageNumber, std::span<const std::uint8_t>(_pageBuffer));
        std::copy(decrypted.begin(), decrypted.end(), _pageBuffer.begin());
    }

    return std::span<const std::uint8_t>(_pageBuffer.data(), domain::PageSize);
}

}
