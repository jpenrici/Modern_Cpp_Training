cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(mini_db_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Place every executable directly under <project_root>/bin, regardless of build
# type or generator, so binaries can be run without reaching into the build/
# tree. This is a placeholder convenience only -- a real `install()` rule /
# staged installation is expected to replace it later.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# C++20/23 module dependency scanning (P1689). Reliable support currently
# requires the Ninja generator (or MSVC) -- not Unix Makefiles.
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if(NOT CMAKE_GENERATOR MATCHES "Ninja")
  message(
    WARNING "mini_db_engine relies on C++ modules; configure with -G Ninja "
            "for reliable module dependency scanning.")
endif()

# All module partitions live in a single library target so that both the main
# executable and the test executable can `import db;` without the module graph
# being scanned/compiled twice.
add_library(mini_db_engine_modules STATIC)

target_sources(
  mini_db_engine_modules
  PUBLIC FILE_SET
         CXX_MODULES
         FILES
         src/db.cppm
         src/db-core.cppm
         src/db-storage.cppm
         src/db-index.cppm
         src/db-wal.cppm
         src/db-concurrency.cppm
         src/db-exec.cppm
         src/db-engine.cppm)

target_compile_options(mini_db_engine_modules PRIVATE -Wall -Wextra -Wpedantic)

# db:index's demos/tests use std::thread (the module itself does not).
find_package(Threads REQUIRED)

set(MINI_DB_ENGINE_RUNTIME_LINK_OPTIONS -static-libstdc++ -static-libgcc)

add_executable(mini_db_engine src/main.cpp)
target_link_libraries(mini_db_engine PRIVATE mini_db_engine_modules
                                             Threads::Threads)
target_link_options(mini_db_engine PRIVATE
                    ${MINI_DB_ENGINE_RUNTIME_LINK_OPTIONS})

enable_testing()
add_subdirectory(test)
