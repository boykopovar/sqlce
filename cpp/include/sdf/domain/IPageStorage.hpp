#ifndef SDF_DOMAIN_I_PAGE_STORAGE_HPP
#define SDF_DOMAIN_I_PAGE_STORAGE_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace sdf::domain
{

class IPageStorage
{
public:
    virtual ~IPageStorage() = default;

    virtual std::size_t PageCount() const = 0;
    virtual std::span<const std::uint8_t> PageBytes(std::size_t pageNumber) const = 0;
};

}

#endif
