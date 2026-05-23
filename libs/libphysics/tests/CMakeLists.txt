set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(test_physics
    test_kinematic_body.cpp
    test_physics_world.cpp
)

target_link_libraries(test_physics PRIVATE physics gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(test_physics)
