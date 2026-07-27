#include "sdf/parsing/LogicalPageResolver.hpp"

#include <algorithm>
#include <unordered_map>
#include <vector>

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

std::uint8_t GenerationOf(std::span<const std::uint8_t> pageBytes)
{
    return pageBytes[OwnerObjectGenerationOffset];
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

    std::unordered_map<std::uint32_t, std::vector<std::size_t>> physicalPagesByLogicalId;
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::uint32_t logicalId = LogicalPageIdOf(storage.PageBytes(pageNumber));
        physicalPagesByLogicalId[logicalId].push_back(pageNumber);
    }

    auto pickByGeneration = [&](std::uint32_t logicalId) -> std::optional<std::size_t>
    {
        const auto it = physicalPagesByLogicalId.find(logicalId);
        if (it == physicalPagesByLogicalId.end() || it->second.empty())
        {
            return std::nullopt;
        }

        std::size_t best = it->second.front();
        for (const std::size_t candidate : it->second)
        {
            if (GenerationOf(storage.PageBytes(candidate)) >= GenerationOf(storage.PageBytes(best)))
            {
                best = candidate;
            }
        }
        return best;
    };

    for (const auto& [logicalId, physicalPages] : physicalPagesByLogicalId)
    {
        if (const std::optional<std::size_t> chosen = pickByGeneration(logicalId))
        {
            map.physicalPageByLogicalId[logicalId] = *chosen;
        }
    }

    const std::optional<std::size_t> mapAPhysicalPage = pickByGeneration(MapAPageLogicalId);
    if (mapAPhysicalPage.has_value())
    {
        const std::span<const std::uint8_t> mapABytes = storage.PageBytes(*mapAPhysicalPage);
        if (mapABytes.size() > PageTypeOffset && mapABytes[PageTypeOffset] == MapAPageType)
        {
            std::uint32_t mapBRangeStart = MapBRangeFirstLogicalId;
            for (std::size_t mapASlotIndex = 2;; ++mapASlotIndex)
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
                    const std::span<const std::uint8_t> mapBBytes = storage.PageBytes(mapBPhysicalPage);
                    for (std::size_t mapBSlotIndex = 0; mapBSlotIndex < MapBRangeLogicalIdSpan; ++mapBSlotIndex)
                    {
                        const std::uint32_t physicalPage = ReadPackedSlot(mapBBytes, MapBPageDataOffset, mapBSlotIndex);
                        if (physicalPage == 0 || physicalPage >= pageCount)
                        {
                            continue;
                        }
                        map.physicalPageByLogicalId[mapBRangeStart + static_cast<std::uint32_t>(mapBSlotIndex)]
                            = physicalPage;
                    }
                }

                mapBRangeStart += static_cast<std::uint32_t>(MapBRangeLogicalIdSpan);
            }
        }
    }

    for (const auto& [logicalId, physicalPage] : map.physicalPageByLogicalId)
    {
        (void)logicalId;
        map.currentPhysicalPages.insert(physicalPage);
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

std::set<std::size_t> LogicalPageResolver::CurrentPhysicalPages(const domain::IPageStorage& storage) const
{
    return _mapFor(storage).currentPhysicalPages;
}

}
