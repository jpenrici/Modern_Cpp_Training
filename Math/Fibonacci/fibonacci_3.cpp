#include <algorithm>
#include <cassert>
#include <print>
#include <utility>
#include <vector>

namespace MyMath {

auto fibonacci(unsigned n) -> unsigned long long
{
    assert(n <= 93 && "overflow in F(n)");

    std::vector<unsigned long long> table(static_cast<std::size_t>(n) + 1);

    std::ranges::generate(table, [a = 0ULL, b = 1ULL]() mutable {
        return std::exchange(a, std::exchange(b, a + b));
    });

    return table.back();
}

} // namespace MyMath

auto main() -> int
{
    unsigned n = 50;
    std::println("Fibonacci({}) = {}", n, MyMath::fibonacci(n));
}

// g++ -std=c++26 -Wall -Wextra fibonacci_3.cpp -o calc3
// ./calc3
