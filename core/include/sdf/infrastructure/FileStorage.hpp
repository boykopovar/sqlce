#ifndef SDF_INFRASTRUCTURE_FILE_STORAGE_HPP
#define SDF_INFRASTRUCTURE_FILE_STORAGE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "sdf/domain/IPageStorage.hpp"

namespace sdf::infrastructure
{

class FileStorage final : public domain::IPageStorage
{
public:
    explicit FileStorage(const std::string& path);

    std::size_t PageCount() const override;
    std::span<const std::uint8_t> PageBytes(std::size_t pageNumber) const override;

private:
    std::vector<std::uint8_t> _fileBytes;
    std::size_t _pageCount;
};

}

#endif
