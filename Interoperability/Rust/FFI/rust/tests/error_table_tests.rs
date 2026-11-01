//! Verifies that the Rust dictionary matches the shared source of truth.

use std::fs;
use std::path::PathBuf;

use ffi_rust::error_codes as ec;
use ffi_rust::errors;

fn source_entries() -> Vec<(i32, String, String)> {
    let path = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../common/error_codes.txt");
    let text = fs::read_to_string(&path).unwrap_or_else(|e| panic!("cannot read {path:?}: {e}"));
    text.lines()
        .map(str::trim)
        .filter(|line| !line.is_empty() && !line.starts_with('#'))
        .map(|line| {
            let mut parts = line.splitn(3, '|');
            let code = parts.next().unwrap().trim().parse::<i32>().unwrap();
            let name = parts.next().unwrap().trim().to_string();
            let message = parts.next().unwrap().trim().to_string();
            (code, name, message)
        })
        .collect()
}

#[test]
fn dictionary_matches_source_file() {
    let entries = source_entries();
    assert_eq!(errors::count(), entries.len());
    for (code, _name, message) in &entries {
        assert_eq!(errors::lookup(*code), Some(message.as_str()), "code {code}");
    }
}

#[test]
fn well_known_codes_are_stable() {
    assert_eq!(ec::OK, 100);
    assert_eq!(ec::EMPTY_STRING, 401);
    assert_eq!(ec::BUFFER_OVERFLOW, 402);
    assert_eq!(ec::INVALID_UTF8, 403);
    assert_eq!(ec::ENTRIES.len(), source_entries().len());
}
