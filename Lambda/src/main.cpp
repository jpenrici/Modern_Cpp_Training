#include <print>

auto main() -> int {

  std::println("Modern C++ : Lambda Expressions");

  std::string str;

  // Example 1: Lambda function captures and initializes variable 's' by moving
  // the value of 'str'.
  str = "Cpp";
  auto func1 = [s = std::move(str)] { std::println("func1: {}", s); };

  // Example 2: Function similar to example 1 with repetition parameter.
  str = "C++";
  auto func2 = [s = std::move(str)](size_t n = 1) {
    for (auto i = 0; i < n; i++) {
      std::println("func2: {}", s);
    }
  };

  // Example 3: Generic lambda function and use of fold expression.
  auto func3 = []<typename... Ts>(Ts &&...ts) {
    (std::println("func3: {}", std::forward<Ts>(ts)), ...);
  };

  // Example 4: Function similar to example 3 with outer function capture.
  auto func4 = [&]<typename... Ts>(Ts &&...ts) {
    (func3(std::forward<Ts>(ts)), ...);
  };

  // Example 5: Lambda with optional ().
  auto msg = [] { return "Lambda with optional ()."; };
  auto func5 = [&] { std::println("func5: {}", msg()); };

  // Test
  func1();
  func2();
  func2(3);
  func3(0, 1.1f, 2.3, 'a', "Modern");
  func4("Lambda", 'z', 9, 7.1, 0x10, true);
  func5();

  return 0;
}
