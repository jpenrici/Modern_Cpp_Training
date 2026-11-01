/**
 * @file ffi_api.h
 * Single source of truth for the C-ABI shared by the C++23 and Rust sides.
 *
 * Conventions
 *  - A string is a (const char* ptr, size_t len) pair. It is NOT NUL-terminated,
 *    `len` counts BYTES, and Rust requires the content to be valid UTF-8.
 *  - Every function returns an int32_t status code (see error_codes.h);
 *    FFI_OK means success. The only exception is ffi_free_string().
 *  - Ownership: a buffer returned through an `out` parameter is owned by the
 *    side that allocated it (Rust). It must be released exactly once with
 *    ffi_free_string(ptr, len), never with free()/delete.
 *  - Borrowing: input pointers are only valid for the duration of the call and
 *    are never retained by the callee. The same holds for callback messages.
 *  - No Rust panic and no C++ exception may cross this boundary.
 */
#ifndef FFI_BRIDGE_FFI_API_H
#define FFI_BRIDGE_FFI_API_H

#include <stddef.h>
#include <stdint.h>

#include "ffi_bridge/error_codes.h"

/** Maximum accepted string length in bytes (mirrored by string_ops::MAX_LEN in Rust). */
#define FFI_MAX_STRING_LEN ((size_t)1048576)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log callback invoked by Rust. `msg`/`len` is a borrowed UTF-8 string valid only
 * during the call (copy it if needed). `level`: 0 = info, 1 = error.
 * The callback must not throw/unwind.
 */
typedef void (*ffi_log_callback_t)(int32_t level, const char* msg, size_t len, void* user_data);

/**
 * Upper-cases `input`. On success `*out`/`*out_len` receive a Rust-allocated
 * buffer that the caller must release with ffi_free_string(*out, *out_len).
 * On failure `*out` is NULL and `*out_len` is 0.
 */
int32_t ffi_to_uppercase(const char* input, size_t input_len, char** out, size_t* out_len);

/** Counts Unicode scalar values (not bytes) of a borrowed UTF-8 string. */
int32_t ffi_count_chars(const char* input, size_t input_len, size_t* out_count);

/**
 * Appends `suffix` to the caller-owned buffer `buf` (mutable borrow).
 * `*len` is the current content length (in) and the new length (out, on success only).
 * `cap` is the total writable size of `buf`; all `cap` bytes must be initialized.
 * The buffer is not NUL-terminated by this function. `suffix` must not overlap
 * `buf` (FFI_ALIASED_BORROW). On failure `buf` and `*len` are unchanged.
 */
int32_t ffi_append_suffix(char* buf, size_t* len, size_t cap, const char* suffix, size_t suffix_len);

/** Releases a buffer returned by Rust. `ptr` may be NULL (no-op). `len` must be the returned length. */
void ffi_free_string(char* ptr, size_t len);

/** Registers (or, with cb == NULL, removes) the log callback. A single callback is active at a time. */
int32_t ffi_set_log_callback(ffi_log_callback_t cb, void* user_data);

/**
 * Looks up Rust's own message for `code`. The returned pointer refers to static
 * storage: do NOT free it. Returns FFI_UNKNOWN_CODE (and NULL) for unknown codes.
 */
int32_t ffi_error_message(int32_t code, const char** out, size_t* out_len);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif /* FFI_BRIDGE_FFI_API_H */
