#include <generator>
#include <print>
#include <random>

auto random_number(int start, int end) -> std::generator<int> {

  start = std::min(start, end);
  end = std::max(start, end);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dist(start, end);

  while (true) {
    auto value = dist(gen);
    co_yield value;
  }
}

auto main() -> int {

  auto random_nums = random_number(0, 10);

  std::print("[ ");
  int count = 0;
  int limit = 20;
  for (int num : random_nums) {
    if (count >= limit) {
      break;
    }
    std::print("{} ", num);
    count++;
  }
  std::println("]");

  return 0;
}
