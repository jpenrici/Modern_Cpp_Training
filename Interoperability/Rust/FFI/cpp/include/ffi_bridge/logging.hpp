#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace ffi_bridge {

enum class LogLevel : std::int32_t { Info = 0,
    Error = 1 };

/// RAII registration of a log sink that Rust calls back (Rust -> C++ direction).
///
///  * `message` is borrowed for the duration of the call: copy it to keep it;
///  * exceptions thrown by the sink are swallowed at the boundary;
///  * a single subscription is active at a time (Rust keeps one callback);
///  * the object is pinned in memory because Rust holds its address as `user_data`.
class LogSubscription {
public:
    using Sink = std::function<void(LogLevel, std::string_view)>;

    explicit LogSubscription(Sink sink);
    ~LogSubscription();

    LogSubscription(const LogSubscription&) = delete;
    LogSubscription& operator=(const LogSubscription&) = delete;
    LogSubscription(LogSubscription&&) = delete;
    LogSubscription& operator=(LogSubscription&&) = delete;

private:
    Sink sink_;
};

} // namespace ffi_bridge
