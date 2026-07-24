cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(Regex LANGUAGES CXX)

set(PROGRAM_NAME test)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SOURCES src/main.cpp)

add_executable(${PROGRAM_NAME} ${SOURCES})

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${PROGRAM_NAME} PRIVATE -freflection)
endif()
