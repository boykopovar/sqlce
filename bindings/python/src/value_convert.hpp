#ifndef SDF_BINDINGS_VALUE_CONVERT_HPP
#define SDF_BINDINGS_VALUE_CONVERT_HPP

#include <pybind11/pybind11.h>

#include "sdf/domain/ColumnValue.hpp"

namespace sdf::bindings
{

pybind11::object ToPyObject(const domain::ColumnValue& value);

}

#endif
