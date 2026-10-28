// Statistic/native/tests/minitest.hpp

/**
 * @file minitest.hpp
 * @brief Tiny dependency-free test harness (no network needed to build).
 *
 * Usage:
 *   TEST(my_case) { CHECK(1 + 1 == 2); CHECK_NEAR(0.1 + 0.2, 0.3, 1e-12); }
 *
 * A failed CHECK is reported but does not abort the test, so one run shows
 * every problem at once.
 */
#pragma once

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace minitest {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline auto registry() -> std::vector<TestCase>&
{
    static std::vector<TestCase> tests; // constructed on first use (safe across TUs)
    return tests;
}

inline auto failure_count() -> int&
{
    static int failures = 0;
    return failures;
}

struct Registrar {
    Registrar(const char* name, void (*body)()) { registry().push_back({ name, body }); }
};

inline void report_failure(const char* file, int line, const char* expr)
{
    ++failure_count();
    std::fprintf(stderr, "    FAILED %s:%d: %s\n", file, line, expr);
}

/// Runs every registered test. Returns the process exit code.
inline auto run_all() -> int
{
    int failed_tests = 0;
    for (const auto& test : registry()) {
        const int before = failure_count();
        test.body();
        const bool ok = failure_count() == before;
        std::printf("[%s] %s\n", ok ? " OK " : "FAIL", test.name.c_str());
        if (!ok)
            ++failed_tests;
    }
    std::printf("\n%zu tests, %d failed, %d failed checks\n", registry().size(), failed_tests,
        failure_count());
    return failed_tests == 0 ? 0 : 1;
}

} // namespace minitest

#define TEST(name)                                                          \
    static void name();                                                     \
    static ::minitest::Registrar minitest_registrar_##name { #name, name }; \
    static void name()

#define CHECK(cond)                                                \
    do {                                                           \
        if (!(cond))                                               \
            ::minitest::report_failure(__FILE__, __LINE__, #cond); \
    } while (false)

#define CHECK_NEAR(actual, expected, eps) \
    CHECK(std::fabs(static_cast<double>(actual) - static_cast<double>(expected)) <= (eps))
