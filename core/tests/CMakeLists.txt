add_executable(count_test count_test.cpp)
target_link_libraries(count_test PRIVATE libsqlce)


add_test(
    NAME count_test
    COMMAND count_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
)
