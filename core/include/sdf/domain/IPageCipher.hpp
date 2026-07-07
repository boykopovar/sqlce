#ifndef SDF_DOMAIN_I_PAGE_CIPHER_HPP
#define SDF_DOMAIN_I_PAGE_CIPHER_HPP

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sdf::domain
{

class IPageCipher
{
public:
    virtual ~IPageCipher() = default;

    virtual bool VerifyPassword() const = 0;
    virtual std::vector<std::uint8_t> DecryptPage(std::size_t pageNumber, std::span<const std::uint8_t> page) const = 0;
};

}

#endif
