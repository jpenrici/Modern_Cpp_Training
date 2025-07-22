#include <print>
#include <type_traits>
#include <variant>

// std::monostate is used to represent an empty or uninitialized state
using DataVariant =
    std::variant<std::monostate, int, float, double, char, bool, std::string>;

template <typename T>
typename std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>,
                          void>
view(T value) {
  std::println("Integer, {} : {}", value, typeid(T).name());
}

template <typename T>
typename std::enable_if_t<std::is_floating_point_v<T>, void> view(T value) {
  std::println("Float, {} : {}", value, typeid(T).name());
}

void view(const std::string &value) {
  std::println("String, {} : {}", value, typeid(value).name());
}

void view(char value) {
  std::println("Char, {} : {}", value, typeid(value).name());
}

void view(bool value) {
  std::println("Bool, {} : {}", (value ? "true" : "false"),
               typeid(value).name());
}

void view(std::monostate) { std::println("Monostate"); }

struct VariantVisitor {
  template <typename T> void operator()(T &&value) const {
    view(std::forward<T>(value));
  }
};

auto main() -> int {

  std::visit(VariantVisitor{}, DataVariant(10));
  std::visit(VariantVisitor{}, DataVariant(-1.2));
  std::visit(VariantVisitor{}, DataVariant(-1.2f));
  std::visit(VariantVisitor{}, DataVariant(true));
  std::visit(VariantVisitor{}, DataVariant('Z'));
  std::visit(VariantVisitor{}, DataVariant("Cpp"));
  std::visit(VariantVisitor{}, DataVariant(1000.123456));
  std::visit(VariantVisitor{}, DataVariant());

  return 0;
}
