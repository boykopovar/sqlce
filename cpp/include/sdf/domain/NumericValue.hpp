#ifndef SDF_DOMAIN_NUMERIC_VALUE_HPP
#define SDF_DOMAIN_NUMERIC_VALUE_HPP

#include <array>
#include <cstdint>
#include <string>

namespace sdf::domain
{

class NumericValue
{
public:
    NumericValue();
    NumericValue(
        std::uint8_t precision,
        std::uint8_t scale,
        bool isPositive,
        const std::array<std::uint8_t, 16>& unscaledLittleEndian);

    std::uint8_t Precision() const;
    std::uint8_t Scale() const;
    bool IsPositive() const;

    std::string ToString() const;

private:
    std::uint8_t _precision;
    std::uint8_t _scale;
    bool _isPositive;
    std::array<std::uint8_t, 16> _unscaledLittleEndian;
};

}

#endif
