file(GLOB_RECURSE TEST_SOURCES "*.cpp")
add_executable(test_sim ${TEST_SOURCES})
target_link_libraries(test_sim gtest_main sim gtest gmock)
gtest_discover_tests(test_sim)
