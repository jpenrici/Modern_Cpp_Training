add_executable(${TEST_NAME} smoke_test.cpp)
target_link_libraries(${TEST_NAME} PRIVATE ${PROGRAM_MODULES} Threads::Threads)
target_link_options(${TEST_NAME} PRIVATE -static-libstdc++ -static-libgcc)

add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
