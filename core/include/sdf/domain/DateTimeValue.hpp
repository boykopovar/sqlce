#ifndef SDF_DOMAIN_DATE_TIME_VALUE_HPP
#define SDF_DOMAIN_DATE_TIME_VALUE_HPP

#include <cstdint>
#include <string>

namespace sdf::domain
{

struct CalendarDate
{
    int year;
    int month;
    int day;
};

class DateTimeValue
{
public:
    DateTimeValue();
    DateTimeValue(std::uint32_t millisecondsSinceMidnight, std::uint32_t daysSinceEpoch);

    std::uint32_t MillisecondsSinceMidnight() const;
    std::uint32_t DaysSinceEpoch() const;
    CalendarDate Date() const;

    std::string ToString() const;

private:
    std::uint32_t _millisecondsSinceMidnight;
    std::uint32_t _daysSinceEpoch;
};

}

#endif
