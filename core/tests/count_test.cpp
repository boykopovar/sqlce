#include <iostream>
#include <filesystem>

#include "sdf/application/SqlceDatabase.hpp"

int main() {
    for (auto& entry : std::filesystem::directory_iterator("../research/raw/examples")) {
        if (entry.path().extension() != ".sdf") continue;
        std::cout << "=== " << entry.path().filename().string() << "\n";
        sdf::application::SqlceDatabase db(entry.path().string());
        for (auto& t : db.ListTables()) {
            std::cout << " table " << t << " " << db.ReadTable(t).size() << "\n";
        }
    }
}
