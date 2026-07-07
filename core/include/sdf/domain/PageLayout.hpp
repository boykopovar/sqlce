#ifndef SDF_DOMAIN_PAGE_LAYOUT_HPP
#define SDF_DOMAIN_PAGE_LAYOUT_HPP

#include <cstddef>
#include <cstdint>

namespace sdf::domain
{

constexpr std::size_t PageSize = 4096;
constexpr std::uint8_t DataPageType = 0x40;
constexpr std::uint8_t LobPageType = 0x50;
constexpr std::size_t LobPageHeaderLength = 16;
constexpr std::size_t SlotArrayBaseIndex = 1017;
constexpr std::size_t RowHeaderLength = 28;

}

#endif
