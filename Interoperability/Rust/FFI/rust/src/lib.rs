//! Rust side of the C++23 <-> Rust FFI bridge.
//!
//! * `string_ops`  - pure business logic working on borrowed `&str` / owned `String`.
//! * `errors`      - HashMap-based error dictionary built from the generated table.
//! * `callback`    - registry for the C++ log callback (Rust -> C++ direction).
//! * `ffi`         - the `extern "C"` surface declared in `common/include/ffi_bridge/ffi_api.h`.

pub mod callback;
pub mod error_codes;
pub mod errors;
pub mod ffi;
pub mod string_ops;
