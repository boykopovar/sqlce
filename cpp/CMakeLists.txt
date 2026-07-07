cmake_minimum_required(VERSION 3.20)
project(sdf_parser LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

add_library(sdf_parser
    src/domain/ColumnDef.cpp
    src/domain/ColumnType.cpp
    src/domain/ColumnValue.cpp
    src/domain/DateTimeValue.cpp
    src/domain/Guid.cpp
    src/domain/NumericValue.cpp
    src/domain/Row.cpp
    src/domain/TableDef.cpp
    src/infrastructure/BinaryReader.cpp
    src/infrastructure/FileStorage.cpp
    src/infrastructure/PageView.cpp
    src/parsing/CatalogPageScanner.cpp
    src/parsing/CatalogRow.cpp
    src/parsing/CatalogRowDecoder.cpp
    src/parsing/CatalogSchema.cpp
    src/parsing/LobChainRegistry.cpp
    src/parsing/RowDecoder.cpp
    src/parsing/TableCatalogBuilder.cpp
    src/parsing/TextDecoder.cpp
    src/application/SdfDatabase.cpp
)

target_include_directories(sdf_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(sdf_parser PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(sdf_parser PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(sdf_parser PRIVATE -Wall -Wextra)
endif()

add_executable(sdf_dump app/main.cpp)
target_link_libraries(sdf_dump PRIVATE sdf_parser)

if(MSVC)
    target_compile_options(sdf_dump PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(sdf_dump PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(sdf_dump PRIVATE -Wall -Wextra)
endif()

enable_testing()

add_subdirectory(tests)
