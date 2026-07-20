#include "sdf/infrastructure/Sha1KeyDerivation.hpp"

#include <array>
#include <vector>

#include "sdf/infrastructure/Sha1.hpp"

namespace sdf::infrastructure
{

namespace
{

constexpr std::size_t PadLength = 64;
constexpr std::uint8_t InnerPadByte = 0x36;
constexpr std::uint8_t OuterPadByte = 0x5C;

}

std::array<std::uint8_t, Sha1KeyDerivationLength> Sha1KeyDerivation(
    std::span<const std::uint8_t> baseHash, std::span<const std::uint8_t> extra)
{
    std::vector<std::uint8_t> hashInput;
    hashInput.reserve(baseHash.size() + extra.size());
    hashInput.insert(hashInput.end(), baseHash.begin(), baseHash.end());
    hashInput.insert(hashInput.end(), extra.begin(), extra.end());

    const std::array<std::uint8_t, 20> pageHash = Sha1(hashInput);

    std::array<std::uint8_t, PadLength> ipad{};
    std::array<std::uint8_t, PadLength> opad{};
    ipad.fill(InnerPadByte);
    opad.fill(OuterPadByte);
    for (std::size_t i = 0; i < pageHash.size(); ++i)
    {
        ipad[i] = static_cast<std::uint8_t>(ipad[i] ^ pageHash[i]);
        opad[i] = static_cast<std::uint8_t>(opad[i] ^ pageHash[i]);
    }

    const std::array<std::uint8_t, 20> innerDigest = Sha1(ipad);
    const std::array<std::uint8_t, 20> outerDigest = Sha1(opad);

    std::array<std::uint8_t, Sha1KeyDerivationLength> derived{};
    for (std::size_t i = 0; i < innerDigest.size(); ++i)
    {
        derived[i] = innerDigest[i];
    }
    for (std::size_t i = 0; i < outerDigest.size(); ++i)
    {
        derived[innerDigest.size() + i] = outerDigest[i];
    }

    return derived;
}

}
