# FFI

Bidirectional C-ABI FFI between C++23 and Rust (standard libraries only).

* Rust -> C++: strings allocated by Rust (`ffi_to_uppercase`), released with `ffi_free_string`.
* C++ -> Rust: borrowed strings (`ffi_count_chars`) and a mutable buffer (`ffi_append_suffix`).
* Rust -> C++ callbacks: C++ registers a log callback (`ffi_set_log_callback`) that Rust invokes.
* Error codes: single source `common/error_codes.txt`, turned into a C header, a Rust module
  and a C++ table by `scripts/gen_errors.pl`. Each language keeps its own dictionary
  (`std::unordered_map` / `HashMap`).

## Layout

    common/    shared C header (ffi_api.h), error source of truth and generated error_codes.h
    rust/      cdylib crate (libffi_rust.so) + integration tests in rust/tests
    cpp/       idiomatic C++23 wrappers, demo executable, tests in cpp/tests
    scripts/   manage.pl (setup|check|build|test|run|demo|clean) and gen_errors.pl

## Quick start

    perl scripts/manage.pl check
    perl scripts/manage.pl build
    perl scripts/manage.pl test
    perl scripts/manage.pl demo

`manage.pl run` invokes `gen_errors.pl` (regenerates the error dictionaries;
pass extra arguments after `--`, e.g. `manage.pl run -- --check`).

## Contract summary

* Strings are `(const char* ptr, size_t len)`: NOT NUL-terminated, `len` in bytes, UTF-8 required.
* Whoever allocates, frees: buffers returned by Rust must go back to `ffi_free_string(ptr, len)` exactly once.
* Input pointers are borrowed for the duration of the call only (also valid for callback messages).
* No panic or exception crosses the boundary (`catch_unwind` in Rust, `try/catch` in the C++ trampoline).
* Every function returns an `int32_t` code; look the message up in the local dictionary.

Requirements: Linux, Perl 5.40, CMake 3.25+, GCC 13+/Clang 17+, Rust 1.85+ (edition 2024).
