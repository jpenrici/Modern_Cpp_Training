#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "ffi_bridge/error_dict.hpp"

namespace ffi_bridge {

/// Immutable borrow: the callee only reads `input` and never keeps it.
[[nodiscard]] Result<std::size_t> count_chars(std::string_view input);

/// Mutable borrow: exclusive access to `target`, mirroring Rust's `&mut String`.
/// Appends `suffix` and returns the new size. On failure `target` is unchanged
/// (strong guarantee). `suffix` must not alias `target` (Rust's "no shared borrow
/// while mutably borrowed" rule): doing so yields FFI_ALIASED_BORROW.
[[nodiscard]] Result<std::size_t> append_suffix(std::string& target, std::string_view suffix);

/// Ownership transfer: takes the string by value (a "move" in Rust terms), counts
/// its characters and drops it. After `std::move`, the caller's pointer is null.
[[nodiscard]] Result<std::size_t> consume_and_count(std::unique_ptr<std::string> owned);

} // namespace ffi_bridge
