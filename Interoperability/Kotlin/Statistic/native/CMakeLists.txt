# Statistic/native/CMakeLists.txt
#
# Targets: statistic_core       (STATIC) - C++23 internals: statistics + tolerant CSV
# reader. statslib              (SHARED) - libstatslib.so, the API consumed by
# Kotlin/Native. statistic_test (EXE)    - Unit + integration tests (ctest).
#
# Usage:
# cmake -S native -B native/build
# cmake --build native/build -j
# ctest --test-dir native/build --output-on-failure
#
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(
  Statistic
  VERSION 1.0.0
  LANGUAGES CXX)

option(STATISTIC_BUILD_TESTS "Build the test executable" ON)
option(STATISTIC_ENABLE_SANITIZERS "Builf with ASan + UBSan" OFF)
set(STATISTIC_LIB_OUTPUT_DIR
    "${PROJECT_SOURCE_DIR}/../lib"
    CACHE PATH "Where libstatslib.so is placed")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clang, IntelliJ, CLion

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE
      Release
      CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION
                                            VERSION_LESS 13)
  message(
    FATAL_ERROR
      "GCC 13 or newer is required (found ${CMAKE_CXX_COMPILER_VERSION})")
endif()

add_library(statistic_options INTERFACE)
target_compile_options(statistic_options INTERFACE -Wall -Wextra -Wpedantic
                                                   -Wconversion -Wshadow)

if(STATISTIC_ENABLE_SANITIZERS)
  target_compile_options(
    statistic_options INTERFACE -fsanitize=address,undefined
                                -fno-omit-frame-pointer)
  target_link_options(statistic_options INTERFACE -fsanitize=address,undefined)
endif()

add_library(statistic_core STATIC src/core/stats.cpp src/io/csv_reader.cpp)
target_include_directories(statistic_core PUBLIC src) # internal headers
target_compile_features(statistic_core PUBLIC cxx_std_23)
target_link_libraries(statistic_core PUBLIC statistic_options)
set_target_properties(statistic_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(statslib SHARED src/c_api/stats_c_api.cpp)
target_include_directories(statslib PUBLIC include) # public C header only
target_link_libraries(statslib PRIVATE statistic_core)
set_target_properties(
  statslib
  PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${STATISTIC_LIB_OUTPUT_DIR}"
             BUILD_RPATH "$ORIGIN"
             INSTALL_RPATH "$ORIGIN")

if(STATISTIC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
