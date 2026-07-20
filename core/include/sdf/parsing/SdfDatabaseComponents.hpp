#ifndef SDF_PARSING_SDF_DATABASE_COMPONENTS_HPP
#define SDF_PARSING_SDF_DATABASE_COMPONENTS_HPP

#include <memory>
#include <string>

#include "sdf/domain/EncryptionMode.hpp"
#include "sdf/domain/FormatVersion.hpp"
#include "sdf/domain/interfaces/IPageStorage.hpp"
#include "sdf/parsing/interfaces/ICatalogPageScanner.hpp"
#include "sdf/parsing/interfaces/ILobChainRegistry.hpp"
#include "sdf/parsing/interfaces/IRowDecoder.hpp"
#include "sdf/parsing/interfaces/IRowFragmentReassembler.hpp"
#include "sdf/parsing/interfaces/ITableCatalogBuilder.hpp"

namespace sdf::parsing
{

struct SdfDatabaseComponents
{
    std::unique_ptr<domain::IPageStorage> storage;
    domain::EncryptionMode encryptionMode;
    domain::FormatVersion formatVersion;
    std::shared_ptr<ICatalogPageScanner> pageScanner;
    std::shared_ptr<ITableCatalogBuilder> tableCatalogBuilder;
    std::shared_ptr<ILobChainRegistry> lobChainRegistry;
    std::shared_ptr<IRowDecoder> rowDecoder;
    std::shared_ptr<IRowFragmentReassembler> rowFragmentReassembler;
};

SdfDatabaseComponents OpenSdfFile(const std::string& path, const std::string& password);

domain::EncryptionMode ReadSdfEncryptionMode(const std::string& path);

domain::FormatVersion ReadSdfFormatVersion(const std::string& path);

}

#endif
