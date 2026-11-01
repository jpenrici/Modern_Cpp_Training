#include "ffi_bridge/error_dict.hpp"

#include "ffi_bridge/error_codes.h"
#include "ffi_bridge/error_table.hpp"

namespace ffi_bridge {

const ErrorDict& ErrorDict::instance()
{
    static const ErrorDict dict;
    return dict;
}

ErrorDict::ErrorDict()
{
    map_.reserve(generated::kErrorEntries.size());
    for (const auto& [code, message] : generated::kErrorEntries) {
        map_.emplace(code, message);
    }
}

std::string_view ErrorDict::message(std::int32_t code) const noexcept
{
    if (const auto it = map_.find(code); it != map_.end()) {
        return it->second;
    }
    if (const auto it = map_.find(FFI_UNKNOWN_CODE); it != map_.end()) {
        return it->second;
    }
    return "Unknown error code";
}

bool ErrorDict::contains(std::int32_t code) const noexcept
{
    return map_.find(code) != map_.end();
}

} // namespace ffi_bridge
