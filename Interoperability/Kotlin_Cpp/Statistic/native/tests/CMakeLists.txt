# Statistic/native/tests/CMakeLists.txt
add_executable(
    statistic_tests
    test_c_api.cpp
    test_csv_reader.cpp
    test_main.cpp
    test_stats.cpp
)

target_link_libraries(statistic_tests PRIVATE statistic_core statslib)

target_compile_definitions(
    statistic_tests
    PRIVATE STATISTIC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

add_test(NAME statistic_tests COMMAND statistic_tests)
