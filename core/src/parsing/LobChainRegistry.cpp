#include "sdf/parsing/LobChainRegistry.hpp"

#include <algorithm>

#include "sdf/domain/PageLayout.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"

namespace sdf::parsing
{

namespace
{

std::uint32_t LogicalPageIdOf(std::span<const std::uint8_t> pageBytes)
{
    return infrastructure::ReadUInt32LE(pageBytes, 4) & 0xFFFFFu;
}

}

LobChainRegistry::LobChainRegistry(const domain::IPageStorage& storage) : _storage(&storage)
{
    const std::size_t pageCount = storage.PageCount();
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::span<const std::uint8_t> pageBytes = storage.PageBytes(pageNumber);
        const std::uint32_t logicalId = LogicalPageIdOf(pageBytes);
        _pageByLogicalId[logicalId] = PageEntry{pageNumber, pageBytes[6]};
    }
}

std::optional<std::size_t> LobChainRegistry::PhysicalPageOf(std::uint32_t logicalId) const
{
    const auto it = _pageByLogicalId.find(logicalId);
    if (it == _pageByLogicalId.end())
    {
        return std::nullopt;
    }
    return it->second.pageNumber;
}

std::vector<std::uint32_t> LobChainRegistry::ReadPackedSlots(
    std::span<const std::uint8_t> bytes, std::size_t offset, std::size_t maxSlots)
{
    std::vector<std::uint32_t> slots;
    slots.reserve(maxSlots);

    for (std::size_t slot = 0; slot < maxSlots; ++slot)
    {
        const std::size_t wordIndex = (domain::LvBitsPerSlot * slot) / (domain::LvSlotsPerWord * domain::LvBitsPerSlot);
        const std::size_t bitOffset = (domain::LvBitsPerSlot * slot) % (domain::LvSlotsPerWord * domain::LvBitsPerSlot);
        const std::size_t wordByteOffset = offset + wordIndex * domain::LvWordBytes;

        if (wordByteOffset + domain::LvWordBytes > bytes.size())
        {
            break;
        }

        const std::uint64_t word = infrastructure::ReadUInt64LE(bytes, wordByteOffset);
        const auto value = static_cast<std::uint32_t>((word >> bitOffset) & 0xFFFFFu);

        if (value == domain::LvNullSlot)
        {
            break;
        }

        slots.push_back(value);
    }

    return slots;
}

class LobChainRegistry::InlineLobSource final : public domain::ILazyLobSource
{
public:
    explicit InlineLobSource(std::vector<std::uint8_t> bytes) : _bytes(std::move(bytes))
    {
    }

    [[nodiscard]] std::size_t TotalLength() const override
    {
        return _bytes.size();
    }

    [[nodiscard]] std::size_t ChunkCount() const override
    {
        return _bytes.empty() ? 0 : 1;
    }

    [[nodiscard]] std::vector<std::uint8_t> ReadChunk(std::size_t) const override
    {
        return _bytes;
    }

private:
    std::vector<std::uint8_t> _bytes;
};

class LobChainRegistry::ChainLobSource final : public domain::ILazyLobSource
{
public:
    ChainLobSource(const domain::IPageStorage* storage, std::vector<std::size_t> pageNumbers, std::size_t totalLength)
        : _storage(storage), _pageNumbers(std::move(pageNumbers)), _totalLength(totalLength)
    {
    }

    [[nodiscard]] std::size_t TotalLength() const override
    {
        return _totalLength;
    }

    [[nodiscard]] std::size_t ChunkCount() const override
    {
        return _pageNumbers.size();
    }

    [[nodiscard]] std::vector<std::uint8_t> ReadChunk(std::size_t chunkIndex) const override
    {
        const std::span<const std::uint8_t> pageBytes = _storage->PageBytes(_pageNumbers[chunkIndex]);
        const std::span<const std::uint8_t> payload = pageBytes.subspan(domain::LobPageHeaderLength);
        const std::size_t consumedBefore = chunkIndex * (domain::PageSize - domain::LobPageHeaderLength);
        const std::size_t remaining = _totalLength > consumedBefore ? _totalLength - consumedBefore : 0;
        const std::size_t take = std::min(payload.size(), remaining);
        return std::vector<std::uint8_t>(payload.begin(), payload.begin() + take);
    }

private:
    const domain::IPageStorage* _storage;
    std::vector<std::size_t> _pageNumbers;
    std::size_t _totalLength;
};

std::shared_ptr<domain::ILazyLobSource> LobChainRegistry::ResolveLob(
    std::span<const std::uint8_t> inlineTail, std::size_t totalLength)
{
    if (totalLength <= domain::LvInlineThreshold)
    {
        const std::size_t take = std::min(inlineTail.size(), totalLength);
        return std::make_shared<InlineLobSource>(
            std::vector<std::uint8_t>(inlineTail.begin(), inlineTail.begin() + take));
    }

    constexpr std::size_t bytesPerPage = domain::PageSize - domain::LobPageHeaderLength;
    const std::size_t pagesNeeded = (totalLength + bytesPerPage - 1) / bytesPerPage;

    std::vector<std::uint32_t> logicalIds;

    if (totalLength <= domain::LvSingleLevelThreshold)
    {
        logicalIds = ReadPackedSlots(inlineTail, 0, std::min(pagesNeeded, domain::LvSlotsPerPage));
    }
    else
    {
        const std::vector<std::uint32_t> mapPageId = ReadPackedSlots(inlineTail, 0, 1);
        if (!mapPageId.empty())
        {
            if (const std::optional<std::size_t> mapPhysicalPage = PhysicalPageOf(mapPageId.front()))
            {
                const std::span<const std::uint8_t> mapPageBytes = _storage->PageBytes(*mapPhysicalPage);
                if (mapPageBytes.size() > 6 && mapPageBytes[6] == domain::LvMapPageType)
                {
                    logicalIds = ReadPackedSlots(
                        mapPageBytes, domain::LobPageHeaderLength,
                        std::min(pagesNeeded, domain::LvSlotsPerMapPage));
                }
            }
        }
    }

    std::vector<std::size_t> pageNumbers;
    pageNumbers.reserve(logicalIds.size());
    for (const std::uint32_t logicalId : logicalIds)
    {
        const std::optional<std::size_t> physicalPage = PhysicalPageOf(logicalId);
        if (!physicalPage)
        {
            break;
        }
        pageNumbers.push_back(*physicalPage);
    }

    if (pageNumbers.size() == pagesNeeded)
    {
        return std::make_shared<ChainLobSource>(_storage, std::move(pageNumbers), totalLength);
    }

    return std::make_shared<InlineLobSource>(std::vector<std::uint8_t>(inlineTail.begin(), inlineTail.end()));
}

}
