cmake_minimum_required(VERSION 3.20)
project(sdf_parser_workspace LANGUAGES CXX)

option(SDF_BUILD_PYTHON_BINDINGS "Build the _sdf_native pybind11 extension module" OFF)

add_subdirectory(core)

if(SDF_BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings)
endif()
