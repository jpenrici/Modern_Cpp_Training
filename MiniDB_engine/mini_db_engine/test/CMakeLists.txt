add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE mini_db_engine_modules
                                         Threads::Threads)
target_link_options(smoke_test PRIVATE -static-libstdc++ -static-libgcc)

add_test(NAME smoke_test COMMAND smoke_test)
