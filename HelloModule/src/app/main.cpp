import calc;

#include <print>

auto main() -> int {

  double width{5};
  double height{10};

  auto area = rectangle_area(width, height);

  std::println("Rectangle: {} x {}\nArea: {}", width, height, area);

  return 0;
}
