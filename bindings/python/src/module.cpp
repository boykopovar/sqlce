#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/application/SqlceDatabase.hpp"
#include "sdf/application/TableRowRange.hpp"
#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/FormatVersion.hpp"
#include "sdf/domain/LazyLob.hpp"
#include "sdf/domain/Row.hpp"

#include "value_convert.hpp"

namespace sdf::bindings
{

namespace py = pybind11;

namespace
{

constexpr char ModuleDoc[] = "Native reader for SQL Server Compact (.sdf) database files.";

constexpr char DatabaseClassName[] = "SqlceDatabase";
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
constexpr char IterateTableName[] = "iterate_table";
constexpr char IterateTableDoc[]
    = "Lazily iterate over the rows of a table, decoding one row at a time without materializing the whole table.";
constexpr char TableNameArgName[] = "table_name";
constexpr char GetEncryptionModeName[] = "get_encryption_mode";
constexpr char GetEncryptionModeDoc[] = "Return the encryption mode the database file was opened with.";
constexpr char GetEncryptionModeStaticName[] = "get_encryption_mode_from_file";
constexpr char GetEncryptionModeStaticDoc[] = "Return the encryption mode of a .sdf file without opening it.";
constexpr char ResolvedEncryptionModeName[] = "resolved_encryption_mode";
constexpr char ResolvedEncryptionModeDoc[]
    = "Return the encryption mode that was actually resolved by matching the password against the file.";
constexpr char GetFormatVersionName[] = "get_format_version";
constexpr char GetFormatVersionDoc[] = "Return the format version of the database file.";
constexpr char GetFormatVersionStaticName[] = "get_format_version_from_file";
constexpr char GetFormatVersionStaticDoc[] = "Return the format version of a .sdf file without opening it.";
constexpr char ExportDecryptedName[] = "export_decrypted";
constexpr char ExportDecryptedDoc[] = "Return the raw bytes of the database file with encryption removed.";

constexpr char EncryptionModeClassName[] = "EncryptionMode";
constexpr char EncryptionModeDoc[] = "Encryption mode of a .sdf database file.";
constexpr char EncryptionModeNoneName[] = "NONE";
constexpr char EncryptionModeTripleDesSha1Name[] = "TRIPLE_DES_SHA1";
constexpr char EncryptionModeAes128Sha1Name[] = "AES128_SHA1";
constexpr char EncryptionModeAes128Sha256Name[] = "AES128_SHA256";
constexpr char EncryptionModeAes256Sha512Name[] = "AES256_SHA512";

constexpr char FormatVersionClassName[] = "FormatVersion";
constexpr char FormatVersionDoc[] = "Format version of a .sdf database file.";
constexpr char FormatVersionUnknownName[] = "UNKNOWN";
constexpr char FormatVersionSqlCe30Name[] = "SQLCE_30";
constexpr char FormatVersionSqlCe35Name[] = "SQLCE_35";
constexpr char FormatVersionSqlCe35Sp2Name[] = "SQLCE_35_SP2";
constexpr char FormatVersionSqlCe40Name[] = "SQLCE_40";

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

constexpr char LazyLobClassName[] = "LazyLob";
constexpr char LazyLobDoc[] = "Ленивое LOB-значение (NText/Image), читаемое по требованию.";
constexpr char LazyLobReadName[] = "read";
constexpr char LazyLobReadChunksName[] = "read_chunks";

constexpr char TableRowIteratorClassName[] = "TableRowIterator";
constexpr char TableRowIteratorDoc[]
    = "Lazy row-by-row iterator over a table, produced by SqlceDatabase.iterate_table().";

py::dict RowToDict(const domain::Row& row)
{
    py::dict result;
    for (const auto& [columnName, value] : row.Values())
    {
        result[py::str(columnName)] = ToPyObject(value);
    }
    return result;
}

class TableRowIterator
{
public:
    TableRowIterator(const application::SqlceDatabase& database, const std::string& tableName)
        : _range(database.IterateTable(tableName)), _current(_range.begin()), _end(_range.end())
    {
    }

    py::dict Next()
    {
        if (_current == _end)
        {
            throw py::stop_iteration();
        }
        py::dict row = RowToDict(*_current);
        ++_current;
        return row;
    }

private:
    application::TableRowRange _range;
    application::TableRowRange::Iterator _current;
    application::TableRowRange::Iterator _end;
};

std::vector<py::dict> ReadTableAsDicts(const application::SqlceDatabase& database, const std::string& tableName)
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

py::bytes ExportDecryptedAsBytes(const application::SqlceDatabase& database)
{
    const std::vector<std::uint8_t> bytes = database.ExportDecrypted();
    return py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
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

PYBIND11_MODULE(_sqlce_native, module)
{
    module.doc() = ModuleDoc;

    RegisterExceptions(module);

    py::enum_<domain::EncryptionMode>(module, EncryptionModeClassName, EncryptionModeDoc)
        .value(EncryptionModeNoneName, domain::EncryptionMode::None)
        .value(EncryptionModeTripleDesSha1Name, domain::EncryptionMode::TripleDesSha1)
        .value(EncryptionModeAes128Sha1Name, domain::EncryptionMode::Aes128Sha1)
        .value(EncryptionModeAes128Sha256Name, domain::EncryptionMode::Aes128Sha256)
        .value(EncryptionModeAes256Sha512Name, domain::EncryptionMode::Aes256Sha512);

    py::enum_<domain::FormatVersion>(module, FormatVersionClassName, FormatVersionDoc)
        .value(FormatVersionUnknownName, domain::FormatVersion::Unknown)
        .value(FormatVersionSqlCe30Name, domain::FormatVersion::SqlCe30)
        .value(FormatVersionSqlCe35Name, domain::FormatVersion::SqlCe35)
        .value(FormatVersionSqlCe35Sp2Name, domain::FormatVersion::SqlCe35Sp2)
        .value(FormatVersionSqlCe40Name, domain::FormatVersion::SqlCe40);

    const auto readLob = [](const domain::LazyLob& lob) -> py::object {
        if (lob.IsText())
        {
            return py::str(lob.ReadText());
        }
        const std::vector<std::uint8_t> bytes = lob.ReadBytes();
        return py::bytes(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    };

    py::class_<domain::LazyLob>(module, LazyLobClassName, LazyLobDoc)
        .def(LazyLobReadName, readLob)
        .def(LazyLobReadChunksName, [](const domain::LazyLob& lob) {
            py::list result;
            for (const std::vector<std::uint8_t>& chunk : lob.ReadChunks())
            {
                result.append(py::bytes(reinterpret_cast<const char*>(chunk.data()), chunk.size()));
            }
            return result;
        })
        .def("__eq__", [readLob](const domain::LazyLob& lob, const py::object& other) -> py::object {
            if (py::isinstance<domain::LazyLob>(other))
            {
                return py::bool_(readLob(lob).equal(readLob(other.cast<const domain::LazyLob&>())));
            }
            if (py::isinstance<py::str>(other) || py::isinstance<py::bytes>(other))
            {
                return py::bool_(readLob(lob).equal(other));
            }
            return py::module_::import("builtins").attr("NotImplemented");
        })
        .def("__repr__", [](const domain::LazyLob& lob) {
            return "<LazyLob " + std::string(lob.IsText() ? "text" : "binary") + " length="
                + std::to_string(lob.TotalLength()) + ">";
        });

    py::class_<TableRowIterator>(module, TableRowIteratorClassName, TableRowIteratorDoc)
        .def(
            "__iter__",
            [](TableRowIterator& self) -> TableRowIterator& { return self; })
        .def("__next__", &TableRowIterator::Next);

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

    py::class_<application::SqlceDatabase>(module, DatabaseClassName, DatabaseDoc)
        .def(py::init<const std::string&>(), py::arg(PathArgName), InitDoc)
        .def(
            py::init<const std::string&, const std::string&>(),
            py::arg(PathArgName),
            py::arg(PasswordArgName),
            InitWithPasswordDoc)
        .def(ListTablesName, &application::SqlceDatabase::ListTables, ListTablesDoc)
        .def(
            TableSchemaName,
            &application::SqlceDatabase::TableSchema,
            py::arg(TableNameArgName),
            TableSchemaDoc)
        .def(ReadTableName, &ReadTableAsDicts, py::arg(TableNameArgName), ReadTableDoc)
        .def(
            IterateTableName,
            [](const application::SqlceDatabase& database, const std::string& tableName) {
                return TableRowIterator(database, tableName);
            },
            py::arg(TableNameArgName),
            IterateTableDoc,
            py::keep_alive<0, 1>())
        .def(
            GetEncryptionModeName,
            py::overload_cast<>(&application::SqlceDatabase::GetEncryptionMode, py::const_),
            GetEncryptionModeDoc)
        .def_static(
            GetEncryptionModeStaticName,
            py::overload_cast<const std::string&>(&application::SqlceDatabase::GetEncryptionMode),
            py::arg(PathArgName),
            GetEncryptionModeStaticDoc)
        .def(
            ResolvedEncryptionModeName,
            &application::SqlceDatabase::ResolvedEncryptionMode,
            ResolvedEncryptionModeDoc)
        .def(
            GetFormatVersionName,
            py::overload_cast<>(&application::SqlceDatabase::GetFormatVersion, py::const_),
            GetFormatVersionDoc)
        .def_static(
            GetFormatVersionStaticName,
            py::overload_cast<const std::string&>(&application::SqlceDatabase::GetFormatVersion),
            py::arg(PathArgName),
            GetFormatVersionStaticDoc)
        .def(ExportDecryptedName, &ExportDecryptedAsBytes, ExportDecryptedDoc);
}

}
