#include <chrono>
#include <memory_resource>
#include <print>
#include <vector>

constexpr int NUM_ITERATIONS = 100000;
constexpr int ELEMENTS_PER_VECTOR = 32;
constexpr std::size_t POOL_SIZE = 4096;

auto test_default() -> long long
{
    long long total = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        std::vector<int> vec;
        for (int j = 0; j < ELEMENTS_PER_VECTOR; ++j) {
            vec.push_back(j);
        }
        total += vec.back();
    }
    return total;
}

auto test_pmr() -> long long
{
    // Allocates a static buffer large enough for the current loop
    // In production, this could come from a thread-local arena or a larger pool.
    alignas(std::max_align_t) char buffer[POOL_SIZE];
    std::pmr::monotonic_buffer_resource mem_pool(buffer, sizeof(buffer), std::pmr::null_memory_resource());

    long long total = 0;
    for (int i = 0; i < NUM_ITERATIONS; ++i) {
        mem_pool.release(); // Resets the pool at each iteration
        std::pmr::vector<int> vec(&mem_pool);
        for (int j = 0; j < ELEMENTS_PER_VECTOR; ++j) {
            vec.push_back(j);
        }
        total += vec.back();
    }
    return total;
}

template <typename F>
auto measure(F&& func, long long& result) -> double
{
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    result = func();
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

auto main() -> int
{
    long long total_default = 0;
    long long total_pmr = 0;

    // Warm-up (cache, page faults, CPU frequency)
    measure(test_default, total_default);
    measure(test_pmr, total_pmr);

    const double ms_default = measure(test_default, total_default);
    const double ms_pmr = measure(test_pmr, total_pmr);

    std::println("Result for {} iterations ({} elements each)", NUM_ITERATIONS, ELEMENTS_PER_VECTOR);
    std::println("Standard Allocation (std::vector): {:.3f} ms (checksum {})", ms_default, total_default);
    std::println("PMR Allocation (monotonic_buffer): {:.3f} ms (checksum {})", ms_pmr, total_pmr);

    if (total_default != total_pmr) {
        std::println(stderr, "Checksums diferentes: resultado inconsistente!");
        return 1;
    }

    if (ms_pmr > 0.0) {
        std::println("Approximate speedup: {:.2f}x", ms_default / ms_pmr);
    }

    return 0;
}

// Build
// g++ -std=c++26 main.cpp -o benchmark
// g++ -std=c++26 -O3 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Werror -static-libstdc++ -static-libgcc main.cpp -o benchmark
