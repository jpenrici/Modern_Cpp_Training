#include <print>

class Calc {
public:
  [[deprecated("Obsolete constructor. Use Calc(n1, n2)!")]]
  Calc()
      : number1(-1.1), number2(-2.3) {}

  Calc(auto n1, auto n2) : number1(n1), number2(n2) {}

  [[maybe_unused]]
  Calc(auto n1, auto n2, auto n3)
      : number1(n1), number2(n2 * n3) {}

  [[nodiscard("The symbolic value is important for internal logic.")]]
  auto symbolicValue() -> int {
    return -2025;
  }

  // Calculates the product of two integers.
  auto calc(int n1, int n2) -> auto { return n1 * n2; }

  [[deprecated("Use double precision calculations for better accuracy, e.g., "
               "calc(double, double).")]]
  auto calc(float n1, float n2) -> auto {
    return n1 * n2;
  }

private:
  // Internal numbers for calculations.
  double number1;
  double number2;

  [[maybe_unused]]
  int symbolic_value = -1971;

  [[maybe_unused]]
  constexpr static const float SYMVALUE = 123.456;
};

auto main() -> int {

  // Using constructor with warning.
  Calc myCalc;

  int symVal = myCalc.symbolicValue(); // Correct usage to avoid warning.
  std::println("Symbolic Value: {}", symVal);

  // Calling a [[deprecated]] function will issue a warning.
  float result_float = myCalc.calc(10.5f, 2.0f);
  std::println("Deprecated float calc result: {}", result_float);

  // A lambda function demonstrating attribute usage.
  auto calc_doubled =
      [] [[nodiscard("The doubled calculation result must be used.")]]
      (auto n1, auto n2) { return n1 * n2 * 2; };

  int doubled_result = calc_doubled(5, 3);
  std::println("Doubled calculation result: {}\n", doubled_result);

  return 0;
}
