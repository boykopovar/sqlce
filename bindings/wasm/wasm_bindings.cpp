#include <emscripten/bind.h>

#include <memory>
#include <string>

#include "sdf/application/ColumnSchema.hpp"
#include "sdf/application/SqlceDatabase.hpp"
#include "sdf/domain/ColumnValue.hpp"
#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/FormatVersion.hpp"
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

class SqlceDatabaseWasm
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

    std::uint32_t encryptionMode() const
    {
        return static_cast<std::uint32_t>(_database->GetEncryptionMode());
    }

    static std::string encryptionModeOfFile(const std::string& path)
    {
        try
        {
            const std::uint32_t mode = static_cast<std::uint32_t>(application::SqlceDatabase::GetEncryptionMode(path));
            std::string json;
            json += "{\"ok\":true,\"data\":";
            json += std::to_string(mode);
            json += "}";
            return json;
        }
        catch (const std::exception& error)
        {
            return ErrorResultJson(error.what());
        }
    }

    std::uint32_t resolvedEncryptionMode() const
    {
        return static_cast<std::uint32_t>(_database->ResolvedEncryptionMode());
    }

    std::uint32_t formatVersion() const
    {
        return static_cast<std::uint32_t>(_database->GetFormatVersion());
    }

    static std::string formatVersionOfFile(const std::string& path)
    {
        try
        {
            const std::uint32_t version = static_cast<std::uint32_t>(application::SqlceDatabase::GetFormatVersion(path));
            std::string json;
            json += "{\"ok\":true,\"data\":";
            json += std::to_string(version);
            json += "}";
            return json;
        }
        catch (const std::exception& error)
        {
            return ErrorResultJson(error.what());
        }
    }

private:
    explicit SqlceDatabaseWasm(std::unique_ptr<application::SqlceDatabase> database) : _database(std::move(database))
    {
    }

    static std::string openInternal(const std::string& path, const std::string* password)
    {
        try
        {
            std::unique_ptr<application::SqlceDatabase> database = password
                ? std::make_unique<application::SqlceDatabase>(path, *password)
                : std::make_unique<application::SqlceDatabase>(path);
            const std::string handleKey = registerInstance(SqlceDatabaseWasm(std::move(database)));
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

    static std::string registerInstance(SqlceDatabaseWasm instance)
    {
        static std::uint64_t nextHandle = 1;
        const std::uint64_t handle = nextHandle;
        nextHandle += 1;
        const std::string handleKey = std::to_string(handle);
        registry().emplace(handleKey, std::move(instance));
        return handleKey;
    }

    static std::map<std::string, SqlceDatabaseWasm>& registry()
    {
        static std::map<std::string, SqlceDatabaseWasm> instances;
        return instances;
    }

    std::unique_ptr<application::SqlceDatabase> _database;

    friend class SqlceDatabaseHandle;
};

class SqlceDatabaseHandle
{
public:
    static std::string open(const std::string& path)
    {
        return SqlceDatabaseWasm::open(path);
    }

    static std::string openWithPassword(const std::string& path, const std::string& password)
    {
        return SqlceDatabaseWasm::openWithPassword(path, password);
    }

    static std::string listTablesJson(const std::string& handleKey)
    {
        return withInstance(handleKey, [](const SqlceDatabaseWasm& db) { return db.listTablesJson(); });
    }

    static std::string tableSchemaJson(const std::string& handleKey, const std::string& tableName)
    {
        return withInstance(handleKey, [&tableName](const SqlceDatabaseWasm& db) { return db.tableSchemaJson(tableName); });
    }

    static std::string tableDataJson(const std::string& handleKey, const std::string& tableName)
    {
        return withInstance(handleKey, [&tableName](const SqlceDatabaseWasm& db) { return db.tableDataJson(tableName); });
    }

    static std::string encryptionModeJson(const std::string& handleKey)
    {
        return withInstance(handleKey, [](const SqlceDatabaseWasm& db) { return std::to_string(db.encryptionMode()); });
    }

    static std::string encryptionModeOfFile(const std::string& path)
    {
        return SqlceDatabaseWasm::encryptionModeOfFile(path);
    }

    static std::string resolvedEncryptionModeJson(const std::string& handleKey)
    {
        return withInstance(handleKey, [](const SqlceDatabaseWasm& db) { return std::to_string(db.resolvedEncryptionMode()); });
    }

    static std::string formatVersionJson(const std::string& handleKey)
    {
        return withInstance(handleKey, [](const SqlceDatabaseWasm& db) { return std::to_string(db.formatVersion()); });
    }

    static std::string formatVersionOfFile(const std::string& path)
    {
        return SqlceDatabaseWasm::formatVersionOfFile(path);
    }

    static void close(const std::string& handleKey)
    {
        SqlceDatabaseWasm::registry().erase(handleKey);
    }

private:
    template <typename Func>
    static std::string withInstance(const std::string& handleKey, Func&& func)
    {
        auto& registry = SqlceDatabaseWasm::registry();
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
    emscripten::class_<sdf::wasm::SqlceDatabaseHandle>("SqlceDatabase")
        .class_function("open", &sdf::wasm::SqlceDatabaseHandle::open)
        .class_function("openWithPassword", &sdf::wasm::SqlceDatabaseHandle::openWithPassword)
        .class_function("listTablesJson", &sdf::wasm::SqlceDatabaseHandle::listTablesJson)
        .class_function("tableSchemaJson", &sdf::wasm::SqlceDatabaseHandle::tableSchemaJson)
        .class_function("tableDataJson", &sdf::wasm::SqlceDatabaseHandle::tableDataJson)
        .class_function("encryptionModeJson", &sdf::wasm::SqlceDatabaseHandle::encryptionModeJson)
        .class_function("encryptionModeOfFile", &sdf::wasm::SqlceDatabaseHandle::encryptionModeOfFile)
        .class_function("resolvedEncryptionModeJson", &sdf::wasm::SqlceDatabaseHandle::resolvedEncryptionModeJson)
        .class_function("formatVersionJson", &sdf::wasm::SqlceDatabaseHandle::formatVersionJson)
        .class_function("formatVersionOfFile", &sdf::wasm::SqlceDatabaseHandle::formatVersionOfFile)
        .class_function("close", &sdf::wasm::SqlceDatabaseHandle::close);
}
