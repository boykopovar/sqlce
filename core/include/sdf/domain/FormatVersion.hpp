#ifndef SDF_DOMAIN_FORMAT_VERSION_HPP
#define SDF_DOMAIN_FORMAT_VERSION_HPP

#include <cstdint>

namespace sdf::domain
{

enum class FormatVersion : std::uint32_t
{
    Unknown = 0,
    SqlCe30 = 3004180,
    SqlCe35 = 3505053,
    SqlCe35Sp2 = 3505625,
    SqlCe40 = 4000000
};

}

#endif
