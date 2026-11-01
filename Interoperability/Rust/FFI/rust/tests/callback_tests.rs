//! Rust -> C++ direction: Rust invokes a registered callback with borrowed messages.
//! Kept in its own test binary because the callback registry is process-global.

use std::ffi::{c_char, c_void};
use std::ptr;
use std::sync::Mutex;

use ffi_rust::callback::LogCallback;
use ffi_rust::error_codes as ec;
use ffi_rust::ffi::*;

static LOGS: Mutex<Vec<(i32, String)>> = Mutex::new(Vec::new());

extern "C" fn sink(level: i32, msg: *const c_char, len: usize, user_data: *mut c_void) {
    assert_eq!(user_data as usize, 0xBEEF);
    let bytes = unsafe { std::slice::from_raw_parts(msg as *const u8, len) };
    // The message is only borrowed for this call: copy it.
    let text = std::str::from_utf8(bytes).unwrap().to_owned();
    LOGS.lock().unwrap().push((level, text));
}

fn count_chars(input: &[u8]) -> i32 {
    let mut count = 0usize;
    unsafe { ffi_count_chars(input.as_ptr() as *const c_char, input.len(), &mut count) }
}

#[test]
fn callback_receives_messages_until_unregistered() {
    let rc = ffi_set_log_callback(Some(sink as LogCallback), 0xBEEF as *mut c_void);
    assert_eq!(rc, ec::OK);

    assert_eq!(count_chars(b"abc"), ec::OK);
    assert_eq!(count_chars(b""), ec::EMPTY_STRING);

    {
        let logs = LOGS.lock().unwrap();
        assert_eq!(logs.len(), 2);
        assert_eq!(logs[0].0, 0);
        assert!(logs[0].1.starts_with("count_chars: "));
        assert_eq!(logs[1].0, 1);
        assert!(logs[1].1.contains("401"));
    }

    assert_eq!(ffi_set_log_callback(None, ptr::null_mut()), ec::OK);
    assert_eq!(count_chars(b"abc"), ec::OK);
    assert_eq!(LOGS.lock().unwrap().len(), 2, "no log after unregistering");
}
