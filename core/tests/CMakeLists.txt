add_executable(count_test count_test.cpp)
target_link_libraries(count_test PRIVATE libsqlce)


add_test(
    NAME count_test
    COMMAND count_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)

add_executable(binary_reader_test binary_reader_test.cpp)
target_link_libraries(binary_reader_test PRIVATE libsqlce)
add_test(NAME binary_reader_test COMMAND binary_reader_test)

add_executable(page_view_test page_view_test.cpp)
target_link_libraries(page_view_test PRIVATE libsqlce)
add_test(NAME page_view_test COMMAND page_view_test)

add_executable(lob_chain_test lob_chain_test.cpp)
target_link_libraries(lob_chain_test PRIVATE libsqlce)
add_test(NAME lob_chain_test COMMAND lob_chain_test)
