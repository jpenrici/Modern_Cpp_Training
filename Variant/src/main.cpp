/*
 * Training Variant
 *
 * References:
 *  https://en.cppreference.com/w/cpp/utility/variant/variant
 *  https://en.cppreference.com/w/cpp/utility/variant/visit
 *  https://en.cppreference.com/w/cpp/utility/format/format.html
 *  https://en.cppreference.com/w/cpp/utility/variant/monostate
 */

#include <cassert>
#include <print>
#include <variant>
#include <vector>

// Using std::monostate as the first alternative to represent an "empty" state
using Variant =
    std::variant<std::monostate, int, float, double, bool, char, std::string>;

enum VariantType { None, Integer, Float, Double, Boolean, Character, String };

std::vector<std::string> variantType{"none",    "integer",   "float", "double",
                                     "boolean", "character", "string"};

// Helper struct for std::visit with multiple overloads
template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

// Function to view and print the contents of a variant container
auto view(const auto v) {
  const auto visitor = overloads{
      [](std::monostate) { return std::string("empty"); },
      [](int value) { return std::format("{}", value); },
      [](float value) { return std::format("{}", value); },
      [](double value) { return std::format("{}", value); },
      [](bool value) { return std::format("{}", value ? "true" : "false"); },
      [](char value) { return std::format("'{}'", value); },
      [](const std::string &value) { return std::format("\"{}\"", value); },
  };

  std::println("{}", std::string(80, '-'));
  for (const auto &elem : v) {
    std::println("{} : {}", std::visit(visitor, elem),
                 variantType.at(elem.index()));
  }
}

auto main() -> int {

  // Using std::array
  std::array<Variant, 7> var_array;

  assert(std::holds_alternative<std::monostate>(var_array.at(0)) &&
         var_array.at(0).index() == None);

  var_array[0] = 10;
  assert(var_array.at(0).index() == Integer);

  var_array[0] = 10.5f;
  assert(var_array.at(0).index() == Float);

  var_array[0] = 10.5;
  assert(var_array.at(0).index() == Double);

  var_array[0] = false;
  assert(var_array.at(0).index() == Boolean);

  var_array[0] = 'a';
  assert(var_array.at(0).index() == Character);

  var_array[1] = {"STR"};
  assert(var_array.at(1).index() == String);

  // std::get_if
  if (int *p_val = std::get_if<int>(&var_array.front())) {
    std::println("Safely retrieved int from var_array[0]: {}", *p_val);
  } else {
    std::println("var_array[0] does not currently hold an int.");
  }

  if (const std::string *p_str = std::get_if<std::string>(&var_array[1])) {
    std::println("Safely retrieved string from var_array[1]: {}", *p_str);
  } else {
    std::println("var_array[1] does not currently hold a string.");
  }

  var_array = {std::monostate{}, 1, 2.1f, 3.5, true, 'a', "Hello World!"};
  view(var_array);

  // Using std::vector
  std::vector<Variant> vec{"ABC", 'z', false, 4.5,
                           2.9f,  10,  true,  std::monostate{}};
  view(vec);

  return 0;
}
