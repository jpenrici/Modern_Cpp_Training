#include <cassert>
#include <print>
#include <vector>

namespace MyMath {

using u128 = unsigned __int128; // GCC 16.1.0

auto fibonacci(unsigned n) -> u128
{
    assert(n <= 184 && "overflow in F(n)");

    if (n <= 1)
        return static_cast<u128>(n);

    std::vector<u128> memo(static_cast<std::size_t>(n) + 1, 0);
    memo[1] = 1;

    auto fib = [&](this auto&& self, int x) -> u128 {
        auto& slot = memo[static_cast<std::size_t>(x)];
        if (x > 1 && slot == 0)
            slot = self(x - 1) + self(x - 2);
        return slot;
    };

    return fib(n);
}

} // namespace MyMath

auto main() -> int
{
    unsigned n = 185; // overflow
    std::println("Fibonacci({}) = {}", n, MyMath::fibonacci(n));
}

// g++ -std=c++26 -Wall -Wextra fibonacci_1.cpp -o calc1
// g++ -std=c++26 -g -O1 -fsanitize=undefined -fno-sanitize-recover=all -static-libstdc++ -static-libgcc fibonacci_1.cpp -o calc1
// ./calc1
