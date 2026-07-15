#include "sdf/parsing/LobChainRegistry.hpp"

#include <algorithm>
#include <optional>

#include "sdf/domain/PageLayout.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"
#include "sdf/infrastructure/PageView.hpp"

namespace sdf::parsing
{

namespace
{

bool IsReliableStreamIdPage(std::span<const std::uint8_t> pageBytes)
{
    const std::uint8_t id = pageBytes[16];
    return pageBytes[16] == id && pageBytes[17] == 0 && pageBytes[18] == id && pageBytes[19] == 0
        && pageBytes[20] == id && pageBytes[21] == 0 && pageBytes[22] == id && pageBytes[23] == 0;
}

std::uint32_t SequenceNumberOf(std::span<const std::uint8_t> pageBytes)
{
    return infrastructure::ReadUInt32LE(pageBytes, 8);
}

}

LobChainRegistry::LobChainRegistry(const domain::IPageStorage& storage) : _storage(&storage)
{
    std::map<std::uint8_t, std::vector<std::pair<std::uint32_t, std::size_t>>> reliableEntries;
    std::vector<std::size_t> unreliablePages;

    const std::size_t pageCount = storage.PageCount();
    for (std::size_t pageNumber = 0; pageNumber < pageCount; ++pageNumber)
    {
        const std::span<const std::uint8_t> pageBytes = storage.PageBytes(pageNumber);
        if (pageBytes[6] != domain::LobPageType)
        {
            continue;
        }

        if (IsReliableStreamIdPage(pageBytes))
        {
            const std::uint8_t streamId = pageBytes[16];
            reliableEntries[streamId].emplace_back(SequenceNumberOf(pageBytes), pageNumber);
        }
        else
        {
            unreliablePages.push_back(pageNumber);
        }
    }

    for (auto& [streamId, entries] : reliableEntries)
    {
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
        Chain chain;
        chain.pageNumbers.reserve(entries.size());
        for (const auto& [sequence, pageNumber] : entries)
        {
            chain.pageNumbers.push_back(pageNumber);
        }
        _chains.emplace(ChainKey(streamId), std::move(chain));
    }

    std::vector<std::vector<std::size_t>> runs;
    std::vector<std::size_t> currentRun;
    std::optional<std::size_t> previousPageNumber;

    for (const std::size_t pageNumber : unreliablePages)
    {
        if (previousPageNumber.has_value() && pageNumber != previousPageNumber.value() + 1)
        {
            runs.push_back(currentRun);
            currentRun.clear();
        }
        currentRun.push_back(pageNumber);
        previousPageNumber = pageNumber;
    }
    if (!currentRun.empty())
    {
        runs.push_back(currentRun);
    }

    const auto sequencesOf = [this](const std::vector<std::size_t>& run) {
        std::vector<std::uint32_t> sequences;
        sequences.reserve(run.size());
        for (const std::size_t pageNumber : run)
        {
            sequences.push_back(SequenceNumberOf(_storage->PageBytes(pageNumber)));
        }
        return sequences;
    };

    std::vector<std::vector<std::size_t>> mergedRuns;
    for (const std::vector<std::size_t>& run : runs)
    {
        const std::vector<std::uint32_t> sequences = sequencesOf(run);

        bool merged = false;
        if (!mergedRuns.empty() && mergedRuns.back().size() >= 2 && !run.empty())
        {
            const std::vector<std::uint32_t> previousSequences = sequencesOf(mergedRuns.back());
            const std::int64_t step = static_cast<std::int64_t>(previousSequences.back())
                - static_cast<std::int64_t>(previousSequences[previousSequences.size() - 2]);
            const std::int64_t gap
                = static_cast<std::int64_t>(sequences.front()) - static_cast<std::int64_t>(previousSequences.back());
            if (step > 0 && gap == step)
            {
                mergedRuns.back().insert(mergedRuns.back().end(), run.begin(), run.end());
                merged = true;
            }
        }

        if (!merged)
        {
            mergedRuns.push_back(run);
        }
    }

    for (const std::vector<std::size_t>& run : mergedRuns)
    {
        if (run.empty())
        {
            continue;
        }

        std::vector<std::pair<std::uint32_t, std::size_t>> entries;
        entries.reserve(run.size());
        for (const std::size_t pageNumber : run)
        {
            entries.emplace_back(SequenceNumberOf(storage.PageBytes(pageNumber)), pageNumber);
        }
        std::sort(entries.begin(), entries.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

        Chain chain;
        chain.pageNumbers.reserve(entries.size());
        for (const auto& [sequence, pageNumber] : entries)
        {
            chain.pageNumbers.push_back(pageNumber);
        }

        const std::string key = "unreliable-run-" + std::to_string(run.front());
        _chains.emplace(ChainKey(key), std::move(chain));
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

    for (auto& [key, chain] : _chains)
    {
        if (_usedChains.find(key) != _usedChains.end())
        {
            continue;
        }

        const std::size_t capacity = chain.pageNumbers.size() * (domain::PageSize - domain::LobPageHeaderLength);
        if (capacity >= totalLength)
        {
            _usedChains.insert(key);
            return std::make_shared<ChainLobSource>(_storage, chain.pageNumbers, totalLength);
        }
    }

    return std::make_shared<InlineLobSource>(std::vector<std::uint8_t>(inlineTail.begin(), inlineTail.end()));
}

}
