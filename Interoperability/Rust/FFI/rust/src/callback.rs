//! Registry for the C++ log callback (the Rust -> C++ direction of the bridge).

use std::ffi::{c_char, c_void};
use std::sync::Mutex;

/// Matches `ffi_log_callback_t` in ffi_api.h.
pub type LogCallback = extern "C" fn(i32, *const c_char, usize, *mut c_void);

#[derive(Clone, Copy)]
struct Registration {
    callback: LogCallback,
    // Stored as an integer so the registry is `Send`; Rust never dereferences it.
    user_data: usize,
}

static REGISTRY: Mutex<Option<Registration>> = Mutex::new(None);

/// Registers the callback, or removes it when `callback` is `None`.
pub fn set(callback: Option<LogCallback>, user_data: *mut c_void) {
    let mut guard = REGISTRY.lock().unwrap_or_else(|e| e.into_inner());
    *guard = callback.map(|callback| Registration {
        callback,
        user_data: user_data as usize,
    });
}

/// Sends a borrowed message to the registered callback (no-op when none).
/// The registry lock is released before calling out, so re-entrancy cannot deadlock.
pub fn emit(level: i32, message: &str) {
    let registration = {
        let guard = REGISTRY.lock().unwrap_or_else(|e| e.into_inner());
        *guard
    };
    if let Some(reg) = registration {
        (reg.callback)(
            level,
            message.as_ptr() as *const c_char,
            message.len(),
            reg.user_data as *mut c_void,
        );
    }
}
