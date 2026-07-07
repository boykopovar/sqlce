#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/application/SdfDatabase.hpp"
#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/Row.hpp"

#include "value_convert.hpp"

namespace sdf::bindings
{

namespace py = pybind11;

namespace
{

constexpr char ModuleDoc[] = "Native reader for SQL Server Compact (.sdf) database files.";

constexpr char DatabaseClassName[] = "SdfDatabase";
constexpr char DatabaseDoc[] = "Reads tables, schemas and rows from a .sdf database file.";
constexpr char InitDoc[] = "Open a .sdf file at the given path.";
constexpr char InitWithPasswordDoc[] = "Open a password-protected .sdf file, auto-detecting its encryption mode.";
constexpr char PathArgName[] = "path";
constexpr char PasswordArgName[] = "password";
constexpr char ListTablesName[] = "list_tables";
constexpr char ListTablesDoc[] = "Return the names of all tables in the database.";
constexpr char TableSchemaName[] = "table_schema";
constexpr char TableSchemaDoc[] = "Return the column schema of a table.";
constexpr char ReadTableName[] = "read_table";
constexpr char ReadTableDoc[] = "Return every row of a table as a list of column-name -> value mappings.";
constexpr char TableNameArgName[] = "table_name";

constexpr char SchemaClassName[] = "ColumnSchema";
constexpr char SchemaDoc[] = "Describes a single column of a table.";
constexpr char OrdinalAttrName[] = "ordinal";
constexpr char NameAttrName[] = "name";
constexpr char TypeNameAttrName[] = "type_name";
constexpr char DeclaredSizeAttrName[] = "declared_size";
constexpr char PrecisionAttrName[] = "precision";
constexpr char ScaleAttrName[] = "scale";

constexpr char UnsupportedEncryptionModeErrorName[] = "UnsupportedEncryptionModeError";
constexpr char InvalidPasswordErrorName[] = "InvalidPasswordError";

py::dict RowToDict(const domain::Row& row)
{
    py::dict result;
    for (const auto& [columnName, value] : row.Values())
    {
        result[py::str(columnName)] = ToPyObject(value);
    }
    return result;
}

std::vector<py::dict> ReadTableAsDicts(const application::SdfDatabase& database, const std::string& tableName)
{
    std::vector<py::dict> result;
    const std::vector<domain::Row> rows = database.ReadTable(tableName);
    result.reserve(rows.size());
    for (const domain::Row& row : rows)
    {
        result.push_back(RowToDict(row));
    }
    return result;
}

std::optional<int> OptionalByteToInt(std::optional<std::uint8_t> value)
{
    if (!value.has_value())
    {
        return std::nullopt;
    }
    return static_cast<int>(*value);
}

void RegisterExceptions(py::module_& module)
{
    static const py::exception<domain::UnsupportedEncryptionModeException> unsupportedModeError(
        module, UnsupportedEncryptionModeErrorName, PyExc_ValueError);
    static const py::exception<domain::InvalidPasswordException> invalidPasswordError(
        module, InvalidPasswordErrorName, PyExc_ValueError);

    py::register_exception_translator([](std::exception_ptr lastException) {
        try
        {
            if (lastException)
            {
                std::rethrow_exception(lastException);
            }
        }
        catch (const domain::UnsupportedEncryptionModeException& error)
        {
            PyErr_SetString(unsupportedModeError.ptr(), error.what());
        }
        catch (const domain::InvalidPasswordException& error)
        {
            PyErr_SetString(invalidPasswordError.ptr(), error.what());
        }
    });
}

}

PYBIND11_MODULE(_sdf_native, module)
{
    module.doc() = ModuleDoc;

    RegisterExceptions(module);

    py::class_<application::ColumnSchema>(module, SchemaClassName, SchemaDoc)
        .def_property_readonly(
            OrdinalAttrName,
            [](const application::ColumnSchema& schema) { return schema.ordinal; })
        .def_property_readonly(
            NameAttrName,
            [](const application::ColumnSchema& schema) { return schema.name; })
        .def_property_readonly(
            TypeNameAttrName,
            [](const application::ColumnSchema& schema) { return std::string(schema.typeName); })
        .def_property_readonly(
            DeclaredSizeAttrName,
            [](const application::ColumnSchema& schema) { return schema.declaredSize; })
        .def_property_readonly(
            PrecisionAttrName,
            [](const application::ColumnSchema& schema) { return OptionalByteToInt(schema.precision); })
        .def_property_readonly(
            ScaleAttrName,
            [](const application::ColumnSchema& schema) { return OptionalByteToInt(schema.scale); });

    py::class_<application::SdfDatabase>(module, DatabaseClassName, DatabaseDoc)
        .def(py::init<const std::string&>(), py::arg(PathArgName), InitDoc)
        .def(
            py::init<const std::string&, const std::string&>(),
            py::arg(PathArgName),
            py::arg(PasswordArgName),
            InitWithPasswordDoc)
        .def(ListTablesName, &application::SdfDatabase::ListTables, ListTablesDoc)
        .def(
            TableSchemaName,
            &application::SdfDatabase::TableSchema,
            py::arg(TableNameArgName),
            TableSchemaDoc)
        .def(ReadTableName, &ReadTableAsDicts, py::arg(TableNameArgName), ReadTableDoc);
}

}
