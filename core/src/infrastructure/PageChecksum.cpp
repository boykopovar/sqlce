#include "sdf/infrastructure/PageChecksum.hpp"

namespace sdf::infrastructure
{

namespace
{

constexpr std::uint32_t Modulus = 0xFFFFu;

std::uint16_t FoldTo16(std::uint32_t value)
{
    while (value > Modulus)
    {
        value = (value >> 16) + (value & Modulus);
    }
    if (value == Modulus)
    {
        value = 0;
    }
    return static_cast<std::uint16_t>(value);
}

std::uint16_t ReadWordLE(std::span<const std::uint8_t> data, std::size_t offset)
{
    return static_cast<std::uint16_t>(data[offset] | (static_cast<std::uint16_t>(data[offset + 1]) << 8));
}

}

std::uint32_t ComputePageChecksum(std::span<const std::uint8_t> page)
{
    std::uint32_t sum1 = 0;
    std::uint32_t sum2 = 0;

    const std::size_t wordCount = page.size() / 2;
    for (std::size_t i = 0; i < wordCount; ++i)
    {
        sum1 += ReadWordLE(page, i * 2);
        sum2 += sum1;

        if (sum1 > 0xFFFF'FFFFu - 0xFFFFu)
        {
            sum1 = FoldTo16(sum1);
        }
        if (sum2 > 0xFFFF'FFFFu - 0xFFFFu)
        {
            sum2 = FoldTo16(sum2);
        }
    }

    if ((page.size() & 1) != 0)
    {
        sum1 += page.back();
        sum2 += sum1;
    }

    const std::uint16_t folded1 = FoldTo16(sum1);
    const std::uint16_t folded2 = FoldTo16(sum2);

    return static_cast<std::uint32_t>(folded2) + Modulus * static_cast<std::uint32_t>(folded1);
}

}
