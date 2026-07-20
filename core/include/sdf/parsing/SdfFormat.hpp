#ifndef SDF_PARSING_SDF_FORMAT_HPP
#define SDF_PARSING_SDF_FORMAT_HPP

#include <cstddef>
#include <cstdint>

namespace sdf::parsing
{

constexpr std::uint8_t DataPageType = 0x40;
constexpr std::uint8_t LvMapPageType = 0x90;
constexpr std::size_t LobPageHeaderLength = 16;
constexpr std::size_t SlotArrayBaseIndex = 1017;
constexpr std::size_t RowHeaderLength = 28;
constexpr std::size_t RowContinuationDwordOffset = 24;
constexpr std::size_t ContinuationPageNumberBias = 1022;

constexpr std::size_t PageTypeOffset = 6;
constexpr std::size_t OwnerObjectIdOffset = 16;
constexpr std::uint8_t SystemCatalogObjectId = 3;
constexpr std::size_t SlotCountFieldOffset = 20;
constexpr std::uint32_t SlotCountFieldMask = 0xFFFu;
constexpr std::size_t SlotEntryLength = 4;
constexpr std::size_t SlotEntryFlagsOffset = 27;
constexpr std::size_t SlotEntryDwordOffset = 24;
constexpr std::uint8_t SlotFlagGhost = 0x01u;
constexpr std::uint8_t SlotFlagFirstFragment = 0x02u;
constexpr std::uint32_t RecordOffsetMask = 0xFFFu;
constexpr unsigned int RecordLengthFieldShift = 12;
constexpr std::uint32_t RecordLengthFieldMask = 0xFFFu;
constexpr std::size_t RecordLengthFieldBias = 4;
constexpr unsigned int ContinuationSlotIndexMask = 0xFFFu;
constexpr unsigned int ContinuationMarkerShift = 12;

constexpr std::size_t LogicalPageIdOffset = 4;
constexpr std::uint32_t LogicalPageIdMask = 0xFFFFFu;

constexpr std::size_t LvBitsPerSlot = 20;
constexpr std::size_t LvSlotsPerWord = 3;
constexpr std::size_t LvWordBytes = 8;
constexpr std::size_t LvInlineThreshold = 0xF8;
constexpr std::size_t LvSingleLevelThreshold = 0x5CA30;
constexpr std::size_t LvSlotsPerPage = 93;
constexpr std::size_t LvSlotsPerMapPage = 1527;
constexpr std::uint32_t LvNullSlot = 0xFFFFF;
constexpr std::uint64_t LvSlotValueMask = 0xFFFFFu;

constexpr std::size_t PlaintextHeaderLength = 16;
constexpr std::size_t Page0FormatVersionOffset = 16;
constexpr std::size_t Page0ModeOffset = 184;
constexpr std::size_t Page0VerifierOffset = 76;
constexpr std::size_t Page0VerifierLength = 96;
constexpr std::size_t Page0KeyParamOffset = 188;
constexpr std::size_t Page0KeyParamLength = 4;
constexpr std::size_t MaxPasswordLength = 40;

constexpr std::size_t EncryptedPageTypeWordOffset = 4;
constexpr unsigned int EncryptedPageTypeShift = 20;
constexpr std::uint32_t EncryptedPageTypeMask = 0xFu;
constexpr std::uint32_t EncryptedPageTypeMaxPlaintext = 2;

constexpr std::size_t VarDirectoryEntryLength = 2;
constexpr std::uint16_t VarDirectoryOffsetMask = 0x7FFFu;
constexpr std::uint16_t VarDirectoryCompressedFlag = 0x8000u;

constexpr std::size_t MaxRowContinuationHops = 64;

constexpr std::size_t CatalogRowHeaderLength = 4;
constexpr std::size_t CatalogBitRegionBytes = 2;
constexpr std::size_t CedbInfoOrdinalOffset = 2;

constexpr std::uint8_t SingleBitMask = 0x01u;

constexpr std::size_t RowInternalHeaderLength = 4;

constexpr std::size_t LobValueHeaderLength = 8;

}

#endif
