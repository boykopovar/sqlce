#include "sdf/infrastructure/AesCbc.hpp"

#include <stdexcept>

namespace sdf::infrastructure
{

namespace
{

constexpr std::array<std::uint8_t, 256> SBox = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9,
    0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f,
    0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07,
    0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3,
    0x29, 0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58,
    0xcf, 0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8, 0x51, 0xa3,
    0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec, 0x5f,
    0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac,
    0x62, 0x91, 0x95, 0xe4, 0x79, 0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a,
    0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70,
    0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11,
    0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42,
    0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

std::array<std::uint8_t, 256> BuildInverseSBox()
{
    std::array<std::uint8_t, 256> inverse{};
    for (std::size_t i = 0; i < SBox.size(); ++i)
    {
        inverse[SBox[i]] = static_cast<std::uint8_t>(i);
    }
    return inverse;
}

const std::array<std::uint8_t, 256> InverseSBox = BuildInverseSBox();

constexpr std::array<std::uint8_t, 11> RoundConstant = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

std::uint8_t GfMultiply(std::uint8_t a, std::uint8_t b)
{
    std::uint8_t product = 0;
    for (int bit = 0; bit < 8; ++bit)
    {
        if ((b & 0x01) != 0)
        {
            product ^= a;
        }
        const bool carry = (a & 0x80) != 0;
        a = static_cast<std::uint8_t>(a << 1);
        if (carry)
        {
            a ^= 0x1b;
        }
        b = static_cast<std::uint8_t>(b >> 1);
    }
    return product;
}

std::uint32_t SubWord(std::uint32_t word)
{
    return (static_cast<std::uint32_t>(SBox[(word >> 24) & 0xFF]) << 24)
        | (static_cast<std::uint32_t>(SBox[(word >> 16) & 0xFF]) << 16)
        | (static_cast<std::uint32_t>(SBox[(word >> 8) & 0xFF]) << 8) | static_cast<std::uint32_t>(SBox[word & 0xFF]);
}

std::uint32_t RotWord(std::uint32_t word)
{
    return (word << 8) | (word >> 24);
}

std::vector<std::uint32_t> ExpandKey(std::span<const std::uint8_t> key, std::size_t roundCount)
{
    const std::size_t keyWordCount = key.size() / 4;
    const std::size_t totalWordCount = 4 * (roundCount + 1);

    std::vector<std::uint32_t> words(totalWordCount);
    for (std::size_t i = 0; i < keyWordCount; ++i)
    {
        words[i] = (static_cast<std::uint32_t>(key[4 * i]) << 24) | (static_cast<std::uint32_t>(key[4 * i + 1]) << 16)
            | (static_cast<std::uint32_t>(key[4 * i + 2]) << 8) | static_cast<std::uint32_t>(key[4 * i + 3]);
    }

    for (std::size_t i = keyWordCount; i < totalWordCount; ++i)
    {
        std::uint32_t temp = words[i - 1];
        if (i % keyWordCount == 0)
        {
            temp = SubWord(RotWord(temp)) ^ (static_cast<std::uint32_t>(RoundConstant[i / keyWordCount]) << 24);
        }
        else if (keyWordCount > 6 && i % keyWordCount == 4)
        {
            temp = SubWord(temp);
        }
        words[i] = words[i - keyWordCount] ^ temp;
    }

    return words;
}

void AddRoundKey(std::uint8_t* state, const std::uint32_t* roundKey)
{
    for (std::size_t column = 0; column < 4; ++column)
    {
        state[column * 4 + 0] ^= static_cast<std::uint8_t>(roundKey[column] >> 24);
        state[column * 4 + 1] ^= static_cast<std::uint8_t>(roundKey[column] >> 16);
        state[column * 4 + 2] ^= static_cast<std::uint8_t>(roundKey[column] >> 8);
        state[column * 4 + 3] ^= static_cast<std::uint8_t>(roundKey[column]);
    }
}

void InverseSubBytes(std::uint8_t* state)
{
    for (std::size_t i = 0; i < 16; ++i)
    {
        state[i] = InverseSBox[state[i]];
    }
}

void InverseShiftRows(std::uint8_t* state)
{
    std::uint8_t temp[16];
    for (std::size_t i = 0; i < 16; ++i)
    {
        temp[i] = state[i];
    }

    for (std::size_t row = 1; row < 4; ++row)
    {
        for (std::size_t column = 0; column < 4; ++column)
        {
            const std::size_t sourceColumn = (column + 4 - row) % 4;
            state[column * 4 + row] = temp[sourceColumn * 4 + row];
        }
    }
}

void InverseMixColumns(std::uint8_t* state)
{
    for (std::size_t column = 0; column < 4; ++column)
    {
        std::uint8_t* cell = state + column * 4;
        const std::uint8_t a0 = cell[0];
        const std::uint8_t a1 = cell[1];
        const std::uint8_t a2 = cell[2];
        const std::uint8_t a3 = cell[3];

        cell[0] = static_cast<std::uint8_t>(GfMultiply(a0, 0x0e) ^ GfMultiply(a1, 0x0b) ^ GfMultiply(a2, 0x0d) ^ GfMultiply(a3, 0x09));
        cell[1] = static_cast<std::uint8_t>(GfMultiply(a0, 0x09) ^ GfMultiply(a1, 0x0e) ^ GfMultiply(a2, 0x0b) ^ GfMultiply(a3, 0x0d));
        cell[2] = static_cast<std::uint8_t>(GfMultiply(a0, 0x0d) ^ GfMultiply(a1, 0x09) ^ GfMultiply(a2, 0x0e) ^ GfMultiply(a3, 0x0b));
        cell[3] = static_cast<std::uint8_t>(GfMultiply(a0, 0x0b) ^ GfMultiply(a1, 0x0d) ^ GfMultiply(a2, 0x09) ^ GfMultiply(a3, 0x0e));
    }
}

}

AesCbcDecryptor::AesCbcDecryptor(std::span<const std::uint8_t> key, std::span<const std::uint8_t, 16> iv)
{
    if (key.size() != 16 && key.size() != 32)
    {
        throw std::invalid_argument("AesCbcDecryptor supports only 128-bit or 256-bit keys");
    }

    _roundCount = (key.size() == 16) ? 10 : 14;
    _roundKeys = ExpandKey(key, _roundCount);
    for (std::size_t i = 0; i < 16; ++i)
    {
        _iv[i] = iv[i];
    }
}

void AesCbcDecryptor::DecryptBlock(const std::uint8_t* in, std::uint8_t* out) const
{
    std::uint8_t state[16];
    for (std::size_t i = 0; i < 16; ++i)
    {
        state[i] = in[i];
    }

    AddRoundKey(state, &_roundKeys[_roundCount * 4]);

    for (std::size_t round = _roundCount - 1; round >= 1; --round)
    {
        InverseShiftRows(state);
        InverseSubBytes(state);
        AddRoundKey(state, &_roundKeys[round * 4]);
        InverseMixColumns(state);
    }

    InverseShiftRows(state);
    InverseSubBytes(state);
    AddRoundKey(state, &_roundKeys[0]);

    for (std::size_t i = 0; i < 16; ++i)
    {
        out[i] = state[i];
    }
}

std::vector<std::uint8_t> AesCbcDecryptor::Decrypt(std::span<const std::uint8_t> ciphertext) const
{
    if (ciphertext.size() % 16 != 0)
    {
        throw std::invalid_argument("AES-CBC ciphertext length must be a multiple of 16 bytes");
    }

    std::vector<std::uint8_t> plaintext(ciphertext.size());
    std::array<std::uint8_t, 16> previousBlock = _iv;

    for (std::size_t offset = 0; offset < ciphertext.size(); offset += 16)
    {
        std::uint8_t decrypted[16];
        DecryptBlock(ciphertext.data() + offset, decrypted);

        for (std::size_t i = 0; i < 16; ++i)
        {
            plaintext[offset + i] = static_cast<std::uint8_t>(decrypted[i] ^ previousBlock[i]);
        }

        for (std::size_t i = 0; i < 16; ++i)
        {
            previousBlock[i] = ciphertext[offset + i];
        }
    }

    return plaintext;
}

}
