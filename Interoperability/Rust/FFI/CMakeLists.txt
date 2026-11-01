cmake_minimum_required(VERSION 3.25)
project(ffi_bridge VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "ffi_bridge supports Linux only")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Feature check: the toolchain must provide std::expected (C++23).
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
#include <expected>
int main() { std::expected<int, int> e{1}; return *e - 1; }
" FFI_HAVE_STD_EXPECTED)
if(NOT FFI_HAVE_STD_EXPECTED)
    message(FATAL_ERROR "The C++ toolchain does not provide <expected> (need GCC 13+/Clang 17+)")
endif()

enable_testing()

add_subdirectory(common)
add_subdirectory(rust)
add_subdirectory(cpp)
