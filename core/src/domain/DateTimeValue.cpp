#include "sdf/domain/DateTimeValue.hpp"

#include <cstdio>

namespace sdf::domain
{

namespace
{

constexpr int EpochYear = 1900;
constexpr int EpochMonth = 1;
constexpr int EpochDay = 1;

bool IsLeapYear(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int DaysInMonth(int year, int month)
{
    static constexpr int daysByMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && IsLeapYear(year))
    {
        return 29;
    }
    return daysByMonth[month - 1];
}

CalendarDate AddDays(int year, int month, int day, std::int32_t daysToAdd)
{
    if (daysToAdd >= 0)
    {
        std::uint32_t remaining = static_cast<std::uint32_t>(daysToAdd);
        while (remaining > 0)
        {
            const int daysLeftInMonth = DaysInMonth(year, month) - day;
            if (static_cast<std::uint32_t>(daysLeftInMonth) >= remaining)
            {
                day += static_cast<int>(remaining);
                remaining = 0;
            }
            else
            {
                remaining -= static_cast<std::uint32_t>(daysLeftInMonth) + 1;
                day = 1;
                month += 1;
                if (month > 12)
                {
                    month = 1;
                    year += 1;
                }
            }
        }
    }
    else
    {
        std::uint32_t remaining = static_cast<std::uint32_t>(-daysToAdd);
        while (remaining > 0)
        {
            const int daysIntoMonth = day - 1;
            if (static_cast<std::uint32_t>(daysIntoMonth) >= remaining)
            {
                day -= static_cast<int>(remaining);
                remaining = 0;
            }
            else
            {
                remaining -= static_cast<std::uint32_t>(daysIntoMonth) + 1;
                month -= 1;
                if (month < 1)
                {
                    month = 12;
                    year -= 1;
                }
                day = DaysInMonth(year, month);
            }
        }
    }
    return CalendarDate{year, month, day};
}

}

DateTimeValue::DateTimeValue() : _daysSinceEpoch(0), _ticksSinceMidnight(0)
{
}

DateTimeValue::DateTimeValue(std::int32_t daysSinceEpoch, std::uint32_t ticksSinceMidnight)
    : _daysSinceEpoch(daysSinceEpoch), _ticksSinceMidnight(ticksSinceMidnight)
{
}

std::uint32_t DateTimeValue::MillisecondsSinceMidnight() const
{
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(_ticksSinceMidnight) * 10ULL + 1ULL) / 3ULL);
}

std::int32_t DateTimeValue::DaysSinceEpoch() const
{
    return _daysSinceEpoch;
}

CalendarDate DateTimeValue::Date() const
{
    return AddDays(EpochYear, EpochMonth, EpochDay, _daysSinceEpoch);
}

std::string DateTimeValue::ToString() const
{
    const CalendarDate date = Date();

    const std::uint32_t milliseconds = MillisecondsSinceMidnight();
    const std::uint32_t totalSeconds = milliseconds / 1000;
    const std::uint32_t ms = milliseconds % 1000;
    const std::uint32_t hours = (totalSeconds / 3600) % 24;
    const std::uint32_t minutes = (totalSeconds / 60) % 60;
    const std::uint32_t seconds = totalSeconds % 60;

    char buffer[32];
    std::snprintf(
        buffer,
        sizeof(buffer),
        "%04d-%02d-%02d %02u:%02u:%02u.%03u",
        date.year,
        date.month,
        date.day,
        hours,
        minutes,
        seconds,
        ms);
    return std::string(buffer);
}

}
