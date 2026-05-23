function(add_io_library)
    if(BUILD_TESTING)
        include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
        initialize_coverage()
    endif()

    set(IO_SRC "")
    set(IO_INC "include")
    set(IO_LIB "")

    set(CURSES_NEED_WIDE TRUE)
    set(CURSES_NEED_NCURSES TRUE)
    find_package(Curses)
    if (CURSES_FOUND)
        message("Adding NCurses support to libIO")
        list(APPEND IO_SRC "src/ncurses_keyboard.cpp" "src/ncurses_keymap.cpp")
        list(APPEND IO_LIB ${CURSES_LIBRARIES})
        list(APPEND IO_INC ${CURSES_INCLUDE_DIRS})
    endif()

    find_package(SDL2)
    if (SDL2_FOUND)
        message("Adding SDL2 support to libIO")
        list(APPEND IO_SRC
            "src/sdl_keyboard.cpp"
            "src/sdl_mouse.cpp"
            "src/sdl_gamepad.cpp"
            "src/sdl_keymap.cpp")
        list(APPEND IO_LIB ${SDL2_LIBRARIES})
        list(APPEND IO_INC ${SDL2_INCLUDE_DIRS})
    endif()

    # Linux evdev: check for linux/input.h
    include(CheckIncludeFileCXX)
    check_include_file_cxx("linux/input.h" HAVE_LINUX_INPUT_H)
    if (HAVE_LINUX_INPUT_H)
        message("Adding Linux evdev support to libIO")
        list(APPEND IO_SRC "src/evdev_keyboard.cpp" "src/evdev_keymap.cpp")
    endif()

    add_library(io "${IO_SRC}")
    target_link_libraries(io ${IO_LIB})
    target_include_directories(io PUBLIC ${IO_INC})

    if(BUILD_TESTING)
        add_subdirectory(tests)
        # Set coverage thresholds (lower for libio due to hardware dependencies)
        set(LIBIO_LINE_COVERAGE_THRESHOLD 50 CACHE STRING "Minimum line coverage percentage for libio")
        set(LIBIO_FUNCTION_COVERAGE_THRESHOLD 50 CACHE STRING "Minimum function coverage percentage for libio")
        add_directory_to_coverage(${CMAKE_CURRENT_SOURCE_DIR} ${LIBIO_LINE_COVERAGE_THRESHOLD} ${LIBIO_FUNCTION_COVERAGE_THRESHOLD})
        if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
            finalize_coverage()
        endif()
    endif()
endfunction()

add_io_library()
