file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(test_libevent ${TEST_SOURCES})
target_link_libraries(test_libevent gtest_main gtest gmock event)

gtest_discover_tests(test_libevent)
