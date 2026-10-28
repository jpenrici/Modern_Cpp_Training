#include "calc.hpp"

#include <print>

auto main() -> int
{
    std::println("Testing calc.hpp ...");

    std::println("Result: {}", calc(10, 5.5)); // a + b * 2 = 21

    return 0;
}
