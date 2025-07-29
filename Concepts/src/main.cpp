#include <concepts>
#include <format>
#include <iostream>
#include <print>

// Example 01
template <typename T>
concept Greater = requires(T a, T b) {
  { a > b } -> std::same_as<bool>;
  { a == b } -> std::same_as<bool>;
  { a < b } -> std::same_as<bool>;
};

template <Greater T> void isGreater(T a, T b) {
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

// Example 02
template <typename T>
concept IsClass = std::is_class_v<T>;

template <IsClass T> void isClass(T obj) {
  std::println("Rule satisfied! Processing a class object ...");
}

class Test_Class {};
struct Test_Struct {};

// Example 03
template <typename T>
concept Rule = requires(T obj) {
  { obj.getAttribute() } -> std::same_as<int>;
};

template <Rule T> void check(const T &obj) {
  std::println("Rule satisfied! Integer attribute = {}", obj.getAttribute());
}

class Test_Rule {
  int attribute;

public:
  Test_Rule(int attribute) : attribute(std::move(attribute)) {}

  auto getAttribute() const -> int { return attribute; }
};

auto main() -> int {
  std::println("Modern C++ : Concepts");

  // Example 01
  isGreater(10, 15);
  isGreater(10.5, 10.4);
  isGreater(true, false);
  isGreater("A", "B");
  isGreater('0', '9');
  isGreater(1, 1);

  Person p1{"Name1", 10};
  Person p2{"Name2", 10};
  isGreater(p1, p2);

  isGreater(Person{"Name3", 25}, {"Name3", 20});

  // Example 02
  isClass(Test_Class{});
  isClass(Test_Struct{});
  isClass(std::string{});

  // Example 03
  check(Test_Rule(-10));

  return 0;
}
