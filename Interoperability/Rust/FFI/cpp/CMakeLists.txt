# Idiomatic C++23 wrapper library, demo executable and tests.

add_library(ffi_cpp STATIC
    src/error_dict.cpp
    src/rust_string.cpp
    src/borrowing.cpp
    src/logging.cpp)
add_library(ffi_bridge::cpp ALIAS ffi_cpp)

target_include_directories(ffi_cpp
    PUBLIC include
    PRIVATE src)
target_compile_features(ffi_cpp PUBLIC cxx_std_23)
target_compile_options(ffi_cpp PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(ffi_cpp PUBLIC ffi_common ffi_rust)
add_dependencies(ffi_cpp ffi_rust_build)

function(ffi_add_executable name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE ffi_cpp)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
    # Let the executable find libffi_rust.so in the build tree at runtime.
    set_target_properties(${name} PROPERTIES BUILD_RPATH "$<TARGET_FILE_DIR:ffi_rust>")
endfunction()

ffi_add_executable(ffi_bridge_demo src/main.cpp)
ffi_add_executable(ffi_cpp_tests tests/test_main.cpp)

add_test(NAME cpp_tests COMMAND ffi_cpp_tests)
