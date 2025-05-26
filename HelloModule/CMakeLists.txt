cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
project(HelloModule CXX)

set(PROGRAM_NAME App)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPERIMENTAL_CXX_MODULE_DIAGNOSTICS 1)

add_library(calc_module
    src/calc_module/calc.cppm
    src/calc_module/calc.cpp
)

add_executable(${PROGRAM_NAME}
    src/app/main.cpp
)

target_compile_options(calc_module PUBLIC -fmodules)
target_compile_options(${PROGRAM_NAME} PRIVATE -fmodules)

target_link_libraries(${PROGRAM_NAME} PRIVATE calc_module)

install(TARGETS ${PROGRAM_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
