#ifndef SDF_INFRASTRUCTURE_FILE_STORAGE_HPP
#define SDF_INFRASTRUCTURE_FILE_STORAGE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "sdf/domain/IPageCipher.hpp"
#include "sdf/domain/IPageStorage.hpp"

namespace sdf::infrastructure
{

std::vector<std::uint8_t> ReadFirstPageRaw(const std::string& path);

class FileStorage final : public domain::IPageStorage
{
public:
    explicit FileStorage(const std::string& path);
    FileStorage(const std::string& path, const domain::IPageCipher& cipher);

    std::size_t PageCount() const override;
    std::span<const std::uint8_t> PageBytes(std::size_t pageNumber) const override;

private:
    std::vector<std::uint8_t> _fileBytes;
    std::size_t _pageCount;

    void Load(const std::string& path);
};

}

#endif
