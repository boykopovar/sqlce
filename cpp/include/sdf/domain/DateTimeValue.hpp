#ifndef SDF_DOMAIN_DATE_TIME_VALUE_HPP
#define SDF_DOMAIN_DATE_TIME_VALUE_HPP

#include <cstdint>
#include <string>

namespace sdf::domain
{

class DateTimeValue
{
public:
    DateTimeValue();
    DateTimeValue(std::uint32_t millisecondsSinceMidnight, std::uint32_t daysSinceEpoch);

    std::string ToString() const;

private:
    std::uint32_t _millisecondsSinceMidnight;
    std::uint32_t _daysSinceEpoch;
};

}

#endif
