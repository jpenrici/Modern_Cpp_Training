//! The `extern "C"` surface. Prototypes live in `common/include/ffi_bridge/ffi_api.h`.
//!
//! Rules applied to every exported function:
//! * the body runs inside `guard`, so a panic becomes `INTERNAL_PANIC` instead of unwinding into C++;
//! * raw pointers are validated before use and converted into borrowed Rust types;
//! * the result is reported to the registered log callback and returned as a numeric code.

use std::ffi::{c_char, c_void};
use std::panic::{catch_unwind, AssertUnwindSafe};

use crate::callback::{self, LogCallback};
use crate::error_codes as ec;
use crate::errors;
use crate::string_ops;

/// Runs `f`, converting any panic into `INTERNAL_PANIC`.
pub fn guard<F: FnOnce() -> i32>(f: F) -> i32 {
    catch_unwind(AssertUnwindSafe(f)).unwrap_or(ec::INTERNAL_PANIC)
}

/// Logs the outcome of `op` through the C++ callback and returns `code`.
fn finish(op: &str, code: i32) -> i32 {
    let level = if code == ec::OK { 0 } else { 1 };
    callback::emit(level, &format!("{op}: {} ({code})", errors::message(code)));
    code
}

/// Builds a borrowed `&str` from a (ptr, len) pair, validating pointer, size and UTF-8.
///
/// # Safety
/// `ptr` must be null or point to `len` readable bytes that outlive the returned reference.
unsafe fn str_from_raw<'a>(ptr: *const c_char, len: usize) -> Result<&'a str, i32> {
    if ptr.is_null() {
        return Err(ec::NULL_POINTER);
    }
    if len > string_ops::MAX_LEN {
        return Err(ec::BUFFER_OVERFLOW);
    }
    let bytes = unsafe { std::slice::from_raw_parts(ptr as *const u8, len) };
    std::str::from_utf8(bytes).map_err(|_| ec::INVALID_UTF8)
}

/// Rust allocates, caller owns: release with `ffi_free_string`.
///
/// # Safety
/// `input` must reference `input_len` readable bytes; `out` and `out_len` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ffi_to_uppercase(
    input: *const c_char,
    input_len: usize,
    out: *mut *mut c_char,
    out_len: *mut usize,
) -> i32 {
    guard(|| {
        const OP: &str = "to_uppercase";
        if out.is_null() || out_len.is_null() {
            return finish(OP, ec::NULL_POINTER);
        }
        unsafe {
            *out = std::ptr::null_mut();
            *out_len = 0;
        }
        let code =
            match unsafe { str_from_raw(input, input_len) }.and_then(string_ops::to_uppercase) {
                Ok(upper) => {
                    // Exact-capacity allocation so (ptr, len) is enough to free it later.
                    let boxed: Box<str> = upper.into_boxed_str();
                    let len = boxed.len();
                    let raw = Box::into_raw(boxed) as *mut u8 as *mut c_char;
                    unsafe {
                        *out = raw;
                        *out_len = len;
                    }
                    ec::OK
                }
                Err(code) => code,
            };
        finish(OP, code)
    })
}

/// Immutable borrow: counts Unicode scalar values of the borrowed input.
///
/// # Safety
/// `input` must reference `input_len` readable bytes; `out_count` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ffi_count_chars(
    input: *const c_char,
    input_len: usize,
    out_count: *mut usize,
) -> i32 {
    guard(|| {
        const OP: &str = "count_chars";
        if out_count.is_null() {
            return finish(OP, ec::NULL_POINTER);
        }
        unsafe { *out_count = 0 };
        let code = match unsafe { str_from_raw(input, input_len) }.and_then(string_ops::count_chars)
        {
            Ok(count) => {
                unsafe { *out_count = count };
                ec::OK
            }
            Err(code) => code,
        };
        finish(OP, code)
    })
}

/// Mutable borrow: appends `suffix` into the caller's buffer.
///
/// # Safety
/// `buf` must be valid for reads and writes of `cap` initialized bytes, `len` valid for
/// reads and writes, and `suffix` must reference `suffix_len` readable bytes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ffi_append_suffix(
    buf: *mut c_char,
    len: *mut usize,
    cap: usize,
    suffix: *const c_char,
    suffix_len: usize,
) -> i32 {
    guard(|| {
        const OP: &str = "append_suffix";
        if buf.is_null() || len.is_null() {
            return finish(OP, ec::NULL_POINTER);
        }
        if cap > string_ops::MAX_LEN {
            return finish(OP, ec::BUFFER_OVERFLOW);
        }
        // Rust's aliasing rule: a mutable borrow excludes any other live borrow.
        let buf_start = buf as usize;
        let suffix_start = suffix as usize;
        if suffix_len > 0
            && suffix_start < buf_start.saturating_add(cap)
            && buf_start < suffix_start.saturating_add(suffix_len)
        {
            return finish(OP, ec::ALIASED_BORROW);
        }
        let suffix_str = match unsafe { str_from_raw(suffix, suffix_len) } {
            Ok(s) => s,
            Err(code) => return finish(OP, code),
        };
        let current = unsafe { *len };
        let slice = unsafe { std::slice::from_raw_parts_mut(buf as *mut u8, cap) };
        match string_ops::append_suffix(slice, current, suffix_str) {
            Ok(new_len) => {
                unsafe { *len = new_len };
                finish(OP, ec::OK)
            }
            Err(code) => finish(OP, code),
        }
    })
}

/// Destructor for buffers returned by `ffi_to_uppercase`.
///
/// # Safety
/// `ptr`/`len` must be exactly a pair returned by this library, released at most once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ffi_free_string(ptr: *mut c_char, len: usize) {
    if ptr.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let slice = std::ptr::slice_from_raw_parts_mut(ptr as *mut u8, len);
        drop(unsafe { Box::from_raw(slice as *mut str) });
    }));
}

/// Registers (or removes, when `cb` is NULL) the C++ log callback.
#[unsafe(no_mangle)]
pub extern "C" fn ffi_set_log_callback(cb: Option<LogCallback>, user_data: *mut c_void) -> i32 {
    guard(|| {
        callback::set(cb, user_data);
        ec::OK
    })
}

/// Exposes Rust's own dictionary entry (static storage, never freed by the caller).
///
/// # Safety
/// `out` and `out_len` must be valid for writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn ffi_error_message(
    code: i32,
    out: *mut *const c_char,
    out_len: *mut usize,
) -> i32 {
    guard(|| {
        if out.is_null() || out_len.is_null() {
            return ec::NULL_POINTER;
        }
        match errors::lookup(code) {
            Some(message) => {
                unsafe {
                    *out = message.as_ptr() as *const c_char;
                    *out_len = message.len();
                }
                ec::OK
            }
            None => {
                unsafe {
                    *out = std::ptr::null();
                    *out_len = 0;
                }
                ec::UNKNOWN_CODE
            }
        }
    })
}
