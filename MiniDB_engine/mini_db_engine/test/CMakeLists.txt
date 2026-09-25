add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE mini_db_engine_modules
                                         Threads::Threads)

add_test(NAME smoke_test COMMAND smoke_test)
