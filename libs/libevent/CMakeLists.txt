function(add_event_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    # Set coverage thresholds (can be customized)
    set(LIBEVENT_LINE_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum line coverage percentage for libevent")
    set(LIBEVENT_FUNCTION_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum function coverage percentage for libevent")

    file(GLOB SOURCES "src/*.cpp")
    add_library(event ${SOURCES})
    target_include_directories(event PUBLIC include)

    if(BUILD_TESTING) 
        add_subdirectory(tests)
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR} ${LIBEVENT_LINE_COVERAGE_THRESHOLD} ${LIBEVENT_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()

endfunction()

add_event_library()
