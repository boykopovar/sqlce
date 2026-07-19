#ifndef SDF_INFRASTRUCTURE_FILE_STORAGE_HPP
#define SDF_INFRASTRUCTURE_FILE_STORAGE_HPP

#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "sdf/domain/interfaces/IPageCipher.hpp"
#include "sdf/domain/interfaces/IPageStorage.hpp"

namespace sdf::infrastructure
{

std::vector<std::uint8_t> ReadFirstPageRaw(const std::string& path);

class FileStorage final : public domain::IPageStorage
{
public:
    explicit FileStorage(const std::string& path);
    FileStorage(const std::string& path, std::shared_ptr<const domain::IPageCipher> cipher);

    std::size_t PageCount() const override;
    std::span<const std::uint8_t> PageBytes(std::size_t pageNumber) const override;

private:
    mutable std::ifstream _file;
    std::size_t _pageCount;
    std::shared_ptr<const domain::IPageCipher> _cipher;
    mutable std::vector<std::uint8_t> _pageBuffer;

    void Open(const std::string& path);
};

}

#endif
