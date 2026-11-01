//! Integration tests for the C-ABI surface, called exactly as C++ would call it.

use std::ffi::c_char;
use std::ptr;

use ffi_rust::error_codes as ec;
use ffi_rust::errors;
use ffi_rust::ffi::*;
use ffi_rust::string_ops::MAX_LEN;

fn call_upper(input: &[u8]) -> Result<String, i32> {
    let mut out: *mut c_char = ptr::null_mut();
    let mut out_len = 0usize;
    let rc = unsafe {
        ffi_to_uppercase(
            input.as_ptr() as *const c_char,
            input.len(),
            &mut out,
            &mut out_len,
        )
    };
    if rc != ec::OK {
        assert!(out.is_null());
        assert_eq!(out_len, 0);
        return Err(rc);
    }
    let bytes = unsafe { std::slice::from_raw_parts(out as *const u8, out_len) }.to_vec();
    unsafe { ffi_free_string(out, out_len) };
    Ok(String::from_utf8(bytes).expect("Rust must return valid UTF-8"))
}

fn call_count(input: &[u8]) -> Result<usize, i32> {
    let mut count = 0usize;
    let rc = unsafe { ffi_count_chars(input.as_ptr() as *const c_char, input.len(), &mut count) };
    if rc == ec::OK {
        Ok(count)
    } else {
        Err(rc)
    }
}

#[test]
fn to_uppercase_ascii() {
    assert_eq!(call_upper(b"hello ffi").unwrap(), "HELLO FFI");
}

#[test]
fn to_uppercase_expands_sharp_s() {
    assert_eq!(call_upper("stra\u{df}e".as_bytes()).unwrap(), "STRASSE");
}

#[test]
fn to_uppercase_rejects_empty() {
    assert_eq!(call_upper(b""), Err(ec::EMPTY_STRING));
}

#[test]
fn to_uppercase_rejects_invalid_utf8() {
    assert_eq!(call_upper(&[0xff, 0xfe]), Err(ec::INVALID_UTF8));
}

#[test]
fn to_uppercase_rejects_oversized_input() {
    let big = vec![b'a'; MAX_LEN + 1];
    assert_eq!(call_upper(&big), Err(ec::BUFFER_OVERFLOW));
}

#[test]
fn to_uppercase_rejects_null_pointers() {
    let mut out: *mut c_char = ptr::null_mut();
    let mut out_len = 0usize;
    let rc = unsafe { ffi_to_uppercase(ptr::null(), 3, &mut out, &mut out_len) };
    assert_eq!(rc, ec::NULL_POINTER);
    let rc = unsafe {
        ffi_to_uppercase(
            b"a".as_ptr() as *const c_char,
            1,
            ptr::null_mut(),
            &mut out_len,
        )
    };
    assert_eq!(rc, ec::NULL_POINTER);
}

#[test]
fn count_chars_counts_scalar_values_not_bytes() {
    let text = "h\u{e9}llo";
    assert_eq!(text.len(), 6);
    assert_eq!(call_count(text.as_bytes()), Ok(5));
}

#[test]
fn count_chars_error_codes() {
    assert_eq!(call_count(b""), Err(ec::EMPTY_STRING));
    assert_eq!(call_count(&[0xc3]), Err(ec::INVALID_UTF8));
    let rc = unsafe { ffi_count_chars(b"a".as_ptr() as *const c_char, 1, ptr::null_mut()) };
    assert_eq!(rc, ec::NULL_POINTER);
}

#[test]
fn append_suffix_writes_in_place() {
    let mut buf = [0u8; 16];
    buf[..4].copy_from_slice(b"file");
    let mut len = 4usize;
    let rc = unsafe {
        ffi_append_suffix(
            buf.as_mut_ptr() as *mut c_char,
            &mut len,
            buf.len(),
            b".txt".as_ptr() as *const c_char,
            4,
        )
    };
    assert_eq!(rc, ec::OK);
    assert_eq!(len, 8);
    assert_eq!(&buf[..8], b"file.txt");
}

#[test]
fn append_suffix_reports_overflow_and_keeps_state() {
    let mut buf = *b"file\0\0";
    let mut len = 4usize;
    let rc = unsafe {
        ffi_append_suffix(
            buf.as_mut_ptr() as *mut c_char,
            &mut len,
            buf.len(),
            b".txt".as_ptr() as *const c_char,
            4,
        )
    };
    assert_eq!(rc, ec::BUFFER_OVERFLOW);
    assert_eq!(len, 4);
    assert_eq!(&buf[..4], b"file");
}

#[test]
fn append_suffix_rejects_empty_suffix_and_bad_buffer() {
    let mut buf = *b"abc\xff";
    let mut len = 3usize;
    let rc = unsafe {
        ffi_append_suffix(
            buf.as_mut_ptr() as *mut c_char,
            &mut len,
            4,
            b"x".as_ptr() as *const c_char,
            0,
        )
    };
    assert_eq!(rc, ec::EMPTY_STRING);

    len = 4; // includes the invalid byte
    let rc = unsafe {
        ffi_append_suffix(
            buf.as_mut_ptr() as *mut c_char,
            &mut len,
            4,
            b"x".as_ptr() as *const c_char,
            1,
        )
    };
    assert_eq!(rc, ec::INVALID_UTF8);
}

#[test]
fn append_suffix_rejects_aliased_borrows() {
    let mut buf = *b"abcdefgh";
    let base = buf.as_mut_ptr();
    let mut len = 4usize;
    let rc = unsafe {
        ffi_append_suffix(
            base as *mut c_char,
            &mut len,
            8,
            base.add(2) as *const c_char,
            2,
        )
    };
    assert_eq!(rc, ec::ALIASED_BORROW);
    assert_eq!(len, 4);
}

#[test]
fn free_string_accepts_null() {
    unsafe { ffi_free_string(ptr::null_mut(), 0) };
}

#[test]
fn guard_turns_panics_into_error_code() {
    assert_eq!(guard(|| panic!("expected test panic")), ec::INTERNAL_PANIC);
    assert_eq!(guard(|| ec::OK), ec::OK);
}

#[test]
fn error_message_matches_dictionary() {
    for &(code, expected) in ec::ENTRIES {
        let mut msg: *const c_char = ptr::null();
        let mut len = 0usize;
        assert_eq!(
            unsafe { ffi_error_message(code, &mut msg, &mut len) },
            ec::OK
        );
        let text = unsafe { std::slice::from_raw_parts(msg as *const u8, len) };
        assert_eq!(text, expected.as_bytes());
        assert_eq!(errors::message(code), expected);
    }
    let mut msg: *const c_char = ptr::null();
    let mut len = 1usize;
    assert_eq!(
        unsafe { ffi_error_message(999, &mut msg, &mut len) },
        ec::UNKNOWN_CODE
    );
    assert!(msg.is_null());
    assert_eq!(len, 0);
    assert_eq!(errors::message(999), errors::message(ec::UNKNOWN_CODE));
}
