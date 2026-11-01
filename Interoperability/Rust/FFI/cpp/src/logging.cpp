#include "ffi_bridge/logging.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

#include "ffi_bridge/error_dict.hpp"
#include "ffi_bridge/ffi_api.h"

namespace {

extern "C" {
// Called by Rust. Must never throw: exceptions cannot unwind into Rust frames.
void ffi_bridge_log_trampoline(std::int32_t level, const char* msg, std::size_t len,
    void* user_data) noexcept
{
    if (user_data == nullptr || msg == nullptr) {
        return;
    }
    try {
        const auto& sink = *static_cast<const ffi_bridge::LogSubscription::Sink*>(user_data);
        const auto lvl = level == 0 ? ffi_bridge::LogLevel::Info : ffi_bridge::LogLevel::Error;
        sink(lvl, std::string_view { msg, len });
    } catch (...) {
        // Swallow: the boundary is exception-free by contract.
    }
}
} // extern "C"

} // namespace

namespace ffi_bridge {

LogSubscription::LogSubscription(Sink sink)
    : sink_ { std::move(sink) }
{
    const std::int32_t rc = ffi_set_log_callback(&ffi_bridge_log_trampoline, &sink_);
    if (rc != FFI_OK) {
        throw std::runtime_error { std::string { Error { rc }.message() } };
    }
}

LogSubscription::~LogSubscription()
{
    static_cast<void>(ffi_set_log_callback(nullptr, nullptr));
}

} // namespace ffi_bridge
