add_executable(sqlce_wasm wasm_bindings.cpp)

target_link_libraries(sqlce_wasm PRIVATE libsqlce)
target_compile_features(sqlce_wasm PRIVATE cxx_std_20)
set_target_properties(sqlce_wasm PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)

target_compile_options(sqlce_wasm PRIVATE -fexceptions)

target_link_options(sqlce_wasm PRIVATE
        "--bind"
        "-fexceptions"
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=createSqlceModule"
        "-sEXPORTED_RUNTIME_METHODS=[FS,ccall,cwrap]"
        "-sFORCE_FILESYSTEM=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sGROWABLE_ARRAYBUFFERS=0"
        "-sENVIRONMENT=web"
)

set_target_properties(sqlce_wasm PROPERTIES SUFFIX ".js")
