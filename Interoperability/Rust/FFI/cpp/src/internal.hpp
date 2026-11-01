#pragma once

#include <string_view>

namespace ffi_bridge::detail {

/// An empty std::string_view may carry a null data(); the C-ABI wants a valid
/// pointer so that emptiness is reported as EMPTY_STRING rather than NULL_POINTER.
[[nodiscard]] inline const char* data_or_empty(std::string_view view) noexcept
{
    return view.data() != nullptr ? view.data() : "";
}

} // namespace ffi_bridge::detail
