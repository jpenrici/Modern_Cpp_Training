cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

project(TypeParser LANGUAGES CXX)

set(PROGRAM_NAME typeParser)
set(TEST_NAME test_typeParser)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) 

include_directories(src/typeParser)

add_executable(${PROGRAM_NAME} src/main.cpp src/typeParser/typeParser.cpp)

set_target_properties(${PROGRAM_NAME} PROPERTIES
    DEBUG_POSTFIX "_d"
    RELEASE_POSTFIX ""
)

add_executable(${TEST_NAME} src/test/test.cpp src/typeParser/typeParser.cpp)

set_target_properties(${TEST_NAME} PROPERTIES
    DEBUG_POSTFIX "_d"
    RELEASE_POSTFIX ""
)

add_test(NAME run_test COMMAND ${TEST_NAME})

install(TARGETS ${PROGRAM_NAME}
    DESTINATION bin
)

install(TARGETS ${TEST_NAME}
    DESTINATION bin/tests
)

install(FILES src/typeParser/typeParser.hpp
    DESTINATION include/${PROJECT_NAME}
)

add_custom_target(cleanAll
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Removing all build artifacts and CMake cache."
)
