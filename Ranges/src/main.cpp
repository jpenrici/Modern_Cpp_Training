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

  // Cards in the deck
  std::vector<std::string> ranks = {"2",     "3",    "4",  "5",  "6",
                                    "7",     "8",    "9",  "10", "Jack",
                                    "Queen", "King", "Ace"};
  std::vector<std::string> suits = {"Hearts", "Diamonds", "Clubs", "Spades"};

  // std::ranges::views::cartesian_product
  // Combines all elements of the 1st range with all elements of the 2sd range.
  std::print("{{");
  for (const auto &[rank, suit] :
       std::ranges::views::cartesian_product(ranks, suits)) {
    std::print("{} of {}; ", rank, suit);
  }
  std::println("}}\n");

  // std::ranges::views::zip
  // Combines corresponding elements (by position) from each range.
  // But zip only iterates up to the length of the shortest range.
  std::print("{{");
  for (const auto &[rank, suit] : std::ranges::views::zip(ranks, suits)) {
    std::print("{} of {}; ", rank, suit);
  }
  std::println("}}");

  for (const auto &[value1, value2] : std::ranges::views::zip(
           std::vector<char>{'A', 'B', 'C', 'D'}, std::vector<int>{1, 3, 2})) {
    std::print("({} : {}) ", value1, value2);
  }
  std::println();

  // std::ranges::views::join_with
  std::string original = "test 01 2025 09 17.html";
  auto result = std::views::split(original, ' ') |
                std::ranges::views::join_with('_') |
                std::ranges::to<std::string>();
  std::println("'{}' --> '{}'", original, result);

  // std::views::repeat
  std::string str(5, '+'); // string(count, char)
  std::println("{}", str);

  auto cch = std::views::repeat("C++", 5); // const char *
  for (auto s : cch) {
    std::print("{} ", s);
  }
  std::println();

  // std::ranges::dangling
  auto process_range = [](auto &&R_ref, char value) {
    auto R_ref_type = typeid(std::decay_t<decltype(R_ref)>).name();
    auto result =
        std::ranges::find(std::forward<decltype(R_ref)>(R_ref), value);
    if constexpr (std::is_same_v<decltype(result), std::ranges::dangling>) {
      // Warning handling for unsafe ranges
      std::println(
          "[TYPE: {}] Result is std::ranges::dangling (WARNING: Non-borrowed "
          "temporary range used!)",
          R_ref_type);
    } else {
      // Normal iteration logic (only instantiated for valid iterators)
      if (result != std::ranges::end(R_ref)) {
        std::println("[TYPE: {}] : Value '{}' found: '{}'", R_ref_type, value,
                     *result);
      } else {
        std::println("[TYPE: {}] : Value '{}' not found.", R_ref_type, value);
      }
    }
  };

  // Borrowed range
  process_range(std::string_view{"alpha"}, 'a');
  process_range(std::string_view{"alpha"}, 'z');

  // Non Borrowed range
  process_range(std::vector<char>{'a', 'b', 'c'}, 'b'); // vector rvalue

  auto vec_lvalue = std::vector<char>{'a', 'b', 'c'};
  process_range(vec_lvalue, 'b');

  return 0;
}
