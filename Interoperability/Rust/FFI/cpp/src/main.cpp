// Demo: ownership, borrowing, error dictionary and Rust -> C++ callbacks.

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "ffi_bridge/borrowing.hpp"
#include "ffi_bridge/error_dict.hpp"
#include "ffi_bridge/ffi_api.h"
#include "ffi_bridge/logging.hpp"
#include "ffi_bridge/rust_string.hpp"

namespace {

void print_error(std::string_view what, const ffi_bridge::Error& error)
{
    std::cerr << "  " << what << " failed: [" << error.code << "] " << error.message() << '\n';
}

} // namespace

int main()
{
    using namespace ffi_bridge;

    // Rust calls back into this lambda for every operation it performs.
    const LogSubscription logs { [](LogLevel level, std::string_view message) {
        std::cout << "  [rust-log:" << (level == LogLevel::Info ? "info" : "error") << "] "
                  << message << '\n';
    } };

    std::cout << "== 1. Ownership: Rust allocates, C++ owns (unique_ptr + FFI destructor) ==\n";
    auto upper = RustString::to_uppercase("hello, ffi bridge!");
    if (!upper) {
        print_error("to_uppercase", upper.error());
        return EXIT_FAILURE;
    }
    RustString owned = std::move(*upper);
    std::cout << "  owned: " << owned.view() << " (" << owned.size() << " bytes)\n";

    std::cout << "\n== 2. Move semantics: the source is empty afterwards ==\n";
    RustString moved = std::move(owned);
    std::cout << "  source empty after move: " << std::boolalpha << owned.empty() << '\n';
    std::cout << "  destination: " << moved.view() << '\n';

    std::cout << "\n== 3. Consuming conversion into a C++-owned std::unique_ptr<std::string> ==\n";
    auto boxed = std::move(moved).into_owned();
    std::cout << "  boxed: " << *boxed << '\n';
    const auto consumed = consume_and_count(std::move(boxed));
    std::cout << "  boxed is null after the move: " << (boxed == nullptr) << '\n';
    if (consumed) {
        std::cout << "  characters counted by Rust: " << *consumed << '\n';
    }

    std::cout << "\n== 4. Immutable borrow (std::string_view) ==\n";
    const std::string text = "h\xC3\xA9llo w\xC3\xB6rld";
    if (const auto count = count_chars(text)) {
        std::cout << "  bytes=" << text.size() << " chars=" << *count << '\n';
    } else {
        print_error("count_chars", count.error());
    }

    std::cout << "\n== 5. Mutable borrow (std::string&) ==\n";
    std::string file = "report";
    if (const auto size = append_suffix(file, ".txt")) {
        std::cout << "  file=" << file << " size=" << *size << '\n';
    } else {
        print_error("append_suffix", size.error());
    }
    if (const auto aliased = append_suffix(file, std::string_view { file }); !aliased) {
        print_error("append_suffix(self)", aliased.error());
    }

    std::cout << "\n== 6. Error dictionary ==\n";
    if (const auto empty = RustString::to_uppercase(""); !empty) {
        print_error("to_uppercase(\"\")", empty.error());
    }
    if (const auto bad = count_chars(std::string_view { "\xff\xfe", 2 }); !bad) {
        print_error("count_chars(invalid utf-8)", bad.error());
    }
    char small[4] = { 'a', 'b', 'c', 'd' };
    std::size_t len = 2;
    const std::int32_t rc = ffi_append_suffix(small, &len, sizeof small, "cdef", 4);
    std::cout << "  raw ffi_append_suffix with cap=4: [" << rc << "] "
              << ErrorDict::instance().message(rc) << " (len still " << len << ")\n";

    return EXIT_SUCCESS;
}
