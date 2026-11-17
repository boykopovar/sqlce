#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <span>

#include "sdf/domain/InvalidSdfException.hpp"
#include "sdf/infrastructure/BinaryReader.hpp"

namespace
{

template <typename TReader>
bool CheckBounds(TReader read, std::size_t width)
{
    using sdf::domain::InvalidSdfException;
    const std::array<std::uint8_t, 32> bytes{};
    const std::span<const std::uint8_t> buffer(bytes);

    (void)read(buffer, 0);
    (void)read(buffer, buffer.size() - width);
    for (std::size_t available = 0; available < width; ++available)
    {
        try
        {
            (void)read(buffer.first(available), 0);
            std::cerr << "short field was accepted\n";
            return false;
        }
        catch (const InvalidSdfException&)
        {
        }
        try
        {
            (void)read(buffer, buffer.size() - available);
            std::cerr << "field crossing the end was accepted\n";
            return false;
        }
        catch (const InvalidSdfException&)
        {
        }
    }
    try
    {
        (void)read(buffer, buffer.size() + 1);
        std::cerr << "offset beyond the buffer was accepted\n";
        return false;
    }
    catch (const InvalidSdfException&)
    {
    }
    try
    {
        (void)read(buffer, std::numeric_limits<std::size_t>::max());
        std::cerr << "overflowing offset was accepted\n";
        return false;
    }
    catch (const InvalidSdfException&)
    {
    }
    return true;
}

}

int main()
{
    using namespace sdf::infrastructure;
    if (!CheckBounds(ReadUInt16LE, 2)
        || !CheckBounds(ReadUInt32LE, 4)
        || !CheckBounds(ReadInt32LE, 4)
        || !CheckBounds(ReadUInt64LE, 8)
        || !CheckBounds(ReadInt64LE, 8)
        || !CheckBounds(ReadFloatLE, 4)
        || !CheckBounds(ReadDoubleLE, 8)
        || !CheckBounds(ReadBytes16, 16))
    {
        return EXIT_FAILURE;
    }

    const std::array<std::uint8_t, 17> bytes{
        0, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    if (ReadUInt16LE(bytes, 1) != 0x2301)
    {
        std::cerr << "16-bit value changed\n";
        return EXIT_FAILURE;
    }
    if (ReadUInt32LE(bytes, 1) != 0x67452301)
    {
        std::cerr << "32-bit value changed\n";
        return EXIT_FAILURE;
    }
    if (ReadUInt64LE(bytes, 1) != 0xefcdab8967452301ULL)
    {
        std::cerr << "64-bit value changed\n";
        return EXIT_FAILURE;
    }
    if (ReadInt32LE(bytes, 9) != -1)
    {
        std::cerr << "signed 32-bit value changed\n";
        return EXIT_FAILURE;
    }
    if (ReadInt64LE(bytes, 9) != -1)
    {
        std::cerr << "signed 64-bit value changed\n";
        return EXIT_FAILURE;
    }
    const auto copied = ReadBytes16(bytes, 1);
    for (std::size_t i = 0; i < copied.size(); ++i)
    {
        if (copied[i] != bytes[i + 1])
        {
            std::cerr << "16-byte value changed\n";
            return EXIT_FAILURE;
        }
    }
    const std::array<std::uint8_t, 4> oneFloat{0, 0, 0x80, 0x3f};
    const std::array<std::uint8_t, 8> oneDouble{0, 0, 0, 0, 0, 0, 0xf0, 0x3f};
    if (ReadFloatLE(oneFloat, 0) != 1.0f)
    {
        std::cerr << "float value changed\n";
        return EXIT_FAILURE;
    }
    if (ReadDoubleLE(oneDouble, 0) != 1.0)
    {
        std::cerr << "double value changed\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
