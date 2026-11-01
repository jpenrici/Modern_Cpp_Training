//! Error dictionary: maps numeric codes to descriptive messages.

use std::collections::HashMap;
use std::sync::OnceLock;

use crate::error_codes::{ENTRIES, UNKNOWN_CODE};

fn dictionary() -> &'static HashMap<i32, &'static str> {
    static DICTIONARY: OnceLock<HashMap<i32, &'static str>> = OnceLock::new();
    DICTIONARY.get_or_init(|| ENTRIES.iter().copied().collect())
}

/// Returns the message registered for `code`, if any.
pub fn lookup(code: i32) -> Option<&'static str> {
    dictionary().get(&code).copied()
}

/// Returns the message for `code`, falling back to the "unknown code" message.
pub fn message(code: i32) -> &'static str {
    lookup(code)
        .or_else(|| lookup(UNKNOWN_CODE))
        .unwrap_or("Unknown error code")
}

/// Number of registered codes.
pub fn count() -> usize {
    dictionary().len()
}
