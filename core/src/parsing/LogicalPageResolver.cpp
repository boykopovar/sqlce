#include "sdf/parsing/LogicalPageResolver.hpp"

#include "sdf/domain/InvalidSdfException.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace sdf::parsing
{

namespace
{

std::uint32_t LogicalPageIdOf(std::span<const std::uint8_t> pageBytes)
{
    return infrastructure::ReadUInt32LE(pageBytes, LogicalPageIdOffset) & LogicalPageIdMask;
}

bool PageMatches(std::span<const std::uint8_t> pageBytes, std::uint8_t expectedType, std::uint32_t expectedLogicalId)
{
    return pageBytes.size() > PageTypeOffset
        && pageBytes[PageTypeOffset] == expectedType
        && LogicalPageIdOf(pageBytes) == expectedLogicalId;
}

std::uint32_t ReadPackedSlot(std::span<const std::uint8_t> pageBytes, std::size_t baseOffset, std::size_t slotIndex)
{
    const std::size_t wordIndex = (LvBitsPerSlot * slotIndex) / (LvSlotsPerWord * LvBitsPerSlot);
    const std::size_t bitOffset = (LvBitsPerSlot * slotIndex) % (LvSlotsPerWord * LvBitsPerSlot);
    const std::size_t wordByteOffset = baseOffset + wordIndex * LvWordBytes;

    if (wordByteOffset + LvWordBytes > pageBytes.size())
    {
        return 0;
    }

    const std::uint64_t word = infrastructure::ReadUInt64LE(pageBytes, wordByteOffset);
    return static_cast<std::uint32_t>((word >> bitOffset) & LvSlotValueMask);
}

}

const LogicalPageResolver::LogicalPageMap& LogicalPageResolver::_mapFor(const domain::IPageStorage& storage) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_cachedStorage == &storage)
    {
        return _cachedMap;
    }

    LogicalPageMap map;

    const std::size_t pageCount = storage.PageCount();

    std::optional<std::size_t> mapAPhysicalPage;
    if (pageCount > 0)
    {
        const std::span<const std::uint8_t> headerBytes = storage.PageBytes(0);
        if (headerBytes.size() >= HeaderMapAPhysicalPageOffset + sizeof(std::uint32_t))
        {
            const std::uint32_t candidate = infrastructure::ReadUInt32LE(headerBytes, HeaderMapAPhysicalPageOffset);
            if (candidate < pageCount)
            {
                mapAPhysicalPage = candidate;
            }
        }
    }
    if (mapAPhysicalPage.has_value())
    {
        const std::span<const std::uint8_t> mapABytes = storage.PageBytes(*mapAPhysicalPage);
        if (!PageMatches(mapABytes, MapAPageType, MapAPageLogicalId))
        {
            throw domain::InvalidSdfException("header page Map A pointer does not reference a valid Map A page");
        }

        map.physicalPageByLogicalId[MapAPageLogicalId] = *mapAPhysicalPage;

        auto decodeMapBRange = [&](std::uint32_t mapBPhysicalPage, std::uint32_t rangeStart)
        {
            const std::span<const std::uint8_t> mapBBytes = storage.PageBytes(mapBPhysicalPage);
            for (std::size_t mapBSlotIndex = 0; mapBSlotIndex < MapBRangeLogicalIdSpan; ++mapBSlotIndex)
            {
                const std::uint32_t physicalPage = ReadPackedSlot(mapBBytes, MapBPageDataOffset, mapBSlotIndex);
                if (physicalPage == 0 || physicalPage >= pageCount)
                {
                    continue;
                }
                map.physicalPageByLogicalId[rangeStart + static_cast<std::uint32_t>(mapBSlotIndex)] = physicalPage;
            }
        };

        const std::uint32_t mapBSlotZero = ReadPackedSlot(mapABytes, MapAPageDataOffset, 0);
        if (mapBSlotZero == 0 || mapBSlotZero >= pageCount
            || !PageMatches(storage.PageBytes(mapBSlotZero), MapBPageType, MapBPageLogicalId))
        {
            throw domain::InvalidSdfException("Map A slot 0 does not reference a valid Map B page");
        }

        map.physicalPageByLogicalId[MapBPageLogicalId] = mapBSlotZero;
        decodeMapBRange(mapBSlotZero, MapBRangeFirstLogicalId);

        std::uint32_t mapBRangeStart = MapBRangeFirstLogicalId + static_cast<std::uint32_t>(MapBRangeLogicalIdSpan);
        for (std::size_t mapASlotIndex = 1;; ++mapASlotIndex)
        {
            const std::size_t wordIndex = (LvBitsPerSlot * mapASlotIndex) / (LvSlotsPerWord * LvBitsPerSlot);
            const std::size_t wordByteOffset = MapAPageDataOffset + wordIndex * LvWordBytes;
            if (wordByteOffset + LvWordBytes > mapABytes.size())
            {
                break;
            }

            const std::uint32_t mapBPhysicalPage = ReadPackedSlot(mapABytes, MapAPageDataOffset, mapASlotIndex);
            if (mapBPhysicalPage != 0 && mapBPhysicalPage < pageCount)
            {
                decodeMapBRange(mapBPhysicalPage, mapBRangeStart);
            }

            mapBRangeStart += static_cast<std::uint32_t>(MapBRangeLogicalIdSpan);
        }
    }

    _cachedMap = std::move(map);
    _cachedStorage = &storage;
    return _cachedMap;
}

std::optional<std::size_t> LogicalPageResolver::ResolvePhysicalPage(
    const domain::IPageStorage& storage, std::uint32_t logicalPageId) const
{
    const LogicalPageMap& map = _mapFor(storage);
    const auto it = map.physicalPageByLogicalId.find(logicalPageId);
    if (it == map.physicalPageByLogicalId.end())
    {
        return std::nullopt;
    }
    return it->second;
}

}
