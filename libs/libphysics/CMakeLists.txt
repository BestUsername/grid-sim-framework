function(add_physics_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    set(LIBPHYSICS_LINE_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum line coverage percentage for libphysics")
    set(LIBPHYSICS_FUNCTION_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum function coverage percentage for libphysics")

    file(GLOB SOURCES "src/*.cpp")
    add_library(physics ${SOURCES})
    target_include_directories(physics PUBLIC include)
    target_link_libraries(physics PUBLIC event)

    if(BUILD_TESTING)
        add_subdirectory(tests)
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR} ${LIBPHYSICS_LINE_COVERAGE_THRESHOLD} ${LIBPHYSICS_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()

endfunction()

add_physics_library()
