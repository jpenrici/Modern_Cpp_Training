#include <print>
#include <ranges>
#include <vector>

auto main() -> int {
  std::vector resources{"/home", "/about", "/contact", "/api/wheater",
                        "/image.png"};
  std::vector statusCode = {200, 200, 404, 500, 200};

  // Traditional approach
  for (size_t i = 0; i < resources.size(); ++i) {
    std::println("Resource: {}, Status: {} {}", resources.at(i),
                 statusCode.at(i), statusCode.at(i) >= 400 ? "Error!" : "");
  }
  std::println();

  // Modern approach
  auto combined_data = std::views::zip(resources, statusCode);

  for (const auto &[index, pair] : std::views::enumerate(combined_data)) {
    const auto &[resource, status] = pair;
    std::println("Resource: {}, Status: {} {}", resource, status,
                 status >= 400 ? "Error!" : "");
  }

  return 0;
}
