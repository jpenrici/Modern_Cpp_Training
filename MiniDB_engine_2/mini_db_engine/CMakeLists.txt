cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(
  mini_db_engine
  VERSION 0.1.0
  DESCRIPTION "mini_db_engine"
  LANGUAGES CXX)

set(PROGRAM_NAME mini_db_engine)
set(PROGRAM_MODULES ${PROGRAM_NAME}_modules)
set(TEST_NAME smoke_test)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(
    WARNING "${PROGRAM_NAME} relies on C++ modules; configure with -G Ninja "
            "for reliable module dependency scanning.")
endif()

add_library(${PROGRAM_NAME}_modules STATIC)

target_sources(
  ${PROGRAM_NAME}_modules
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         src/db-concurrency.cppm
         src/db-core.cppm
         src/db-engine.cppm
         src/db-exec.cppm
         src/db-index.cppm
         src/db-storage.cppm
         src/db-wal.cppm
         src/db.cppm)

add_executable(${PROGRAM_NAME} src/main.cpp)

target_link_libraries(${PROGRAM_NAME} PRIVATE ${PROGRAM_MODULES}
                                              Threads::Threads)
target_compile_options(${PROGRAM_MODULES} PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(${PROGRAM_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(${PROGRAM_NAME} PRIVATE -static-libstdc++ -static-libgcc)

if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
  enable_testing()
  add_subdirectory(tests)
endif()
