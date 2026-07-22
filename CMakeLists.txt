cmake_minimum_required(VERSION 3.20)
project(sqlce_workspace LANGUAGES CXX)

option(SQLCE_BUILD_PYTHON_BINDINGS "Build the _sqlce_native pybind11 extension module" OFF)
option(SQLCE_BUILD_WASM "Build the Emscripten wasm module" OFF)

add_subdirectory(core)

if(SQLCE_BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings/python)
endif()

if(SQLCE_BUILD_WASM)
    add_subdirectory(bindings/wasm)
    set_target_properties(sqlce_wasm PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/demo/web/src/native"
    )
endif()

