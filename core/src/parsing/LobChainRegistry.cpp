#include "sdf/parsing/LobChainRegistry.hpp"

#include <algorithm>

#include "sdf/domain/PageLayout.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"

namespace sdf::parsing
{

namespace
{

std::uint16_t LogicalPageIdOf(std::span<const std::uint8_t> pageBytes)
{
    return static_cast<std::uint16_t>(infrastructure::ReadUInt32LE(pageBytes, 4) & 0xFFFFu);
}

}

LobChainRegistry::LobChainRegistry(const domain::IPageStorage& storage) : _storage(&storage)
{
    const std::size_t pageCount = storage.PageCount();
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::span<const std::uint8_t> pageBytes = storage.PageBytes(pageNumber);
        const std::uint16_t logicalId = LogicalPageIdOf(pageBytes);
        _pageByLogicalId[logicalId] = PageEntry{pageNumber, pageBytes[6]};
    }
}

class LobChainRegistry::InlineLobSource final : public domain::ILazyLobSource
{
public:
    explicit InlineLobSource(std::vector<std::uint8_t> bytes) : _bytes(std::move(bytes))
    {
    }

    std::size_t TotalLength() const override
    {
        return _bytes.size();
    }

    std::size_t ChunkCount() const override
    {
        return _bytes.empty() ? 0 : 1;
    }

    std::vector<std::uint8_t> ReadChunk(std::size_t) const override
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

    std::size_t TotalLength() const override
    {
        return _totalLength;
    }

    std::size_t ChunkCount() const override
    {
        return _pageNumbers.size();
    }

    std::vector<std::uint8_t> ReadChunk(std::size_t chunkIndex) const override
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
    if (inlineTail.size() >= totalLength)
    {
        return std::make_shared<InlineLobSource>(
            std::vector<std::uint8_t>(inlineTail.begin(), inlineTail.begin() + totalLength));
    }

    const std::size_t bytesPerPage = domain::PageSize - domain::LobPageHeaderLength;
    const std::size_t pagesNeeded = (totalLength + bytesPerPage - 1) / bytesPerPage;

    std::vector<std::size_t> pageNumbers;

    if (inlineTail.size() >= 2 && pagesNeeded > 0)
    {
        pageNumbers.reserve(pagesNeeded);

        std::uint16_t logicalId = infrastructure::ReadUInt16LE(inlineTail, 0);
        std::size_t missCount = 0;
        constexpr std::size_t MaxConsecutiveMisses = 64;

        while (pageNumbers.size() < pagesNeeded && missCount < MaxConsecutiveMisses)
        {
            const auto it = _pageByLogicalId.find(logicalId);
            if (it != _pageByLogicalId.end() && it->second.pageType == domain::LobPageType)
            {
                pageNumbers.push_back(it->second.pageNumber);
                missCount = 0;
            }
            else
            {
                ++missCount;
            }
            ++logicalId;
        }
    }

    if (pageNumbers.size() == pagesNeeded)
    {
        return std::make_shared<ChainLobSource>(_storage, std::move(pageNumbers), totalLength);
    }

    return std::make_shared<InlineLobSource>(std::vector<std::uint8_t>(inlineTail.begin(), inlineTail.end()));
}

}
