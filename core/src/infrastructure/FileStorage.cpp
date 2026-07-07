#include "sdf/infrastructure/FileStorage.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/PageLayout.hpp"

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

FileStorage::FileStorage(const std::string& path) : _pageCount(0)
{
    Load(path);
}

FileStorage::FileStorage(const std::string& path, const domain::IPageCipher& cipher) : _pageCount(0)
{
    Load(path);

    if (!cipher.VerifyPassword())
    {
        throw domain::InvalidPasswordException();
    }

    for (std::size_t pageNumber = 0; pageNumber < _pageCount; ++pageNumber)
    {
        const std::size_t offset = pageNumber * domain::PageSize;
        const std::span<const std::uint8_t> encryptedPage(_fileBytes.data() + offset, domain::PageSize);
        const std::vector<std::uint8_t> decryptedPage = cipher.DecryptPage(pageNumber, encryptedPage);
        std::copy(decryptedPage.begin(), decryptedPage.end(), _fileBytes.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

void FileStorage::Load(const std::string& path)
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
