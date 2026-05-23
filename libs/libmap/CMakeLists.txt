function(add_map_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    set(LIBMAP_LINE_COVERAGE_THRESHOLD     80 CACHE STRING "Minimum line coverage % for libmap")
    set(LIBMAP_FUNCTION_COVERAGE_THRESHOLD 80 CACHE STRING "Minimum function coverage % for libmap")

    # ── tinyxml2 (OSM XML parsing) ────────────────────────────────────────────
    find_package(tinyxml2 QUIET)
    if(NOT tinyxml2_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            tinyxml2
            GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
            GIT_TAG        10.0.0
            GIT_SHALLOW    TRUE
        )
        FetchContent_MakeAvailable(tinyxml2)
        # FetchContent builds a plain 'tinyxml2' target; alias it so we can
        # always refer to the namespaced form tinyxml2::tinyxml2.
        if(NOT TARGET tinyxml2::tinyxml2)
            add_library(tinyxml2::tinyxml2 ALIAS tinyxml2)
        endif()
    endif()

    file(GLOB SOURCES "src/*.cpp")
    add_library(map ${SOURCES})
    target_include_directories(map PUBLIC include)
    target_compile_features(map PUBLIC cxx_std_20)
    target_link_libraries(map PUBLIC tinyxml2::tinyxml2)

    if(BUILD_TESTING)
        add_subdirectory(tests)
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR}
            ${LIBMAP_LINE_COVERAGE_THRESHOLD}
            ${LIBMAP_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()
endfunction()

add_map_library()
