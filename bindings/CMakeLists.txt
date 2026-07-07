find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(_sdf_native
    src/module.cpp
    src/value_convert.cpp
)

target_link_libraries(_sdf_native PRIVATE sdf_parser)

if(MSVC)
    target_compile_options(_sdf_native PRIVATE /W4 /utf-8 /permissive-)
    target_compile_definitions(_sdf_native PRIVATE NOMINMAX _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(_sdf_native PRIVATE -Wall -Wextra)
endif()

install(TARGETS _sdf_native LIBRARY DESTINATION sdf)
