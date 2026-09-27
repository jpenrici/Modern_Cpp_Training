cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(mini_db_engine VERSION 1.0.0 DESCRIPTION "Mini database engine for study purposes." LANGUAGES CXX)

set(PROGRAM_NAME minidb)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if (NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(WARNING
        "${PROGRAM_NAME} relies on C++ modules; configure with -G Ninja "
        "for reliable module dependency scanning.")
endif()

add_library(${PROGRAM_NAME}_modules STATIC)

target_sources(${PROGRAM_NAME}_modules
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/db.cppm
            src/db-exec.cppm
            src/db-storage.cppm
)

add_executable(${PROGRAM_NAME} src/main.cpp)

target_link_libraries(${PROGRAM_NAME} PRIVATE ${PROGRAM_NAME}_modules Threads::Threads)
target_compile_options(${PROGRAM_NAME}_modules PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(${PROGRAM_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(${PROGRAM_NAME} PRIVATE -static-libstdc++ -static-libgcc)
