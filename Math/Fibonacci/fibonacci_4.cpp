#include <cassert>
#include <generator>
#include <limits>
#include <print>
#include <ranges>
#include <utility>

namespace MyMath {

auto fibonacci() -> std::generator<unsigned long long>
{
    constexpr auto max = std::numeric_limits<unsigned long long>::max();
    unsigned long long a = 0, b = 1;
    while (true) {
        co_yield a;
        assert(b <= max - a && "overflow in F(n)");
        a = std::exchange(b, a + b);
    }
}

} // namespace MyMath

auto main() -> int
{
    unsigned n = 50;
    for (auto f : MyMath::fibonacci() | std::views::drop(n) | std::views::take(1))
        std::println("Fibonacci({}) = {}", n, f);
}

// g++ -std=c++26 -Wall -Wextra fibonacci_4.cpp -o calc4
// ./calc4
