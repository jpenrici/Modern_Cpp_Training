cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(GameCoroutineDemo LANGUAGES CXX)

set(PROGRAM_NAME game_demo)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(Threads REQUIRED)
set(DEMO_RUNTIME_LINK_OPTIONS -static-libstdc++ -static-libgcc)

add_executable(${PROGRAM_NAME} src/main.cpp)

target_compile_options(${PROGRAM_NAME} PRIVATE -Wall -Wextra -Wpedantic -O3)
target_link_options(${PROGRAM_NAME} PRIVATE ${DEMO_RUNTIME_LINK_OPTIONS})
