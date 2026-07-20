#include "sdf/infrastructure/TripleDesCbc.hpp"

#include <algorithm>
#include <stdexcept>

namespace sdf::infrastructure
{

namespace
{

constexpr std::array<int, 64> InitialPermutation = {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4, 62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8, 57, 49, 41, 33, 25, 17, 9,  1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};

constexpr std::array<int, 64> FinalPermutation = {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31, 38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29, 36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9,  49, 17, 57, 25};

constexpr std::array<int, 48> Expansion = {
    32, 1,  2,  3,  4,  5,  4,  5,  6,  7,  8,  9,  8,  9,  10, 11, 12, 13, 12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21, 20, 21, 22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};

constexpr std::array<int, 32> Permutation = {
    16, 7, 20, 21, 29, 12, 28, 17, 1, 15, 23, 26, 5,  18, 31, 10,
    2,  8, 24, 14, 32, 27, 3,  9,  19, 13, 30, 6,  22, 11, 4,  25};

constexpr std::array<int, 56> Pc1 = {
    57, 49, 41, 33, 25, 17, 9,  1,  58, 50, 42, 34, 26, 18, 10, 2,  59, 51, 43, 35, 27, 19, 11, 3,
    60, 52, 44, 36, 63, 55, 47, 39, 31, 23, 15, 7,  62, 54, 46, 38, 30, 22, 14, 6,  61, 53, 45, 37,
    29, 21, 13, 5,  28, 20, 12, 4};

constexpr std::array<int, 48> Pc2 = {
    14, 17, 11, 24, 1,  5,  3,  28, 15, 6,  21, 10, 23, 19, 12, 4,  26, 8,  16, 7,  27, 20, 13, 2,
    41, 52, 31, 37, 47, 55, 30, 40, 51, 45, 33, 48, 44, 49, 39, 56, 34, 53, 46, 42, 50, 36, 29, 32};

constexpr std::array<int, 16> RoundShifts = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

constexpr std::array<int, 512> SBoxes = {
    14, 4,  13, 1,  2,  15, 11, 8,  3,  10, 6,  12, 5,  9,  0,  7,  0,  15, 7,  4,  14, 2,  13, 1,
    10, 6,  12, 11, 9,  5,  3,  8,  4,  1,  14, 8,  13, 6,  2,  11, 15, 12, 9,  7,  3,  10, 5,  0,
    15, 12, 8,  2,  4,  9,  1,  7,  5,  11, 3,  14, 10, 0,  6,  13,

    15, 1,  8,  14, 6,  11, 3,  4,  9,  7,  2,  13, 12, 0,  5,  10, 3,  13, 4,  7,  15, 2,  8,  14,
    12, 0,  1,  10, 6,  9,  11, 5,  0,  14, 7,  11, 10, 4,  13, 1,  5,  8,  12, 6,  9,  3,  2,  15,
    13, 8,  10, 1,  3,  15, 4,  2,  11, 6,  7,  12, 0,  5,  14, 9,

    10, 0,  9,  14, 6,  3,  15, 5,  1,  13, 12, 7,  11, 4,  2,  8,  13, 7,  0,  9,  3,  4,  6,  10,
    2,  8,  5,  14, 12, 11, 15, 1,  13, 6,  4,  9,  8,  15, 3,  0,  11, 1,  2,  12, 5,  10, 14, 7,
    1,  10, 13, 0,  6,  9,  8,  7,  4,  15, 14, 3,  11, 5,  2,  12,

    7,  13, 14, 3,  0,  6,  9,  10, 1,  2,  8,  5,  11, 12, 4,  15, 13, 8,  11, 5,  6,  15, 0,  3,
    4,  7,  2,  12, 1,  10, 14, 9,  10, 6,  9,  0,  12, 11, 7,  13, 15, 1,  3,  14, 5,  2,  8,  4,
    3,  15, 0,  6,  10, 1,  13, 8,  9,  4,  5,  11, 12, 7,  2,  14,

    2,  12, 4,  1,  7,  10, 11, 6,  8,  5,  3,  15, 13, 0,  14, 9,  14, 11, 2,  12, 4,  7,  13, 1,
    5,  0,  15, 10, 3,  9,  8,  6,  4,  2,  1,  11, 10, 13, 7,  8,  15, 9,  12, 5,  6,  3,  0,  14,
    11, 8,  12, 7,  1,  14, 2,  13, 6,  15, 0,  9,  10, 4,  5,  3,

    12, 1,  10, 15, 9,  2,  6,  8,  0,  13, 3,  4,  14, 7,  5,  11, 10, 15, 4,  2,  7,  12, 9,  5,
    6,  1,  13, 14, 0,  11, 3,  8,  9,  14, 15, 5,  2,  8,  12, 3,  7,  0,  4,  10, 1,  13, 11, 6,
    4,  3,  2,  12, 9,  5,  15, 10, 11, 14, 1,  7,  6,  0,  8,  13,

    4,  11, 2,  14, 15, 0,  8,  13, 3,  12, 9,  7,  5,  10, 6,  1,  13, 0,  11, 7,  4,  9,  1,  10,
    14, 3,  5,  12, 2,  15, 8,  6,  1,  4,  11, 13, 12, 3,  7,  14, 10, 15, 6,  8,  0,  5,  9,  2,
    6,  11, 13, 8,  1,  4,  10, 7,  9,  5,  0,  15, 14, 2,  3,  12,

    13, 2,  8,  4,  6,  15, 11, 1,  10, 9,  3,  14, 5,  0,  12, 7,  1,  15, 13, 8,  10, 3,  7,  4,
    12, 5,  6,  11, 0,  14, 9,  2,  7,  11, 4,  1,  9,  12, 14, 2,  0,  6,  10, 13, 15, 3,  5,  8,
    2,  1,  14, 7,  4,  10, 8,  13, 15, 12, 9,  0,  3,  5,  6,  11};

using Bits64 = std::array<int, 64>;
using Bits56 = std::array<int, 56>;
using Bits48 = std::array<int, 48>;
using Bits32 = std::array<int, 32>;
using SubKeys = std::array<Bits48, 16>;

Bits64 BytesToBits(const std::uint8_t* bytes, std::size_t byteCount)
{
    Bits64 bits{};
    for (std::size_t i = 0; i < byteCount; ++i)
    {
        for (int bit = 0; bit < 8; ++bit)
        {
            bits[i * 8 + bit] = (bytes[i] >> (7 - bit)) & 1;
        }
    }
    return bits;
}

void BitsToBytes(const int* bits, std::size_t bitCount, std::uint8_t* out)
{
    for (std::size_t i = 0; i < bitCount / 8; ++i)
    {
        std::uint8_t value = 0;
        for (int bit = 0; bit < 8; ++bit)
        {
            value = static_cast<std::uint8_t>((value << 1) | bits[i * 8 + bit]);
        }
        out[i] = value;
    }
}

template <std::size_t OutSize, typename InputArray, std::size_t TableSize>
std::array<int, OutSize> Permute(const InputArray& input, const std::array<int, TableSize>& table)
{
    static_assert(TableSize == OutSize);
    std::array<int, OutSize> output{};
    for (std::size_t i = 0; i < OutSize; ++i)
    {
        output[i] = input[static_cast<std::size_t>(table[i]) - 1];
    }
    return output;
}

template <std::size_t Size>
std::array<int, Size> LeftRotate(const std::array<int, Size>& bits, int amount)
{
    std::array<int, Size> rotated{};
    for (std::size_t i = 0; i < Size; ++i)
    {
        rotated[i] = bits[(i + static_cast<std::size_t>(amount)) % Size];
    }
    return rotated;
}

template <std::size_t Size>
std::array<int, Size> Xor(const std::array<int, Size>& a, const std::array<int, Size>& b)
{
    std::array<int, Size> result{};
    for (std::size_t i = 0; i < Size; ++i)
    {
        result[i] = a[i] ^ b[i];
    }
    return result;
}

SubKeys GenerateSubKeys(const std::uint8_t* key8)
{
    const Bits64 keyBits = BytesToBits(key8, DesBlockLength);
    const Bits56 key56 = Permute<56>(keyBits, Pc1);

    std::array<int, 28> c{};
    std::array<int, 28> d{};
    for (std::size_t i = 0; i < 28; ++i)
    {
        c[i] = key56[i];
        d[i] = key56[i + 28];
    }

    SubKeys subKeys{};
    for (std::size_t round = 0; round < 16; ++round)
    {
        c = LeftRotate(c, RoundShifts[round]);
        d = LeftRotate(d, RoundShifts[round]);

        Bits56 cd{};
        for (std::size_t i = 0; i < 28; ++i)
        {
            cd[i] = c[i];
            cd[i + 28] = d[i];
        }
        subKeys[round] = Permute<48>(cd, Pc2);
    }
    return subKeys;
}

Bits32 FeistelFunction(const Bits32& r, const Bits48& subKey)
{
    const Bits48 expanded = Permute<48>(r, Expansion);
    const Bits48 x = Xor(expanded, subKey);

    std::array<int, 32> out{};
    for (std::size_t box = 0; box < 8; ++box)
    {
        const int b0 = x[box * 6 + 0];
        const int b1 = x[box * 6 + 1];
        const int b2 = x[box * 6 + 2];
        const int b3 = x[box * 6 + 3];
        const int b4 = x[box * 6 + 4];
        const int b5 = x[box * 6 + 5];

        const int row = (b0 << 1) | b5;
        const int col = (b1 << 3) | (b2 << 2) | (b3 << 1) | b4;
        const int value = SBoxes[box * 64 + static_cast<std::size_t>(row) * 16 + static_cast<std::size_t>(col)];

        out[box * 4 + 0] = (value >> 3) & 1;
        out[box * 4 + 1] = (value >> 2) & 1;
        out[box * 4 + 2] = (value >> 1) & 1;
        out[box * 4 + 3] = value & 1;
    }
    return Permute<32>(out, Permutation);
}

void DesCryptBlock(const std::uint8_t* block8, const std::uint8_t* key8, bool decrypt, std::uint8_t* out8)
{
    SubKeys subKeys = GenerateSubKeys(key8);
    if (decrypt)
    {
        std::reverse(subKeys.begin(), subKeys.end());
    }

    const Bits64 permuted = Permute<64>(BytesToBits(block8, DesBlockLength), InitialPermutation);

    Bits32 l{};
    Bits32 r{};
    for (std::size_t i = 0; i < 32; ++i)
    {
        l[i] = permuted[i];
        r[i] = permuted[i + 32];
    }

    for (const Bits48& subKey : subKeys)
    {
        const Bits32 newR = Xor(l, FeistelFunction(r, subKey));
        l = r;
        r = newR;
    }

    Bits64 preOutput{};
    for (std::size_t i = 0; i < 32; ++i)
    {
        preOutput[i] = r[i];
        preOutput[i + 32] = l[i];
    }

    const Bits64 outBits = Permute<64>(preOutput, FinalPermutation);
    BitsToBytes(outBits.data(), 64, out8);
}

}

TripleDesCbcDecryptor::TripleDesCbcDecryptor(
    std::span<const std::uint8_t> key, std::span<const std::uint8_t, DesBlockLength> iv)
{
    if (key.size() != TripleDesKeyLength)
    {
        throw std::invalid_argument("TripleDesCbcDecryptor requires a 24-byte (168-bit) key");
    }

    for (std::size_t i = 0; i < TripleDesKeyLength; ++i)
    {
        _key[i] = key[i];
    }
    for (std::size_t i = 0; i < DesBlockLength; ++i)
    {
        _iv[i] = iv[i];
    }
}

void TripleDesCbcDecryptor::DecryptBlock(const std::uint8_t* in, std::uint8_t* out) const
{
    std::uint8_t step1[DesBlockLength];
    std::uint8_t step2[DesBlockLength];

    DesCryptBlock(in, _key.data() + 16, /*decrypt=*/true, step1);
    DesCryptBlock(step1, _key.data() + 8, /*decrypt=*/false, step2);
    DesCryptBlock(step2, _key.data(), /*decrypt=*/true, out);
}

std::vector<std::uint8_t> TripleDesCbcDecryptor::Decrypt(std::span<const std::uint8_t> ciphertext) const
{
    if (ciphertext.size() % DesBlockLength != 0)
    {
        throw std::invalid_argument("3DES-CBC ciphertext length must be a multiple of 8 bytes");
    }

    std::vector<std::uint8_t> plaintext(ciphertext.size());
    std::array<std::uint8_t, DesBlockLength> previousBlock = _iv;

    for (std::size_t offset = 0; offset < ciphertext.size(); offset += DesBlockLength)
    {
        std::uint8_t decrypted[DesBlockLength];
        DecryptBlock(ciphertext.data() + offset, decrypted);

        for (std::size_t i = 0; i < DesBlockLength; ++i)
        {
            plaintext[offset + i] = static_cast<std::uint8_t>(decrypted[i] ^ previousBlock[i]);
        }

        for (std::size_t i = 0; i < DesBlockLength; ++i)
        {
            previousBlock[i] = ciphertext[offset + i];
        }
    }

    return plaintext;
}

}
