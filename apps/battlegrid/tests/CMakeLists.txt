set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(test_battlegrid
    test_health.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/soldier.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/civilian.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/vehicle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/terrain.cpp
)

target_compile_features(test_battlegrid PRIVATE cxx_std_20)
target_include_directories(test_battlegrid PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
target_link_libraries(test_battlegrid PRIVATE sim physics gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(test_battlegrid)
