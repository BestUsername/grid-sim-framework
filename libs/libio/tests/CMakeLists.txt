set(BASIC_TEST_SOURCES "test_io_basic.cpp")
set(SDL_TEST_SOURCES "test_sdl_devices.cpp")

# Basic tests (event types, keycodes) always build
add_executable(test_libio_basic ${BASIC_TEST_SOURCES})
target_link_libraries(test_libio_basic gtest_main gtest)
target_include_directories(test_libio_basic PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
gtest_discover_tests(test_libio_basic)

# SDL device tests need SDL2 and the io library
if(SDL2_FOUND)
    add_executable(test_libio_sdl ${SDL_TEST_SOURCES})
    target_link_libraries(test_libio_sdl gtest_main gtest io)
    gtest_discover_tests(test_libio_sdl)
else()
    message(STATUS "Skipping SDL libio tests - SDL2 not found")
endif()

# Evdev keymap tests (Linux only, no root needed — just tests the mapping)
if(HAVE_LINUX_INPUT_H)
    add_executable(test_libio_evdev "test_evdev_keymap.cpp")
    target_link_libraries(test_libio_evdev gtest_main gtest io)
    gtest_discover_tests(test_libio_evdev)
else()
    message(STATUS "Skipping evdev libio tests - linux/input.h not found")
endif()

if(CURSES_FOUND)
    add_executable(test_libio_ncurses "test_ncurses_keymap.cpp")
    target_link_libraries(test_libio_ncurses gtest_main gtest io)
    gtest_discover_tests(test_libio_ncurses)
else()
    message(STATUS "Skipping NCurses libio tests - Curses not found")
endif()
