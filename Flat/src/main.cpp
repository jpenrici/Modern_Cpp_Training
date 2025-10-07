#include <flat_set>
#include <print>
#include <vector>

auto main() -> int {

  // std::vector<int>
  std::vector list{101, 325, 42, 15, 99, 100};

  // To efficiently use std::flat_set
  std::sort(list.begin(), list.end());

  // Before, with the ordered vector
  std::println("Vector (Before Move): {}", list);

  // std::flat_set<int>
  std::flat_set flat_set_list(
      // instructs flat_set NOT to re-sort
      std::sorted_unique_t{},
      // instructs flat_set to take ownership of the
      // underlying memory that was managed by std::vector
      std::move(list));

  // After after std::move
  std::println("Vector (After Move): {}", list);
  std::println("Flat_set           : {}", flat_set_list);

  // Search Example
  int value{42};
  std::println("Value {}: {}", value,
               flat_set_list.contains(value) ? "Correct search!"
                                             : "Search failed!");

  return 0;
}
