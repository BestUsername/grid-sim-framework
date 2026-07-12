if(BUILD_TESTING)
    add_executable(net_tests
        message_test.cpp
        serializer_test.cpp
        transport_test.cpp
    )
    target_link_libraries(net_tests PRIVATE net event gtest gtest_main)
    target_include_directories(net_tests PRIVATE ${CMAKE_SOURCE_DIR}/libs/libnet/include)

    include(GoogleTest)
    gtest_discover_tests(net_tests)
endif()
