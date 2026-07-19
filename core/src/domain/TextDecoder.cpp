#include "sdf/domain/TextDecoder.hpp"

namespace sdf::domain
{

namespace
{

std::uint16_t ReadUInt16LE(std::span<const std::uint8_t> chunk, std::size_t offset)
{
    return static_cast<std::uint16_t>(chunk[offset]) | static_cast<std::uint16_t>(chunk[offset + 1] << 8);
}

void AppendUtf8CodePoint(std::string& out, std::uint32_t codePoint)
{
    if (codePoint <= 0x7F)
    {
        out.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint <= 0x7FF)
    {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else if (codePoint <= 0xFFFF)
    {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

std::string DecodeLatin1(std::span<const std::uint8_t> chunk)
{
    std::string result;
    result.reserve(chunk.size());
    for (const std::uint8_t byte : chunk)
    {
        AppendUtf8CodePoint(result, byte);
    }
    return result;
}

std::string DecodeUtf16LE(std::span<const std::uint8_t> chunk)
{
    std::string result;
    result.reserve(chunk.size());

    const std::size_t unitCount = chunk.size() / 2;
    for (std::size_t i = 0; i < unitCount; ++i)
    {
        const std::uint16_t unit = ReadUInt16LE(chunk, 2 * i);

        if (unit >= 0xD800 && unit <= 0xDBFF && i + 1 < unitCount)
        {
            const std::uint16_t nextUnit = ReadUInt16LE(chunk, 2 * (i + 1));
            if (nextUnit >= 0xDC00 && nextUnit <= 0xDFFF)
            {
                const std::uint32_t codePoint
                    = 0x10000u + ((static_cast<std::uint32_t>(unit) - 0xD800u) << 10) + (nextUnit - 0xDC00u);
                AppendUtf8CodePoint(result, codePoint);
                ++i;
                continue;
            }
        }

        AppendUtf8CodePoint(result, unit);
    }

    return result;
}

}

std::string DecodeText(std::span<const std::uint8_t> chunk, bool compressed)
{
    if (compressed)
    {
        return DecodeLatin1(chunk);
    }
    return DecodeUtf16LE(chunk);
}

}
