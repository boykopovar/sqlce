#include "sdf/parsing/SdfDatabaseComponents.hpp"

#include <stdexcept>
#include <utility>

#include "sdf/infrastructure/FileStorage.hpp"
#include "sdf/parsing/CatalogPageScanner.hpp"
#include "sdf/parsing/CatalogRowDecoder.hpp"
#include "sdf/parsing/LobChainRegistry.hpp"
#include "sdf/parsing/RowDecoder.hpp"
#include "sdf/parsing/RowFragmentReassembler.hpp"
#include "sdf/parsing/SdfPageCipher.hpp"
#include "sdf/parsing/TableCatalogBuilder.hpp"

namespace sdf::parsing
{

namespace
{

struct OpenedStorage
{
    std::unique_ptr<domain::IPageStorage> storage;
    domain::EncryptionMode encryptionMode;
};

OpenedStorage OpenStorage(const std::string& path, const std::string& password)
{
    const std::vector<std::uint8_t> firstPage = infrastructure::ReadFirstPageRaw(path);
    const domain::EncryptionMode mode = ReadSdfEncryptionMode(path);

    if (password.empty())
    {
        if (mode != domain::EncryptionMode::None)
        {
            throw std::runtime_error("file is password-protected, password required");
        }
        return OpenedStorage{std::make_unique<infrastructure::FileStorage>(path), mode};
    }

    auto cipher = std::make_shared<SdfPageCipher>(firstPage, password);
    return OpenedStorage{std::make_unique<infrastructure::FileStorage>(path, cipher), mode};
}

}

domain::EncryptionMode ReadSdfEncryptionMode(const std::string& path)
{
    const std::vector<std::uint8_t> firstPage = infrastructure::ReadFirstPageRaw(path);
    return SdfPageCipher::ReadMode(firstPage);
}

SdfDatabaseComponents OpenSdfFile(const std::string& path, const std::string& password)
{
    OpenedStorage opened = OpenStorage(path, password);

    auto pageScanner = std::make_shared<CatalogPageScanner>();
    auto tableCatalogBuilder
        = std::make_shared<TableCatalogBuilder>(pageScanner, std::make_shared<CatalogRowDecoder>());
    auto lobChainRegistry = std::make_shared<LobChainRegistry>(*opened.storage);
    auto rowDecoder = std::make_shared<RowDecoder>(lobChainRegistry);
    auto rowFragmentReassembler = std::make_shared<RowFragmentReassembler>();

    return SdfDatabaseComponents{
        std::move(opened.storage), opened.encryptionMode, std::move(pageScanner), std::move(tableCatalogBuilder),
        std::move(lobChainRegistry), std::move(rowDecoder), std::move(rowFragmentReassembler)};
}

}
