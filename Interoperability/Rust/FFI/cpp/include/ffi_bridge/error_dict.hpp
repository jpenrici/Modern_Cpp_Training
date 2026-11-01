#pragma once

#include <cstdint>
#include <expected>
#include <string_view>
#include <unordered_map>

namespace ffi_bridge {

/// C++ side error dictionary (code -> message), built from the generated table.
class ErrorDict {
public:
    using Map = std::unordered_map<std::int32_t, std::string_view>;

    [[nodiscard]] static const ErrorDict& instance();

    /// Message for `code`; unknown codes map to the "unknown code" message.
    [[nodiscard]] std::string_view message(std::int32_t code) const noexcept;
    [[nodiscard]] bool contains(std::int32_t code) const noexcept;
    [[nodiscard]] const Map& entries() const noexcept { return map_; }

private:
    ErrorDict();
    Map map_;
};

/// A numeric FFI status code that knows how to describe itself.
struct Error {
    std::int32_t code {};
    [[nodiscard]] std::string_view message() const noexcept
    {
        return ErrorDict::instance().message(code);
    }
};

/// Result type of the idiomatic wrappers: value on success, Error otherwise.
template <typename T>
using Result = std::expected<T, Error>;

} // namespace ffi_bridge
