#include "sdf/domain/NumericValue.hpp"

#include <algorithm>

namespace sdf::domain
{

static std::string ToDecimalDigits(std::array<std::uint8_t, 16> littleEndianMagnitude)
{
    std::string digits;
    bool allZero = std::all_of(
        littleEndianMagnitude.begin(), littleEndianMagnitude.end(), [](std::uint8_t b) { return b == 0; });
    if (allZero)
    {
        return "0";
    }

    while (!std::all_of(
        littleEndianMagnitude.begin(), littleEndianMagnitude.end(), [](std::uint8_t b) { return b == 0; }))
    {
        std::uint32_t remainder = 0;
        for (std::size_t i = littleEndianMagnitude.size(); i-- > 0;)
        {
            const std::uint32_t current = (remainder << 8) | littleEndianMagnitude[i];
            littleEndianMagnitude[i] = static_cast<std::uint8_t>(current / 10);
            remainder = current % 10;
        }
        digits.push_back(static_cast<char>('0' + remainder));
    }

    std::reverse(digits.begin(), digits.end());
    return digits;
}

NumericValue::NumericValue() : _precision(0), _scale(0), _isPositive(true), _unscaledLittleEndian{}
{
}

NumericValue::NumericValue(
    std::uint8_t precision,
    std::uint8_t scale,
    bool isPositive,
    const std::array<std::uint8_t, 16>& unscaledLittleEndian)
    : _precision(precision)
    , _scale(scale)
    , _isPositive(isPositive)
    , _unscaledLittleEndian(unscaledLittleEndian)
{
}

std::uint8_t NumericValue::Precision() const
{
    return _precision;
}

std::uint8_t NumericValue::Scale() const
{
    return _scale;
}

bool NumericValue::IsPositive() const
{
    return _isPositive;
}

std::string NumericValue::UnscaledDigits() const
{
    return ToDecimalDigits(_unscaledLittleEndian);
}

std::string NumericValue::ToString() const
{
    std::string digits = UnscaledDigits();

    if (_scale > 0)
    {
        if (digits.size() <= _scale)
        {
            digits.insert(digits.begin(), _scale - digits.size() + 1, '0');
        }
        digits.insert(digits.size() - _scale, ".");
    }

    const bool isZero = digits.find_first_not_of("0.") == std::string::npos;
    if (!_isPositive && !isZero)
    {
        digits.insert(digits.begin(), '-');
    }

    return digits;
}

}
