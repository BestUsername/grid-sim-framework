cmake_minimum_required(VERSION 3.14)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(
  GridSimFramework
  VERSION 0.1
  DESCRIPTION "Reusable C++20 agent-based simulation framework"
  LANGUAGES CXX)

include(CTest)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_EXTENSIONS ON)

  set_property(GLOBAL PROPERTY USE_FOLDERS ON)

  if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
      DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
    include(GoogleTest)
  endif()

  option(BUILD_DOC "Build documentation" ON)
  if(BUILD_DOC)
    find_package(Doxygen)
    if(Doxygen_FOUND)
      add_subdirectory(docs)
    else()
      message(STATUS "Doxygen not found, not building docs")
    endif()
  else()
    message(STATUS "Not building documentation")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

if(CMAKE_BUILD_TYPE MATCHES Debug)
    message("Adding Debug flags...")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
endif()

include(${CMAKE_SOURCE_DIR}/cmake/Coverage.cmake)
initialize_coverage()

add_subdirectory(libs)
add_subdirectory(apps)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  finalize_coverage()
endif()
