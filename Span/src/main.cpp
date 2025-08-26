#include <print>
#include <span>
#include <vector>

auto main() -> int {

  // Automatic type deduction, std::vector<int>, std::span<int>
  std::vector numbers{10, 100, 20, 200, 30, 300, 444, 400, 50, 500};
  std::span slice(numbers.begin() + 2, 5); // index 2 to 6

  // Copy
  std::vector copy(slice.begin(), slice.end());

  std::println("{}", numbers);
  std::println("{}", slice);
  std::println("{}", copy);

  // Changed values
  numbers[2] = 999;
  numbers[3] = 888;

  std::println("{}", numbers);
  std::println("{}", slice);
  std::println("{}", copy);

  // Changed range
  auto it = numbers.begin() + 2; // index 2
  std::ranges::copy(std::vector{-1, -2, -3, -4}, it);

  std::println("{}", numbers);
  std::println("{}", slice);
  std::println("{}", copy);

  return 0;
}
