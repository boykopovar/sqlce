#ifndef SDF_DOMAIN_ENCRYPTION_MODE_HPP
#define SDF_DOMAIN_ENCRYPTION_MODE_HPP

#include <cstdint>
#include <stdexcept>
#include <string>

namespace sdf::domain
{

enum class EncryptionMode : std::uint32_t
{
    None = 0,
    Rc4Sha1 = 1,
    Aes128Sha1 = 2,
    Aes128Sha256 = 3,
    Aes256Sha512 = 4
};

class UnsupportedEncryptionModeException final : public std::runtime_error
{
public:
    explicit UnsupportedEncryptionModeException(EncryptionMode mode)
        : std::runtime_error("unsupported .sdf encryption mode: " + std::to_string(static_cast<std::uint32_t>(mode)))
        , _mode(mode)
    {
    }

    EncryptionMode Mode() const
    {
        return _mode;
    }

private:
    EncryptionMode _mode;
};

class InvalidPasswordException final : public std::runtime_error
{
public:
    InvalidPasswordException() : std::runtime_error("password verification failed")
    {
    }
};

}

#endif
