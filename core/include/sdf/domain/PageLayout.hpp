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
constexpr std::size_t RowContinuationDwordOffset = 24;
constexpr std::size_t ContinuationPageNumberBias = 1022;

constexpr std::size_t PlaintextHeaderLength = 16;
constexpr std::size_t Page0ModeOffset = 184;
constexpr std::size_t Page0VerifierOffset = 76;
constexpr std::size_t Page0VerifierLength = 96;
constexpr std::size_t Page0KeyParamOffset = 188;
constexpr std::size_t Page0KeyParamLength = 4;
constexpr std::size_t MaxPasswordLength = 40;

}

#endif
