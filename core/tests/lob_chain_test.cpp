#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <vector>

#include "sdf/domain/InvalidSdfException.hpp"
#include "sdf/domain/PageSize.hpp"
#include "sdf/parsing/LobChainRegistry.hpp"
#include "sdf/parsing/SdfFormat.hpp"

namespace
{

class MemoryStorage final : public sdf::domain::IPageStorage
{
public:
    std::array<std::vector<std::uint8_t>, 2> pages{
        std::vector<std::uint8_t>(sdf::domain::PageSize, 0x12),
        std::vector<std::uint8_t>(sdf::domain::PageSize, 0x34)};

    std::size_t PageCount() const override { return pages.size(); }
    std::span<const std::uint8_t> PageBytes(std::size_t pageNumber) const override
    {
        return pages.at(pageNumber);
    }
};

class IdentityResolver final : public sdf::parsing::ILogicalPageResolver
{
public:
    mutable std::size_t calls = 0;

    std::optional<std::size_t> ResolvePhysicalPage(
        const sdf::domain::IPageStorage& storage, std::uint32_t logicalPageId) const override
    {
        ++calls;
        if (logicalPageId < storage.PageCount())
        {
            return logicalPageId;
        }
        return std::nullopt;
    }
};

}

int main()
{
    using namespace sdf::parsing;
    using sdf::domain::InvalidSdfException;
    using sdf::domain::PageSize;

    MemoryStorage storage;
    auto resolver = std::make_shared<IdentityResolver>();
    LobChainRegistry registry(storage, resolver);
    const std::vector<std::uint8_t> inlineBytes(LvInlineThreshold, 0x56);
    auto empty = registry.ResolveLob({}, 0);
    if (empty->TotalLength() != 0 || empty->ChunkCount() != 0)
    {
        std::cerr << "empty LOB changed\n";
        return EXIT_FAILURE;
    }
    for (std::size_t length = 1; length <= LvInlineThreshold; ++length)
    {
        auto source = registry.ResolveLob(inlineBytes, length);
        if (source->TotalLength() != length || source->ChunkCount() != 1
            || source->ReadChunk(0) != std::vector<std::uint8_t>(length, 0x56))
        {
            std::cerr << "inline LOB changed\n";
            return EXIT_FAILURE;
        }
        try
        {
            (void)registry.ResolveLob(std::span(inlineBytes).first(length - 1), length);
            std::cerr << "truncated inline LOB was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const InvalidSdfException&)
        {
        }
    }

    // One packed word names pages 0, 1, then the end marker.
    const std::uint64_t packed = (std::uint64_t(1) << LvBitsPerSlot)
        | (std::uint64_t(LvNullSlot) << (2 * LvBitsPerSlot));
    std::array<std::uint8_t, LvWordBytes> ids{};
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        ids[i] = static_cast<std::uint8_t>(packed >> (8 * i));
    }
    constexpr std::size_t bytesPerPage = PageSize - LobPageHeaderLength;
    for (std::size_t length : {LvInlineThreshold + 1, bytesPerPage, bytesPerPage + 1, 2 * bytesPerPage})
    {
        auto source = registry.ResolveLob(ids, length);
        if (source->TotalLength() != length)
        {
            std::cerr << "chained LOB length changed\n";
            return EXIT_FAILURE;
        }
        std::vector<std::uint8_t> actual;
        for (std::size_t i = 0; i < source->ChunkCount(); ++i)
        {
            const auto chunk = source->ReadChunk(i);
            actual.insert(actual.end(), chunk.begin(), chunk.end());
        }
        std::vector<std::uint8_t> expected(std::min(length, bytesPerPage), 0x12);
        if (length > bytesPerPage)
        {
            expected.insert(expected.end(), length - bytesPerPage, 0x34);
        }
        if (actual != expected)
        {
            std::cerr << "chained LOB contents changed\n";
            return EXIT_FAILURE;
        }
        try
        {
            (void)source->ReadChunk(source->ChunkCount());
            std::cerr << "out-of-range chunk was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const std::out_of_range&)
        {
        }
        try
        {
            (void)source->ReadChunk(std::numeric_limits<std::size_t>::max());
            std::cerr << "overflowing chunk index was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const std::out_of_range&)
        {
        }
    }

    resolver->calls = 0;
    for (std::size_t length : {2 * bytesPerPage + 1, std::numeric_limits<std::size_t>::max()})
    {
        try
        {
            (void)registry.ResolveLob(ids, length);
            std::cerr << "impossible LOB length was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const InvalidSdfException&)
        {
        }
        if (resolver->calls != 0)
        {
            std::cerr << "impossible LOB length reached page resolution\n";
            return EXIT_FAILURE;
        }
    }
    try
    {
        (void)registry.ResolveLob({}, LvInlineThreshold + 1);
        std::cerr << "missing page chain was accepted\n";
        return EXIT_FAILURE;
    }
    catch (const InvalidSdfException&)
    {
    }

    auto source = registry.ResolveLob(ids, LvInlineThreshold + 1);
    for (std::size_t size : {std::size_t(0), LobPageHeaderLength - 1, PageSize - 1, PageSize + 1})
    {
        storage.pages[0].resize(size);
        try
        {
            (void)source->ReadChunk(0);
            std::cerr << "invalid LOB page size was accepted\n";
            return EXIT_FAILURE;
        }
        catch (const InvalidSdfException&)
        {
        }
    }
    return EXIT_SUCCESS;
}
