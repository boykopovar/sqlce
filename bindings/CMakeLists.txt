if(POLICY CMP0148)
    cmake_policy(SET CMP0148 NEW)
endif()

find_package(pybind11 CONFIG REQUIRED)
pybind11_add_module(_sdf_native
        src/module.cpp
        src/value_convert.cpp
)

target_compile_features(_sdf_native PRIVATE cxx_std_20)
set_target_properties(_sdf_native PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

target_link_libraries(_sdf_native PRIVATE libsdf)

if(MINGW)
    target_link_options(_sdf_native PRIVATE -static-libgcc -static-libstdc++ -static -lwinpthread)
endif()

if(MSVC)
    target_compile_options(_sdf_native PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(_sdf_native PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(_sdf_native PRIVATE -Wall -Wextra)
endif()

install(TARGETS _sdf_native LIBRARY DESTINATION sqlce)
