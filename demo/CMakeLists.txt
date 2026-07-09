if(SDF_BUILD_WASM)
    add_custom_command(TARGET sqlce_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sqlce_wasm>
            "${CMAKE_CURRENT_SOURCE_DIR}/web/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE_DIR:sqlce_wasm>/sqlce_wasm.wasm"
            "${CMAKE_CURRENT_SOURCE_DIR}/web/"
    )
endif()
