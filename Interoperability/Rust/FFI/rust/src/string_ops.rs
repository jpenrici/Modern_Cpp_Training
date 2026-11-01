//! Pure business logic. No raw pointers here: inputs are borrowed (`&str`, `&mut [u8]`)
//! and results are owned values moved back to the caller. Errors are numeric codes.

use crate::error_codes as ec;

/// Maximum accepted string length in bytes (mirrors `FFI_MAX_STRING_LEN` in ffi_api.h).
pub const MAX_LEN: usize = 1_048_576;

/// Business validation shared by all operations.
pub fn validate(input: &str) -> Result<(), i32> {
    if input.is_empty() {
        return Err(ec::EMPTY_STRING);
    }
    if input.len() > MAX_LEN {
        return Err(ec::BUFFER_OVERFLOW);
    }
    Ok(())
}

/// Upper-cases `input`. The returned `String` is owned by the caller (moved out).
pub fn to_uppercase(input: &str) -> Result<String, i32> {
    validate(input)?;
    let upper = input.to_uppercase();
    if upper.len() > MAX_LEN {
        return Err(ec::BUFFER_OVERFLOW);
    }
    Ok(upper)
}

/// Counts Unicode scalar values.
pub fn count_chars(input: &str) -> Result<usize, i32> {
    validate(input)?;
    Ok(input.chars().count())
}

/// Appends `suffix` after the first `len` bytes of `buf` (mutable borrow).
/// Returns the new content length. `buf` is untouched on error.
pub fn append_suffix(buf: &mut [u8], len: usize, suffix: &str) -> Result<usize, i32> {
    if len > buf.len() {
        return Err(ec::BUFFER_OVERFLOW);
    }
    if suffix.is_empty() {
        return Err(ec::EMPTY_STRING);
    }
    std::str::from_utf8(&buf[..len]).map_err(|_| ec::INVALID_UTF8)?;
    let new_len = len.checked_add(suffix.len()).ok_or(ec::BUFFER_OVERFLOW)?;
    if new_len > MAX_LEN || new_len > buf.len() {
        return Err(ec::BUFFER_OVERFLOW);
    }
    buf[len..new_len].copy_from_slice(suffix.as_bytes());
    Ok(new_len)
}
