function(add_physics_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    include(FetchContent)
    FetchContent_Declare(
        box3d
        GIT_REPOSITORY https://github.com/erincatto/box3d.git
        GIT_TAG v0.1.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(box3d)

    set(LIBPHYSICS_LINE_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum line coverage percentage for libphysics")
    set(LIBPHYSICS_FUNCTION_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum function coverage percentage for libphysics")

    file(GLOB SOURCES "src/*.cpp")
    add_library(physics ${SOURCES})
    target_include_directories(physics PUBLIC include)
    target_link_libraries(physics PUBLIC event box3d::box3d)

    if(BUILD_TESTING)
        add_subdirectory(tests)
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR} ${LIBPHYSICS_LINE_COVERAGE_THRESHOLD} ${LIBPHYSICS_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()

endfunction()

add_physics_library()
