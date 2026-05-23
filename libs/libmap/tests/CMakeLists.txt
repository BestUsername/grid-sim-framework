file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(test_map ${TEST_SOURCES})
target_link_libraries(test_map gtest_main map gtest gmock)
target_compile_definitions(test_map PRIVATE
    LIBMAP_TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
gtest_discover_tests(test_map)
