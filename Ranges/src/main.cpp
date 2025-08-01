#include <cassert>
#include <print>
#include <ranges>
#include <vector>

auto main() -> int {
  std::println("Modern C++ : Ranges");

  std::vector v1{-2, -1, 1}; // range
  std::println("{}", v1);

  assert(v1[0] == -2);
  assert(v1[1] == -1);
  assert(v1[2] == 1);

  // std::ranges::range
  assert(*std::ranges::begin(v1) == -2);
  assert(*std::ranges::end(v1) == 0); // returns a sentinel for the end
  std::println("{} {}", *std::ranges::begin(v1), *std::ranges::end(v1));

  // std::ranges::input_range
  auto input_r = v1 | std::views::filter([](int x) { return x > -2; });
  std::println("{}", input_r);

  // TO DO

  // std::ranges::output_range

  // TO DO

  // std::ranges::forward_range

  // TO DO

  // std::ranges::view

  // TO DO

  // std::ranges::to

  // TO DO

  // std::ranges::subrange

  // TO DO

  // std::ranges::dangling

  // TO DO

  // std::ranges::elements_of

  // TO DO

  // std::ranges::data

  // TO DO

  // std::ranges::size

  // TO DO

  // std::ranges::empty

  // TO DO

  return 0;
}
