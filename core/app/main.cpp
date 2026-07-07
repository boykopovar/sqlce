#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

#include "sdf/application/SdfDatabase.hpp"
#include "sdf/domain/EncryptionMode.hpp"

namespace
{

std::string Truncate(const std::string& value, std::size_t maxLen)
{
    if (value.size() > maxLen)
    {
        return value.substr(0, maxLen) + "...";
    }
    return value;
}

void PrintTableRows(const std::vector<std::string>& columnNames, const std::vector<sdf::domain::Row>& rows)
{
    std::vector<std::vector<std::string>> stringRows;
    stringRows.reserve(rows.size());
    for (const sdf::domain::Row& row : rows)
    {
        std::vector<std::string> stringRow;
        stringRow.reserve(columnNames.size());
        for (const std::string& columnName : columnNames)
        {
            const sdf::domain::ColumnValue* value = row.Find(columnName);
            stringRow.push_back(Truncate(value != nullptr ? value->ToDisplayString() : "", 20));
        }
        stringRows.push_back(std::move(stringRow));
    }

    std::vector<std::size_t> widths;
    widths.reserve(columnNames.size());
    for (const std::string& name : columnNames)
    {
        widths.push_back(name.size());
    }
    for (const std::vector<std::string>& stringRow : stringRows)
    {
        for (std::size_t i = 0; i < stringRow.size(); ++i)
        {
            widths[i] = std::max(widths[i], stringRow[i].size());
        }
    }

    for (std::size_t i = 0; i < columnNames.size(); ++i)
    {
        std::cout << "  " << columnNames[i];
        for (std::size_t pad = columnNames[i].size(); pad < widths[i]; ++pad)
        {
            std::cout << ' ';
        }
    }
    std::cout << '\n';

    for (const std::vector<std::string>& stringRow : stringRows)
    {
        for (std::size_t i = 0; i < stringRow.size(); ++i)
        {
            std::cout << "  " << stringRow[i];
            for (std::size_t pad = stringRow[i].size(); pad < widths[i]; ++pad)
            {
                std::cout << ' ';
            }
        }
        std::cout << '\n';
    }
}

void DumpFile(const std::filesystem::path& sdfPath, const std::string& password)
{
    std::cout << "\n" << sdfPath.filename().string() << "\n";

    std::unique_ptr<sdf::application::SdfDatabase> db;
    try
    {
        db = password.empty() ? std::make_unique<sdf::application::SdfDatabase>(sdfPath.string())
                               : std::make_unique<sdf::application::SdfDatabase>(sdfPath.string(), password);
    }
    catch (const sdf::domain::UnsupportedEncryptionModeException& error)
    {
        std::cout << "skipped: " << error.what() << "\n";
        return;
    }
    catch (const sdf::domain::InvalidPasswordException& error)
    {
        std::cout << "skipped: " << error.what() << "\n";
        return;
    }

    std::vector<std::string> tableNames = db->ListTables();

    if (tableNames.empty())
    {
        std::cout << "no tables found\n";
        return;
    }

    for (const std::string& tableName : tableNames)
    {
        std::cout << "\ntable: " << tableName << "\n";

        for (const sdf::application::ColumnSchema& column : db->TableSchema(tableName))
        {
            std::cout << "  " << column.ordinal << ". " << column.name << " " << column.typeName
                       << " size=" << column.declaredSize;
            if (column.precision.has_value())
            {
                std::cout << ", precision=" << static_cast<int>(*column.precision)
                           << ", scale=" << static_cast<int>(*column.scale);
            }
            std::cout << "\n";
        }

        const std::vector<sdf::domain::Row> rows = db->ReadTable(tableName);
        if (rows.empty())
        {
            std::cout << "  (no rows)\n";
            continue;
        }

        std::vector<std::string> columnNames;
        for (const auto& [name, value] : rows.front().Values())
        {
            columnNames.push_back(name);
        }
        PrintTableRows(columnNames, rows);
    }
}

}

int main(int argc, char** argv)
{
    std::filesystem::path directory("research/raw/examples");
    std::string password;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argument = argv[i];
        if (argument == "--password" && i + 1 < argc)
        {
            password = argv[++i];
        }
        else
        {
            directory = std::filesystem::path(argument);
        }
    }

    std::cout << "current working directory: " << std::filesystem::current_path() << "\n";

    std::vector<std::pair<std::filesystem::path, std::string>> directoriesToScan;
    directoriesToScan.emplace_back(directory, password);
    directoriesToScan.emplace_back(directory / "protected", password.empty() ? "secret123" : password);

    for (const auto& [scanDirectory, scanPassword] : directoriesToScan)
    {
        std::cout << "looking for: " << std::filesystem::absolute(scanDirectory) << "\n";

        if (!std::filesystem::is_directory(scanDirectory))
        {
            std::cerr << "directory not found: " << scanDirectory << "\n";
            continue;
        }

        std::vector<std::filesystem::path> sdfFiles;
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(scanDirectory))
        {
            if (entry.path().extension() == ".sdf")
            {
                sdfFiles.push_back(entry.path());
            }
        }
        std::sort(sdfFiles.begin(), sdfFiles.end());

        if (sdfFiles.empty())
        {
            std::cout << "no .sdf files found in " << scanDirectory << "\n";
            continue;
        }

        for (const std::filesystem::path& sdfFile : sdfFiles)
        {
            DumpFile(sdfFile, scanPassword);
        }
    }

    return 0;
}
