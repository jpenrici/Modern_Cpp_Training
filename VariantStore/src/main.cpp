#include <chrono>
#include <print>
#include <ranges>
#include <variant>

// std::monostate is used to represent an empty or uninitialized state
using DataVariant =
    std::variant<std::monostate, int, float, double, char, bool, std::string>;

struct Data {
  DataVariant data_variant;
  std::chrono::time_point<std::chrono::system_clock> created;

  template <typename T>
  Data(T &&value) : created(std::chrono::system_clock::now()) {
    data_variant = std::forward<T>(value);
  }
};

class DataStore {
public:
  // Variadic template function to add multiple Data objects.
  template <typename... Args> void addData(Args &&...args) {
    (data_store.emplace_back(std::forward<Args>(args)), ...);
  }

  // A helper struct for creating a callable object that can overload operator()
  template <class... Ts> struct OverloadSet : Ts... {
    using Ts::operator()...;
  };

  // View/print the stored data
  auto view() const {
    for (const auto &elem : data_store) {
      std::println("{} : (Created: {})",
                   std::visit(s_data_printer_visitor, elem.data_variant),
                   date_str(elem.created));
    }
  }

  // Data filter
  template <typename T> void viewFilteredByType() const {
    for (const auto &elem_str :
         data_store | std::ranges::views::filter([](const Data &d) {
           return std::holds_alternative<T>(d.data_variant);
         }) | std::views::transform([&](const Data &d) {
           return std::format(
               "{} (Created: {})",
               std::visit(s_data_printer_visitor, d.data_variant),
               date_str(d.created));
         })) {
      std::println("{}", elem_str);
    }
  }

private:
  std::vector<Data> data_store;

  // Shared visitor
  static constexpr auto s_data_printer_visitor = OverloadSet{
      [](std::monostate) { return std::string("empty"); },
      [](int value) { return std::format("int: {}", value); },
      [](float value) { return std::format("float: {}", value); },
      [](double value) { return std::format("double: {}", value); },
      [](bool value) {
        return std::format("bool: {}", value ? "true" : "false");
      },
      [](char value) { return std::format("char: '{}'", value); },
      [](const std::string &value) {
        return std::format("string: \"{}\"", value);
      },
  };

  // A help to convert timestamp to string
  auto date_str(std::chrono::time_point<std::chrono::system_clock> time) const
      -> std::string {
    auto c_time = std::chrono::system_clock::to_time_t(time);
    std::tm tm_buf;
#ifdef _MSV_VER
    localtime_s(&tm_buf, &c_time);
#else
    localtime_r(&c_time, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%d/%m/%Y %H:%M:%S");
    return ss.str();
  }
};

auto main() -> int {

  DataStore store;

  store.addData(Data(10), Data(3.14f), Data("Hello C++ Modern"), Data(99.9),
                Data('A'), Data(true), Data(10), Data("C++ 26"), Data(-1));
  store.addData(Data(0.1), Data(256), Data(1000), Data("Word"));

  auto line = []() { std::println("{}", std::string(80, '-')); };

  store.view();
  line();
  store.viewFilteredByType<int>();
  line();
  store.viewFilteredByType<std::string>();

  return 0;
}
