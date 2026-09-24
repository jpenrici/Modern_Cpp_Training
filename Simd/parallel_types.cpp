// parallel_types.cpp
//
// Build (tries C++26 std::simd first):
//  g++ -std=c++26 -march=native parallel_types.cpp -o test
//
// Build (forces fallback to std::experimental::simd, e.g. older GCC or C++23):
//  g++ -std=c++23 -march=native parallel_types.cpp -o test

#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <print>
#include <version>

#if defined(__has_include)
#if __has_include(<simd>)
#include <simd>
#endif
#if __has_include(<experimental/simd>)
#include <experimental/simd>
#define PT_HAS_EXPERIMENTAL_SIMD_HEADER 1
#endif
#endif

#if defined(__cpp_lib_simd)
#define PT_USE_STD_SIMD 1
namespace simd = std::simd;
#elif defined(PT_HAS_EXPERIMENTAL_SIMD_HEADER)
#define PT_USE_EXPERIMENTAL_SIMD 1
namespace simd = std::experimental;
#endif

template <typename V>
concept SimdLike = requires(V v, std::size_t i) {
    { v.size() } -> std::convertible_to<std::size_t>;
    { v[i] } -> std::convertible_to<int>;
};

namespace {

auto show(SimdLike auto vec, std::size_t limit)
{
    // NOTE: assert() is fully compiled out when NDEBUG is defined
    // (the default for most Release build configurations). Unlike
    // C++26 Contracts, this check disappears in optimized builds —
    // limit > vec.size() then becomes silent undefined behavior in
    // the loop below (out-of-bounds read).
    assert(limit <= vec.size());

    std::print("{{ ");
    for (std::size_t i = 0; i < limit; ++i)
        std::print("{} ", static_cast<int>(vec[i]));
    std::println("}}");
}

}

auto main() -> int
{
    constexpr std::size_t LIMIT = 4;

#if defined(PT_USE_STD_SIMD)
    std::println("Using std::simd (C++26), __cpp_lib_simd = {}", __cpp_lib_simd);

    // parallel vector of LIMIT integers
    simd::vec<int, LIMIT> a([](int i) { return i; }); // {0, 1, 2, 3}
    simd::vec<int, LIMIT> b([](int i) { return 3 - i; }); // {3, 2, 1, 0}

    auto sum = a + b; // { 3 3 3 3 }
    auto minimum = simd::min(a, b); // { 0 1 1 0 }

    show(sum, LIMIT);
    show(minimum, LIMIT);

    return EXIT_SUCCESS;

#elif defined(PT_USE_EXPERIMENTAL_SIMD)
    std::println("std::simd unavailable — falling back to std::experimental::simd (TS)");

    // native_simd automatically picks the optimal width for the target
    simd::native_simd<int> a, b;
    for (std::size_t i = 0; i < LIMIT; ++i) {
        a[i] = static_cast<int>(i);
        b[i] = static_cast<int>(LIMIT - 1 - i);
    }

    auto sum = a + b;
    auto minimum = simd::min(a, b);

    show(sum, LIMIT); // { 3 3 3 3 }
    show(minimum, LIMIT); // { 0 1 1 0 }

    return EXIT_SUCCESS;

#else
    std::println("No SIMD implementation available (neither std::simd nor std::experimental::simd)!");
    return EXIT_FAILURE;
#endif
}
