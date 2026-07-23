cmake_minimum_required(VERSION 3.20)
project(libsqlce LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

add_library(libsqlce
        src/domain/ColumnDef.cpp
        src/domain/ColumnType.cpp
        src/domain/ColumnValue.cpp
        src/domain/DateTimeValue.cpp
        src/domain/Guid.cpp
        src/domain/LazyLob.cpp
        src/domain/NumericValue.cpp
        src/domain/Row.cpp
        src/domain/TableDef.cpp
        src/domain/TextDecoder.cpp
        src/infrastructure/AesCbc.cpp
        src/infrastructure/BinaryReader.cpp
        src/infrastructure/FileStorage.cpp
        src/infrastructure/PageChecksum.cpp
        src/infrastructure/Sha1.cpp
        src/infrastructure/Sha1KeyDerivation.cpp
        src/infrastructure/Sha256.cpp
        src/infrastructure/Sha512.cpp
        src/infrastructure/TripleDesCbc.cpp
        src/parsing/CatalogPageScanner.cpp
        src/parsing/CatalogRow.cpp
        src/parsing/CatalogRowDecoder.cpp
        src/parsing/CatalogSchema.cpp
        src/parsing/LobChainRegistry.cpp
        src/parsing/PageView.cpp
        src/parsing/RowDecoder.cpp
        src/parsing/RowFragmentReassembler.cpp
        src/parsing/SdfDatabaseComponents.cpp
        src/parsing/SdfPageCipher.cpp
        src/parsing/TableCatalogBuilder.cpp
        src/application/SqlceDatabase.cpp
        src/application/TableRowRange.cpp
)

set_target_properties(libsqlce PROPERTIES
        PREFIX ""
        OUTPUT_NAME "libsqlce"
        POSITION_INDEPENDENT_CODE ON
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist/lib"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist/lib"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist/lib"
)

target_include_directories(libsqlce PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(libsqlce PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(libsqlce PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(libsqlce PRIVATE -Wall -Wextra -Wno-free-nonheap-object)
else()
    target_compile_options(libsqlce PRIVATE -Wall -Wextra)
endif()

add_executable(sdf_dump main.cpp)
target_link_libraries(sdf_dump PRIVATE libsqlce)

if(MSVC)
    target_compile_options(sdf_dump PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(sdf_dump PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(sdf_dump PRIVATE -Wall -Wextra)
    if(MINGW)
        target_link_options(sdf_dump PRIVATE
                -static
                -static-libgcc
                -static-libstdc++
                -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive
        )
    endif()
endif()

enable_testing()

add_subdirectory(tests)
