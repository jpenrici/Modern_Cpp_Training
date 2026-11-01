#include "ffi_bridge/rust_string.hpp"

#include <utility>

#include "internal.hpp"

namespace ffi_bridge {

Result<RustString> RustString::to_uppercase(std::string_view input)
{
    char* out = nullptr;
    std::size_t out_len = 0;
    const std::int32_t rc = ffi_to_uppercase(detail::data_or_empty(input), input.size(), &out, &out_len);
    if (rc != FFI_OK) {
        return std::unexpected(Error { rc });
    }
    return RustString { out, out_len };
}

std::unique_ptr<std::string> RustString::into_owned() &&
{
    auto owned = std::make_unique<std::string>(view());
    ptr_.reset(); // frees the Rust buffer through ffi_free_string
    return owned;
}

} // namespace ffi_bridge
