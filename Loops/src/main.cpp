#include <print>
#include <vector>

auto main() -> int {
  using ComplexType = std::tuple<std::string, int, bool>;

  std::vector<ComplexType> data{{"user1", 1000, true},
                                {"user2", 500, false},
                                {"user3", 8900, true},
                                {"user4", 100, false},
                                {"user5", 1010, true}};

  auto view = [](ComplexType item) {
    auto [user, score, status] = item;
    std::println("User: {} [Score: {}, Status: {}]", user, score,
                 status ? "Enabled" : "Disabled");
  };

  // Traditional
  for (size_t i = 0; i < data.size(); ++i) {
    view(data.at(i));
  }
  std::println();

  // Range-based
  for (const auto &item : data) {
    view(item);
  }
  std::println();

  // Traditional alias with typedef
  for (typedef ComplexType T; const T &item : data) {
    view(item);
  }
  std::println();

  // Range-based with Alias
  for (using T = ComplexType; const T &item : data) {
    view(item);
  }
  std::println();

  // Range-based com Structured Binding
  for (const auto &[user, score, status] : data) {
    std::println("User: {} [Score: {}, Status: {}]", user, score,
                 status ? "Enabled" : "Disabled");
  }

  return 0;
}
