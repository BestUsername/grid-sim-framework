find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_executable(battlegrid
    src/main.cpp
    src/terrain.cpp
    src/soldier.cpp
    src/civilian.cpp
    src/vehicle.cpp
    src/input_map.cpp
    src/player_controller.cpp
    src/authority_registry.cpp
    src/battlegrid_world.cpp
    src/gl_display.cpp
    src/map_overlay.cpp
)

target_compile_features(battlegrid PRIVATE cxx_std_20)
target_link_libraries(battlegrid PRIVATE sim io physics net map ${SDL2_LIBRARIES} OpenGL::GL GLEW::GLEW)
target_include_directories(battlegrid PRIVATE include ${SDL2_INCLUDE_DIRS})

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
