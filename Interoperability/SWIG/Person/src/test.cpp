#include "person.hpp"

#include <print>

auto main() -> int
{
    std::println("Testing person.hpp ...");

    Place address { "Teresopolis", "RJ", 42 };

    Person p1 { "Juan", 30 };
    p1.set_address(address);
    p1.birthday();

    std::println("{}", p1.greetings());

    return 0;
}
