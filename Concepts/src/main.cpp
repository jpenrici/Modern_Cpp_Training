#include <concepts>
#include <format>
#include <iostream>
#include <print>

template <typename T>
concept Greater = requires(T a, T b) {
  { a > b } -> std::same_as<bool>;
  { a == b } -> std::same_as<bool>;
  { a < b } -> std::same_as<bool>;
};

template <Greater T> void IsGreater(T a, T b) {
  auto res = a <=> b;
  // overloading std::formatter to use std::print, didn't work!
  std::cout << "(" << a << "," << b << "): ";
  if (res < 0) {
    std::cout << a << " is not greater\n";
  } else if (res == 0) {
    std::cout << a << " is not greater\n";
  } else if (res > 0) {
    std::cout << a << " is greater\n";
  }
}

struct Person {
  std::string name;
  unsigned age;

  Person(std::string newName, unsigned newAge)
      : name(std::move(newName)), age(std::move(newAge)) {}

  // this.name == other.name && this.age == other.age
  bool operator==(const Person &other) const = default;

  // this operator makes a 3-way comparison.
  std::strong_ordering operator<=>(const Person &other) const = default;

  // overload for display
  friend std::ostream &operator<<(std::ostream &os, const Person &p) {
    std::string s = "{" + p.name + "," + std::to_string(p.age) + "}";
    return os << s;
  }
};

auto main() -> int {
  std::println("Modern C++ : Concepts");

  IsGreater(10, 15);
  IsGreater(10.5, 10.4);
  IsGreater(true, false);
  IsGreater("A", "B");
  IsGreater('0', '9');
  IsGreater(1, 1);

  Person p1{"Name1", 10};
  Person p2{"Name2", 10};
  IsGreater(p1, p2);

  return 0;
}
