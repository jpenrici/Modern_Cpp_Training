cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(TypeEnsure_Test LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(main main.cpp)
