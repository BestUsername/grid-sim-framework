function(add_net_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    set(LIBNET_LINE_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum line coverage percentage for libnet")
    set(LIBNET_FUNCTION_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum function coverage percentage for libnet")

    find_package(Boost REQUIRED COMPONENTS system)

    # Header-only interface library with a single translation unit for
    # any non-template symbols we may add later.
    file(GLOB SOURCES "src/*.cpp")
    if(NOT SOURCES)
        # Purely header-only for now — create an INTERFACE target.
        add_library(net INTERFACE)
        target_include_directories(net INTERFACE include)
        target_link_libraries(net INTERFACE event Boost::system)
    else()
        add_library(net ${SOURCES})
        target_include_directories(net PUBLIC include)
        target_link_libraries(net PUBLIC event Boost::system)
    endif()

    if(BUILD_TESTING)
        add_subdirectory(tests)
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR} ${LIBNET_LINE_COVERAGE_THRESHOLD} ${LIBNET_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()

endfunction()

add_net_library()
