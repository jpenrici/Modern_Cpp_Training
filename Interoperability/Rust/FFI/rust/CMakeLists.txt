# Builds the Rust cdylib with cargo and exposes it as the imported target `ffi_rust`.

find_program(CARGO_EXECUTABLE cargo REQUIRED)

set(RUST_TARGET_DIR "${CMAKE_BINARY_DIR}/rust-target")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(RUST_PROFILE_DIR debug)
    set(RUST_PROFILE_ARGS "")
else()
    set(RUST_PROFILE_DIR release)
    set(RUST_PROFILE_ARGS --release)
endif()

set(RUST_LIBRARY "${RUST_TARGET_DIR}/${RUST_PROFILE_DIR}/libffi_rust.so")
set(RUST_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml")

file(GLOB_RECURSE RUST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs")

add_custom_command(
    OUTPUT "${RUST_LIBRARY}"
    COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${RUST_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" build ${RUST_PROFILE_ARGS} --manifest-path "${RUST_MANIFEST}"
    DEPENDS ${RUST_SOURCES} "${RUST_MANIFEST}" "${CMAKE_CURRENT_SOURCE_DIR}/build.rs"
    COMMENT "Building Rust cdylib (cargo, ${RUST_PROFILE_DIR})"
    VERBATIM)
add_custom_target(ffi_rust_build DEPENDS "${RUST_LIBRARY}")

add_library(ffi_rust SHARED IMPORTED GLOBAL)
set_target_properties(ffi_rust PROPERTIES
    IMPORTED_LOCATION "${RUST_LIBRARY}"
    IMPORTED_SONAME "libffi_rust.so")
add_dependencies(ffi_rust ffi_rust_build)

add_test(
    NAME rust_tests
    COMMAND ${CMAKE_COMMAND} -E env "CARGO_TARGET_DIR=${RUST_TARGET_DIR}"
            "${CARGO_EXECUTABLE}" test ${RUST_PROFILE_ARGS} --manifest-path "${RUST_MANIFEST}")
