#include <cassert>
#include <print>
#include <ranges>
#include <vector>

auto main() -> int {
  std::println("Modern C++ : Ranges");

  std::vector v1{-2, -1, 1, 8, -5}; // range
  assert(v1[0] == -2);
  assert(v1[4] == -5);
  std::println("{}", v1);

  // std::ranges::range
  auto begin = *std::ranges::begin(v1); // value of the first iterator
  auto end = *std::ranges::end(v1);     // returns a sentinel for the end
  assert(begin == -2);
  assert(end == 0);
  std::println("{} {}", begin, end);

  // Pipe operator
  auto v2 =
      v1 | std::views::filter([](int x) { return x >= -1; }); // complex type
  auto expected = std::vector<int>{-1, 1, 8} | std::views::all;
  assert(std::ranges::equal(v2, expected));
  std::println("{}", v2);

  // assert(*(std::ranges::begin(v2) + 0) == -1); // error: cannot iterate
  // assert(v2[2] == 8);                          // error: cannot iterate
  assert(*(std::ranges::begin(expected) + 0) == -1); // std::ranges ...
  assert(expected[2] == 8); // std::ranges::ref_view<std::vector<int>>

  // std::ranges::view
  auto v3 = v1 | std::views::transform([](int x) { return x * x; });
  assert(*(std::ranges::begin(v3) + 0) == 4);
  assert(*(std::ranges::begin(v3) + 1) == 1);
  assert(*(std::ranges::begin(v3) + 4) == 25);
  std::println("{}", v3);

  // std::ranges::to
  std::vector<int> v4 = v1 |
                        std::views::transform([](int x) { return -1 * x; }) |
                        std::ranges::to<std::vector>();
  assert(v4[0] == 2);
  assert(v4[2] == -1);
  assert(v4[4] == 5);
  std::println("{}", v4);

  // std::ranges::subrange
  auto v5 = std::ranges::subrange(std::ranges::begin(v4) + 0,
                                  std::ranges::begin(v4) + 2);
  assert(v5[0] == 2);
  assert(v5[1] == 1);
  std::println("{}", v5);

  auto v6 =
      std::ranges::subrange(std::ranges::begin(v4) + 2, std::ranges::end(v4));
  assert(v6[0] == -1);
  assert(v6[2] == 5);
  std::println("{}", v6);

  return 0;
}
