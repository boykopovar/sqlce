#include "value_convert.hpp"

#include <pybind11/stl.h>

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

#include "sdf/domain/DateTimeValue.hpp"
#include "sdf/domain/Guid.hpp"
#include "sdf/domain/NumericValue.hpp"

namespace sdf::bindings
{

namespace py = pybind11;

namespace
{

constexpr char DatetimeModuleName[] = "datetime";
constexpr char DatetimeClassName[] = "datetime";
constexpr char DecimalModuleName[] = "decimal";
constexpr char DecimalClassName[] = "Decimal";
constexpr char UuidModuleName[] = "uuid";
constexpr char UuidClassName[] = "UUID";
constexpr char BytesLeArgName[] = "bytes_le";
constexpr std::uint32_t MicrosecondsPerMillisecond = 1000;
constexpr std::uint32_t MillisecondsPerSecond = 1000;
constexpr std::uint32_t SecondsPerMinute = 60;
constexpr std::uint32_t MinutesPerHour = 60;
constexpr std::uint32_t HoursPerDay = 24;

py::object ImportAttr(const char* moduleName, const char* attrName)
{
    return py::module_::import(moduleName).attr(attrName);
}

py::tuple DigitsToTuple(const std::string& digits)
{
    py::tuple result(digits.size());
    for (std::size_t i = 0; i < digits.size(); ++i)
    {
        result[i] = py::int_(digits[i] - '0');
    }
    return result;
}

py::object DateTimeFromValue(const domain::DateTimeValue& value)
{
    static const py::object datetimeClass = ImportAttr(DatetimeModuleName, DatetimeClassName);

    const domain::CalendarDate date = value.Date();
    const std::uint32_t totalSeconds = value.MillisecondsSinceMidnight() / MillisecondsPerSecond;
    const std::uint32_t microsecond =
        (value.MillisecondsSinceMidnight() % MillisecondsPerSecond) * MicrosecondsPerMillisecond;
    const std::uint32_t hour = (totalSeconds / (SecondsPerMinute * MinutesPerHour)) % HoursPerDay;
    const std::uint32_t minute = (totalSeconds / SecondsPerMinute) % MinutesPerHour;
    const std::uint32_t second = totalSeconds % SecondsPerMinute;

    return datetimeClass(date.year, date.month, date.day, hour, minute, second, microsecond);
}

py::object DecimalFromValue(const domain::NumericValue& value)
{
    static const py::object decimalClass = ImportAttr(DecimalModuleName, DecimalClassName);

    const int sign = value.IsPositive() ? 0 : 1;
    const py::tuple digits = DigitsToTuple(value.UnscaledDigits());
    const int exponent = -static_cast<int>(value.Scale());

    return decimalClass(py::make_tuple(sign, digits, exponent));
}

py::object UuidFromValue(const domain::Guid& value)
{
    static const py::object uuidClass = ImportAttr(UuidModuleName, UuidClassName);

    const std::array<std::uint8_t, 16>& bytes = value.Bytes();
    py::bytes bytesLe(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return uuidClass(py::arg(BytesLeArgName) = bytesLe);
}

struct ValueVisitor
{
    py::object operator()(const domain::NullValue&) const
    {
        return py::none();
    }

    py::object operator()(std::int64_t value) const
    {
        return py::int_(value);
    }

    py::object operator()(std::uint64_t value) const
    {
        return py::int_(value);
    }

    py::object operator()(double value) const
    {
        return py::float_(value);
    }

    py::object operator()(bool value) const
    {
        return py::bool_(value);
    }

    py::object operator()(const std::string& value) const
    {
        return py::str(value);
    }

    py::object operator()(const std::vector<std::uint8_t>& value) const
    {
        return py::bytes(reinterpret_cast<const char*>(value.data()), value.size());
    }

    py::object operator()(const domain::DateTimeValue& value) const
    {
        return DateTimeFromValue(value);
    }

    py::object operator()(const domain::NumericValue& value) const
    {
        return DecimalFromValue(value);
    }

    py::object operator()(const domain::Guid& value) const
    {
        return UuidFromValue(value);
    }
};

}

py::object ToPyObject(const domain::ColumnValue& value)
{
    return std::visit(ValueVisitor{}, value.Storage());
}

}
