#include "sdf/infrastructure/Sha1.hpp"

#include <vector>

namespace sdf::infrastructure
{

namespace
{

std::uint32_t RotateLeft(std::uint32_t value, unsigned int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

std::vector<std::uint8_t> Pad(std::span<const std::uint8_t> message)
{
    std::vector<std::uint8_t> padded(message.begin(), message.end());
    const std::uint64_t bitLength = message.size() * 8;

    padded.push_back(0x80);
    while (padded.size() % 64 != 56)
    {
        padded.push_back(0x00);
    }

    for (int shift = 56; shift >= 0; shift -= 8)
    {
        padded.push_back(static_cast<std::uint8_t>(bitLength >> shift));
    }

    return padded;
}

}

std::array<std::uint8_t, 20> Sha1(std::span<const std::uint8_t> message)
{
    std::array<std::uint32_t, 5> state = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};

    const std::vector<std::uint8_t> padded = Pad(message);
    std::array<std::uint32_t, 80> schedule{};

    for (std::size_t chunkOffset = 0; chunkOffset < padded.size(); chunkOffset += 64)
    {
        for (std::size_t i = 0; i < 16; ++i)
        {
            const std::size_t byteOffset = chunkOffset + i * 4;
            schedule[i] = (static_cast<std::uint32_t>(padded[byteOffset]) << 24)
                | (static_cast<std::uint32_t>(padded[byteOffset + 1]) << 16)
                | (static_cast<std::uint32_t>(padded[byteOffset + 2]) << 8)
                | static_cast<std::uint32_t>(padded[byteOffset + 3]);
        }

        for (std::size_t i = 16; i < 80; ++i)
        {
            schedule[i] = RotateLeft(schedule[i - 3] ^ schedule[i - 8] ^ schedule[i - 14] ^ schedule[i - 16], 1);
        }

        std::uint32_t a = state[0];
        std::uint32_t b = state[1];
        std::uint32_t c = state[2];
        std::uint32_t d = state[3];
        std::uint32_t e = state[4];

        for (std::size_t i = 0; i < 80; ++i)
        {
            std::uint32_t f;
            std::uint32_t k;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            const std::uint32_t temp = RotateLeft(a, 5) + f + e + k + schedule[i];
            e = d;
            d = c;
            c = RotateLeft(b, 30);
            b = a;
            a = temp;
        }

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }

    std::array<std::uint8_t, 20> digest{};
    for (std::size_t i = 0; i < 5; ++i)
    {
        digest[i * 4] = static_cast<std::uint8_t>(state[i] >> 24);
        digest[i * 4 + 1] = static_cast<std::uint8_t>(state[i] >> 16);
        digest[i * 4 + 2] = static_cast<std::uint8_t>(state[i] >> 8);
        digest[i * 4 + 3] = static_cast<std::uint8_t>(state[i]);
    }

    return digest;
}

}
