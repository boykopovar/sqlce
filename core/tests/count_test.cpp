#include <cstdlib>
#include <iostream>
#include <filesystem>

#include "sdf/application/SqlceDatabase.hpp"

int main() {
    bool allOk = true;
    for (auto& entry : std::filesystem::directory_iterator("../research/raw/examples")) {
        if (entry.path().extension() != ".sdf") continue;
        std::cout << "=== " << entry.path().filename().string() << "\n";
        sdf::application::SqlceDatabase db(entry.path().string());
        for (auto& t : db.ListTables()) {
            const std::uint32_t rowCount = db.RowCount(t);
            const std::size_t fullScanCount = db.ReadTable(t).size();
            const bool ok = rowCount == fullScanCount;
            allOk = allOk && ok;
            std::cout << " table " << t << " " << fullScanCount;
            if (!ok) {
                std::cout << " (RowCount mismatch: " << rowCount << ")";
            }
            std::cout << "\n";
        }
    }
    return allOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
