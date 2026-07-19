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
        DateTimeValue(std::int32_t daysSinceEpoch, std::uint32_t ticksSinceMidnight);

        [[nodiscard]] std::uint32_t MillisecondsSinceMidnight() const;
        [[nodiscard]] std::int32_t DaysSinceEpoch() const;
        [[nodiscard]] CalendarDate Date() const;

        [[nodiscard]] std::string ToString() const;

    private:
        std::int32_t _daysSinceEpoch;
        std::uint32_t _ticksSinceMidnight;
    };

}

#endif
