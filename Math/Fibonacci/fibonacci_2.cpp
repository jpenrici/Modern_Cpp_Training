#include <cassert>
#include <print>
#include <utility>
#include <vector>

namespace MyMath {

auto fibonacci(unsigned n) -> unsigned long long
{
    assert(n <= 93 && "overflow in F(n)");

    unsigned long long a = 0, b = 1;
    for (unsigned i = 0; i < n; ++i)
        a = std::exchange(b, a + b);
    return a;
}

} // namespace MyMath

auto main() -> int
{
    unsigned n = 50;
    std::println("Fibonacci({}) = {}", n, MyMath::fibonacci(n));
}

// g++ -std=c++26 -Wall -Wextra fibonacci_2.cpp -o calc2
// ./calc2
