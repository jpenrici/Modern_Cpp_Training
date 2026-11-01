# Shared, header-only interface library + error dictionary generation.

find_package(Perl REQUIRED)

set(FFI_GEN_SCRIPT "${CMAKE_SOURCE_DIR}/scripts/gen_errors.pl")
set(FFI_ERROR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/error_codes.txt")

# Re-run CMake (and therefore the generator) whenever the source of truth changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${FFI_ERROR_SOURCE}" "${FFI_GEN_SCRIPT}")

execute_process(
    COMMAND "${PERL_EXECUTABLE}" "${FFI_GEN_SCRIPT}"
    RESULT_VARIABLE FFI_GEN_RESULT
    OUTPUT_VARIABLE FFI_GEN_OUTPUT
    ERROR_VARIABLE FFI_GEN_ERROR)
if(NOT FFI_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "gen_errors.pl failed: ${FFI_GEN_ERROR}")
endif()

add_library(ffi_common INTERFACE)
add_library(ffi_bridge::common ALIAS ffi_common)
target_include_directories(ffi_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
