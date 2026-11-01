#include "ffi_bridge/borrowing.hpp"

#include <functional>

#include "ffi_bridge/ffi_api.h"
#include "internal.hpp"

namespace ffi_bridge {

Result<std::size_t> count_chars(std::string_view input)
{
    std::size_t count = 0;
    const std::int32_t rc = ffi_count_chars(detail::data_or_empty(input), input.size(), &count);
    if (rc != FFI_OK) {
        return std::unexpected(Error { rc });
    }
    return count;
}

Result<std::size_t> append_suffix(std::string& target, std::string_view suffix)
{
    // Reject aliasing BEFORE resizing: a reallocation would dangle `suffix`.
    if (!suffix.empty()) {
        const std::less<const char*> before;
        const char* const begin = target.data();
        const char* const end = begin + target.capacity();
        if (!before(suffix.data(), begin) && before(suffix.data(), end)) {
            return std::unexpected(Error { FFI_ALIASED_BORROW });
        }
    }
    const std::size_t old_size = target.size();
    if (suffix.size() > FFI_MAX_STRING_LEN || old_size > FFI_MAX_STRING_LEN - suffix.size()) {
        return std::unexpected(Error { FFI_BUFFER_OVERFLOW });
    }

    target.resize(old_size + suffix.size()); // writable, initialized region for Rust
    std::size_t len = old_size;
    const std::int32_t rc = ffi_append_suffix(target.data(), &len, target.size(),
        detail::data_or_empty(suffix), suffix.size());
    if (rc != FFI_OK) {
        target.resize(old_size);
        return std::unexpected(Error { rc });
    }
    target.resize(len);
    return len;
}

Result<std::size_t> consume_and_count(std::unique_ptr<std::string> owned)
{
    if (!owned) {
        return std::unexpected(Error { FFI_NULL_POINTER });
    }
    return count_chars(*owned); // `owned` is destroyed when this function returns
}

} // namespace ffi_bridge
