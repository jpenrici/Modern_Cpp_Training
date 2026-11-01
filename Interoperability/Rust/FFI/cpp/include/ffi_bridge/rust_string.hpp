#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "ffi_bridge/error_dict.hpp"
#include "ffi_bridge/ffi_api.h"

namespace ffi_bridge {

/// Deleter that hands the buffer back to the allocator that created it (Rust).
struct RustStringDeleter {
    std::size_t len { 0 };
    void operator()(char* ptr) const noexcept
    {
        if (ptr != nullptr) {
            ffi_free_string(ptr, len);
        }
    }
};

/// Unique ownership of a Rust-allocated string (simulates a moved Rust `String`).
///
///  * move-only: copying is deleted, so a double free is impossible;
///  * `view()` is an immutable borrow (std::string_view) that must not outlive the object;
///  * `std::move(s).into_owned()` consumes the object and yields a C++-owned string.
class RustString {
public:
    RustString() noexcept = default;
    RustString(RustString&&) noexcept = default;
    RustString& operator=(RustString&&) noexcept = default;
    RustString(const RustString&) = delete;
    RustString& operator=(const RustString&) = delete;
    ~RustString() = default;

    /// Rust allocates the upper-cased copy; this object becomes its owner.
    [[nodiscard]] static Result<RustString> to_uppercase(std::string_view input);

    [[nodiscard]] std::string_view view() const noexcept
    {
        return ptr_ ? std::string_view { ptr_.get(), ptr_.get_deleter().len } : std::string_view {};
    }
    [[nodiscard]] std::size_t size() const noexcept { return view().size(); }
    [[nodiscard]] bool empty() const noexcept { return ptr_ == nullptr; }

    /// Consumes the object: copies into a C++ heap string and releases the Rust buffer.
    [[nodiscard]] std::unique_ptr<std::string> into_owned() &&;

private:
    RustString(char* ptr, std::size_t len) noexcept
        : ptr_ { ptr, RustStringDeleter { len } }
    {
    }

    std::unique_ptr<char, RustStringDeleter> ptr_;
};

} // namespace ffi_bridge
