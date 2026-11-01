// Minimal test harness (no external framework). Exit code != 0 on any failed CHECK.

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "ffi_bridge/borrowing.hpp"
#include "ffi_bridge/error_dict.hpp"
#include "ffi_bridge/ffi_api.h"
#include "ffi_bridge/logging.hpp"
#include "ffi_bridge/rust_string.hpp"

namespace {
int g_checks = 0;
int g_failures = 0;
} // namespace

#define CHECK(...)                                                                            \
    do {                                                                                      \
        ++g_checks;                                                                           \
        if (!(__VA_ARGS__)) {                                                                 \
            ++g_failures;                                                                     \
            std::cerr << __FILE__ << ':' << __LINE__ << ": CHECK failed: " #__VA_ARGS__ "\n"; \
        }                                                                                     \
    } while (false)

#define CHECK_ERR(expr, expected_code)                                          \
    do {                                                                        \
        auto&& result_ = (expr);                                                \
        CHECK(!result_.has_value() && result_.error().code == (expected_code)); \
    } while (false)

using namespace ffi_bridge;

static void test_error_dict()
{
    const auto& dict = ErrorDict::instance();
    CHECK(dict.contains(FFI_OK));
    CHECK(dict.message(FFI_EMPTY_STRING) == "Input string is empty");
    CHECK(dict.message(999) == dict.message(FFI_UNKNOWN_CODE));
    CHECK(dict.entries().size() == static_cast<std::size_t>(FFI_ERROR_COUNT));

    // The C++ dictionary and the Rust dictionary must agree on every code.
    for (const auto& [code, message] : dict.entries()) {
        CHECK(!message.empty());
        const char* rust_msg = nullptr;
        std::size_t rust_len = 0;
        const std::int32_t rc = ffi_error_message(code, &rust_msg, &rust_len);
        CHECK(rc == FFI_OK);
        if (rc == FFI_OK) {
            CHECK(std::string_view { rust_msg, rust_len } == message);
        }
    }
    const char* msg = nullptr;
    std::size_t len = 1;
    CHECK(ffi_error_message(999, &msg, &len) == FFI_UNKNOWN_CODE);
    CHECK(msg == nullptr && len == 0);
}

static void test_to_uppercase()
{
    auto ok = RustString::to_uppercase("hello");
    CHECK(ok.has_value());
    if (ok) {
        CHECK(ok->view() == "HELLO");
    }

    auto sharp = RustString::to_uppercase("stra\xC3\x9F"
                                          "e"); // "strasse" with sharp s
    CHECK(sharp.has_value());
    if (sharp) {
        CHECK(sharp->view() == "STRASSE");
    }

    CHECK_ERR(RustString::to_uppercase(""), FFI_EMPTY_STRING);
    CHECK_ERR(RustString::to_uppercase(std::string_view {}), FFI_EMPTY_STRING);
    CHECK_ERR(RustString::to_uppercase(std::string_view { "\xff\xfe", 2 }), FFI_INVALID_UTF8);

    const std::string big(FFI_MAX_STRING_LEN + 1, 'a');
    CHECK_ERR(RustString::to_uppercase(big), FFI_BUFFER_OVERFLOW);

    char* out = nullptr;
    std::size_t out_len = 0;
    CHECK(ffi_to_uppercase(nullptr, 3, &out, &out_len) == FFI_NULL_POINTER);
    CHECK(out == nullptr && out_len == 0);
}

static void test_count_chars()
{
    const std::string text = "h\xC3\xA9llo";
    CHECK(text.size() == 6);
    const auto count = count_chars(text);
    CHECK(count.has_value() && *count == 5);
    CHECK_ERR(count_chars(""), FFI_EMPTY_STRING);
    CHECK_ERR(count_chars(std::string_view { "\xc3", 1 }), FFI_INVALID_UTF8);
}

static void test_append_suffix()
{
    std::string name = "file";
    const auto size = append_suffix(name, ".txt");
    CHECK(size.has_value() && *size == 8);
    CHECK(name == "file.txt");

    // Failures leave the target untouched (strong guarantee).
    CHECK_ERR(append_suffix(name, ""), FFI_EMPTY_STRING);
    CHECK_ERR(append_suffix(name, std::string_view { "\xff", 1 }), FFI_INVALID_UTF8);
    CHECK(name == "file.txt");

    // Rust's aliasing rule: no shared view of the string while it is mutably borrowed.
    CHECK_ERR(append_suffix(name, std::string_view { name }), FFI_ALIASED_BORROW);
    CHECK(name == "file.txt");

    // A shared view of a *different* string is fine.
    const std::string other = ".bak";
    CHECK(append_suffix(name, other).has_value());
    CHECK(name == "file.txt.bak");

    // Raw capacity check: the buffer is too small for the suffix.
    char buf[4] = { 'a', 'b', 'c', 'd' };
    std::size_t len = 2;
    CHECK(ffi_append_suffix(buf, &len, sizeof buf, "cdef", 4) == FFI_BUFFER_OVERFLOW);
    CHECK(len == 2);
    CHECK(ffi_append_suffix(nullptr, &len, 4, "x", 1) == FFI_NULL_POINTER);
}

static void test_ownership()
{
    auto upper = RustString::to_uppercase("move me");
    CHECK(upper.has_value());
    if (!upper) {
        return;
    }
    RustString first = std::move(*upper);
    CHECK(!first.empty());
    RustString second = std::move(first);
    CHECK(first.empty()); // moved-from: nothing to free twice
    CHECK(second.view() == "MOVE ME");

    auto boxed = std::move(second).into_owned();
    CHECK(second.empty());
    CHECK(boxed != nullptr && *boxed == "MOVE ME");

    const auto counted = consume_and_count(std::move(boxed));
    CHECK(boxed == nullptr);
    CHECK(counted.has_value() && *counted == 7);
    CHECK_ERR(consume_and_count(nullptr), FFI_NULL_POINTER);

    ffi_free_string(nullptr, 0); // must be a harmless no-op
}

static void test_callbacks()
{
    std::vector<std::pair<LogLevel, std::string>> logs;
    {
        const LogSubscription subscription { [&logs](LogLevel level, std::string_view message) {
            logs.emplace_back(level, std::string { message }); // copy: message is borrowed
        } };
        static_cast<void>(RustString::to_uppercase("x"));
        static_cast<void>(RustString::to_uppercase(""));
    }
    CHECK(logs.size() == 2);
    if (logs.size() == 2) {
        CHECK(logs[0].first == LogLevel::Info);
        CHECK(logs[1].first == LogLevel::Error);
        CHECK(logs[1].second.find("401") != std::string::npos);
    }

    static_cast<void>(RustString::to_uppercase("after unsubscribe"));
    CHECK(logs.size() == 2);

    // An exception in the sink must not cross into Rust.
    {
        const LogSubscription throwing { [](LogLevel, std::string_view) {
            throw std::runtime_error { "sink failure" };
        } };
        const auto count = count_chars("abc");
        CHECK(count.has_value() && *count == 3);
    }
}

int main()
{
    test_error_dict();
    test_to_uppercase();
    test_count_chars();
    test_append_suffix();
    test_ownership();
    test_callbacks();

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
