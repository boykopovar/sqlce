#include <emscripten/bind.h>

#include <memory>
#include <string>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/application/SdfDatabase.hpp"
#include "sdf/domain/ColumnValue.hpp"
#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/Row.hpp"

namespace sdf::wasm
{

namespace
{

void AppendEscapedJsonString(std::string& out, const std::string& value)
{
    out.push_back('"');
    for (char ch : value)
    {
        switch (ch)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (static_cast<unsigned char>(ch) < 0x20)
            {
                static const char hexDigits[] = "0123456789abcdef";
                out += "\\u00";
                out.push_back(hexDigits[(static_cast<unsigned char>(ch) >> 4) & 0xF]);
                out.push_back(hexDigits[static_cast<unsigned char>(ch) & 0xF]);
            }
            else
            {
                out.push_back(ch);
            }
            break;
        }
    }
    out.push_back('"');
}

std::string EscapeJsonString(const std::string& value)
{
    std::string result;
    result.reserve(value.size() + 2);
    AppendEscapedJsonString(result, value);
    return result;
}

std::string OptionalByteToJson(std::optional<std::uint8_t> value)
{
    if (!value.has_value())
    {
        return "null";
    }
    return std::to_string(static_cast<int>(*value));
}

std::string ErrorResultJson(const std::string& message)
{
    std::string json;
    json += "{\"ok\":false,\"error\":";
    AppendEscapedJsonString(json, message);
    json += "}";
    return json;
}

std::string OkResultJson(const std::string& handleKey)
{
    std::string json;
    json += "{\"ok\":true,\"handle\":";
    AppendEscapedJsonString(json, handleKey);
    json += "}";
    return json;
}

}

class SdfDatabaseWasm
{
public:
    static std::string open(const std::string& path)
    {
        return openInternal(path, nullptr);
    }

    static std::string openWithPassword(const std::string& path, const std::string& password)
    {
        return openInternal(path, &password);
    }

    std::string listTablesJson() const
    {
        std::string json;
        json.push_back('[');
        const std::vector<std::string> tables = _database->ListTables();
        bool first = true;
        for (const std::string& table : tables)
        {
            if (!first)
            {
                json.push_back(',');
            }
            first = false;
            AppendEscapedJsonString(json, table);
        }
        json.push_back(']');
        return json;
    }

    std::string tableSchemaJson(const std::string& tableName) const
    {
        std::string json;
        json.push_back('[');
        const std::vector<application::ColumnSchema> columns = _database->TableSchema(tableName);
        bool first = true;
        for (const application::ColumnSchema& column : columns)
        {
            if (!first)
            {
                json.push_back(',');
            }
            first = false;
            json += "{\"ordinal\":";
            json += std::to_string(column.ordinal);
            json += ",\"name\":";
            AppendEscapedJsonString(json, column.name);
            json += ",\"type_name\":";
            AppendEscapedJsonString(json, std::string(column.typeName));
            json += ",\"declared_size\":";
            json += std::to_string(column.declaredSize);
            json += ",\"precision\":";
            json += OptionalByteToJson(column.precision);
            json += ",\"scale\":";
            json += OptionalByteToJson(column.scale);
            json += "}";
        }
        json.push_back(']');
        return json;
    }

    std::string tableDataJson(const std::string& tableName) const
    {
        std::string json;
        json.push_back('[');
        const std::vector<domain::Row> rows = _database->ReadTable(tableName);
        bool firstRow = true;
        for (const domain::Row& row : rows)
        {
            if (!firstRow)
            {
                json.push_back(',');
            }
            firstRow = false;
            json.push_back('{');
            bool firstColumn = true;
            for (const auto& columnNameAndValue : row.Values())
            {
                const std::string& columnName = columnNameAndValue.first;
                const domain::ColumnValue& value = columnNameAndValue.second;
                if (!firstColumn)
                {
                    json.push_back(',');
                }
                firstColumn = false;
                AppendEscapedJsonString(json, columnName);
                json += ":{\"isNull\":";
                json += value.IsNull() ? "true" : "false";
                json += ",\"value\":";
                AppendEscapedJsonString(json, value.ToDisplayString());
                json += "}";
            }
            json.push_back('}');
        }
        json.push_back(']');
        return json;
    }

private:
    explicit SdfDatabaseWasm(std::unique_ptr<application::SdfDatabase> database) : _database(std::move(database))
    {
    }

    static std::string openInternal(const std::string& path, const std::string* password)
    {
        try
        {
            std::unique_ptr<application::SdfDatabase> database = password
                ? std::make_unique<application::SdfDatabase>(path, *password)
                : std::make_unique<application::SdfDatabase>(path);
            const std::string handleKey = registerInstance(SdfDatabaseWasm(std::move(database)));
            return OkResultJson(handleKey);
        }
        catch (const domain::InvalidPasswordException& error)
        {
            return ErrorResultJson(error.what());
        }
        catch (const domain::UnsupportedEncryptionModeException& error)
        {
            return ErrorResultJson(error.what());
        }
        catch (const std::exception& error)
        {
            return ErrorResultJson(error.what());
        }
    }

    static std::string registerInstance(SdfDatabaseWasm instance)
    {
        static std::uint64_t nextHandle = 1;
        const std::uint64_t handle = nextHandle;
        nextHandle += 1;
        const std::string handleKey = std::to_string(handle);
        registry().emplace(handleKey, std::move(instance));
        return handleKey;
    }

    static std::map<std::string, SdfDatabaseWasm>& registry()
    {
        static std::map<std::string, SdfDatabaseWasm> instances;
        return instances;
    }

    std::unique_ptr<application::SdfDatabase> _database;

    friend class SdfDatabaseHandle;
};

class SdfDatabaseHandle
{
public:
    static std::string open(const std::string& path)
    {
        return SdfDatabaseWasm::open(path);
    }

    static std::string openWithPassword(const std::string& path, const std::string& password)
    {
        return SdfDatabaseWasm::openWithPassword(path, password);
    }

    static std::string listTablesJson(const std::string& handleKey)
    {
        return withInstance(handleKey, [](const SdfDatabaseWasm& db) { return db.listTablesJson(); });
    }

    static std::string tableSchemaJson(const std::string& handleKey, const std::string& tableName)
    {
        return withInstance(handleKey, [&tableName](const SdfDatabaseWasm& db) { return db.tableSchemaJson(tableName); });
    }

    static std::string tableDataJson(const std::string& handleKey, const std::string& tableName)
    {
        return withInstance(handleKey, [&tableName](const SdfDatabaseWasm& db) { return db.tableDataJson(tableName); });
    }

    static void close(const std::string& handleKey)
    {
        SdfDatabaseWasm::registry().erase(handleKey);
    }

private:
    template <typename Func>
    static std::string withInstance(const std::string& handleKey, Func&& func)
    {
        auto& registry = SdfDatabaseWasm::registry();
        auto it = registry.find(handleKey);
        if (it == registry.end())
        {
            return ErrorResultJson("unknown database handle");
        }
        try
        {
            const std::string payload = func(it->second);
            std::string json;
            json += "{\"ok\":true,\"data\":";
            json += payload;
            json += "}";
            return json;
        }
        catch (const std::exception& error)
        {
            return ErrorResultJson(error.what());
        }
    }
};

}

EMSCRIPTEN_BINDINGS(sqlce)
{
    emscripten::class_<sdf::wasm::SdfDatabaseHandle>("SdfDatabase")
        .class_function("open", &sdf::wasm::SdfDatabaseHandle::open)
        .class_function("openWithPassword", &sdf::wasm::SdfDatabaseHandle::openWithPassword)
        .class_function("listTablesJson", &sdf::wasm::SdfDatabaseHandle::listTablesJson)
        .class_function("tableSchemaJson", &sdf::wasm::SdfDatabaseHandle::tableSchemaJson)
        .class_function("tableDataJson", &sdf::wasm::SdfDatabaseHandle::tableDataJson)
        .class_function("close", &sdf::wasm::SdfDatabaseHandle::close);
}
