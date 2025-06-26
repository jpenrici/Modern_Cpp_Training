/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */

#include <cassert>
#include <print>
#include <regex>
#include <string_view>

// --- Structs and Type Aliases ---
struct Data {
  bool status = false;
  std::string value;
};

struct Dict {
  std::string key;
  std::string value;

  auto compare(Dict Other) -> bool {
    return key == Other.key && value == Other.value;
  }
};

// Define a variant type for the possible return types of parsed data
using ParsedData =
    std::variant<std::vector<int>, std::vector<float>, std::vector<char>,
                 std::vector<std::string>, std::vector<Dict>>;

// --- Constants ---
const std::string_view WHITESPACE = " \t\n\r\f\v";

// --- Helper Functions ---
auto trim_left(std::string_view input,
               std::string_view chars_to_trim = WHITESPACE) -> std::string {
  size_t first_char = input.find_first_not_of(chars_to_trim);
  if (std::string_view::npos == first_char) {
    return {};
  }
  return std::string(input.substr(first_char));
}

auto trim_right(std::string_view input,
                std::string_view chars_to_trim = WHITESPACE) -> std::string {
  size_t last_char = input.find_last_not_of(chars_to_trim);
  if (std::string_view::npos == last_char) {
    return {};
  }
  return std::string(input.substr(0, last_char + 1));
}

auto trim(std::string_view input, std::string_view chars_to_trim = WHITESPACE)
    -> std::string {
  auto trimmed_left = trim_left(input, chars_to_trim);
  return trim_right(trimmed_left, chars_to_trim);
}

// Funtion to split string_view
auto split(std::string_view sv, std::string_view delimiter)
    -> std::vector<std::string_view> {
  std::vector<std::string_view> result;
  size_t start = 0;
  size_t end = sv.find(delimiter);

  while (end != std::string_view::npos) {
    result.push_back(sv.substr(start, end - start));
    start = end + delimiter.length();
    end = sv.find(delimiter, start);
  }
  // Add the last part
  result.push_back(sv.substr(start));
  return result;
}

auto remove_char(std::string_view input, char char_to_remove) -> std::string {
  std::string s{input};
  auto end_it = std::remove(s.begin(), s.end(), char_to_remove);
  s.erase(end_it, s.end());
  return s;
}

auto clean(std::string_view input) -> std::string {
  return remove_char(input, ' ');
}

auto match(std::string_view value, std::string_view pattern) -> Data {
  auto rgx = std::regex(std::string(pattern));
  auto result = std::regex_match(std::string(value), rgx);
  return {result, result ? std::string(value) : ""};
}

auto isNumber(std::string_view input) -> Data {
  // Check if there is a digit
  if (!std::regex_search(input.begin(), input.end(), std::regex("\\d"))) {
    return {false, ""}; // It has no digit
  }

  // Check if there is a signal or if it is fractional
  auto str = clean(input);
  if (str.size() >= 2) {
    if (str.starts_with('+')) {
      str = str.substr(1, str.length() - 1);
    }
    if (str.starts_with('.')) {
      str = std::format("0{}", str);
    }
    if (str.at(0) == '-' && str.at(1) == '.') {
      str = str.substr(2, str.length() - 1);
      str = std::format("-0.{}", str);
    }
    if (str.ends_with('.')) {
      str = std::format("{}0", str);
    }
  }

  // Check if it is number
  auto pattern = "^[+-]?(\\d+\\.?\\d*|\\d*\\.\\d+)$";
  return match(str, pattern);
}

auto isInteger(std::string_view input) -> Data {
  auto pattern = "^-?\\d*";
  return match(isNumber(input).value, pattern);
}

auto isFloat(std::string_view input) -> Data {
  auto pattern = "^[+-]?\\d+\\.\\d*$";
  return match(isNumber(input).value, pattern);
}

auto isChar(std::string_view input) -> Data {
  auto result = input.size() == 1;
  return {result, result ? std::string(input) : ""};
}

auto isString(std::string_view input) -> Data {
  auto pattern = "^'.*'$";
  return {match(clean(input), pattern).status, std::string(input)};
}

auto isDictionary(std::string_view input) -> Data {
  auto pattern = "^\\'.+'\\s*:\\s*'?.*'?$";
  return match(clean(input), pattern);
}

auto isGroup(std::string_view input) -> Data {
  auto pattern = "^\\{.*\\}$";
  return match(clean(input), pattern);
}

// Funtion convert string to number (integer or float)
template <typename T>
auto stringviewToNumber(std::string_view sv) -> std::variant<int, float, bool> {
  auto result = isNumber(sv);
  if (result.status) {
    T val;
    std::string_view s = result.value;
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
    if (ec == std::errc() && ptr == s.data() + s.size()) {
      if (val == static_cast<int>(val)) {
        return static_cast<int>(val);
      }
      return static_cast<float>(val);
    }
  }
  return false;
}

auto stringviewToDict(std::string_view input) -> std::variant<Dict, bool> {
  auto result = isDictionary(input);
  if (result.status) {
    // Prepare
    auto values = split(input, ":");
    if (values.size() != 2) {
      return false;
    }
    Data result;
    result = isNumber(values[0]);
    if (result.status) {
      return false;
    }
    result = isString(values[0]);
    if (!result.status) {
      return false;
    }
    return Dict{result.value, std::string(values[1])};
  }

  return false;
}

// --- Viewer Function (uses std::visit for variant) ---
template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

void view(const ParsedData &data) {
  const auto visitor = overloads{
      [](std::vector<int> vec) {
        for (const auto &item : vec) {
          std::print("{} ", item);
        }
        std::println("{}", vec.empty() ? "Unable to parse." : "");
      },
      [](std::vector<float> vec) {
        for (const auto &item : vec) {
          std::print("{} ", item);
        }
        std::println("{}", vec.empty() ? "Unable to parse." : "");
      },
      [](std::vector<char> vec) {
        for (const auto &item : vec) {
          std::print("{} ", item);
        }
        std::println("{}", vec.empty() ? "Unable to parse." : "");
      },
      [](std::vector<std::string> vec) {
        for (const auto &item : vec) {
          std::print("{} ", item);
        }
        std::println("{}", vec.empty() ? "Unable to parse." : "");
      },
      [](std::vector<Dict> vec) {
        for (const auto &item : vec) {
          std::print("Key: {}, Value: {}; ", item.key, item.value);
        }
        std::println("{}", vec.empty() ? "Unable to parse." : "");
      },
  };
  std::visit(visitor, data);
}

auto process(std::string_view input) -> ParsedData {
  // First check
  if (input.empty()) {
    return std::vector<std::string>{};
  }
  return {};
}

// --- Test ---
void test1() {

  auto check = [](std::function<Data(std::string_view)> func, std::string value,
                  std::string expected) -> bool {
    std::string result = func(value).value;
    auto test = result == expected;
    // std::println("Value: '{}'\nExpected: '{}'\nResult: '{}' [{}]", value,
    //              expected, result, test);
    return test;
  };

  // Invalid Numbers
  assert(check(isNumber, "", ""));
  assert(check(isNumber, " ", ""));
  assert(check(isNumber, "A", ""));
  assert(check(isNumber, "?", ""));
  assert(check(isNumber, ".", ""));
  assert(check(isNumber, "-", ""));
  assert(check(isNumber, "+", ""));
  assert(check(isNumber, "+.", ""));
  assert(check(isNumber, "-.", ""));
  assert(check(isNumber, ".-", ""));
  assert(check(isNumber, ".+", ""));
  assert(check(isNumber, ".1a", ""));
  assert(check(isNumber, "1.a", ""));
  assert(check(isNumber, "1. a", ""));
  assert(check(isNumber, "+.1a", ""));
  assert(check(isNumber, "+1.a", ""));
  assert(check(isNumber, "-1.0 a", ""));

  // Numbers
  assert(check(isNumber, ".1", "0.1"));
  assert(check(isNumber, "1.", "1.0"));
  assert(check(isNumber, "+.1", "0.1"));
  assert(check(isNumber, "-.1", "-0.1"));
  assert(check(isNumber, "+1.", "1.0"));
  assert(check(isNumber, "-1.", "-1.0"));
  assert(check(isNumber, "-1.1", "-1.1"));
  assert(check(isNumber, "+1.2", "1.2"));
  assert(check(isNumber, "+1", "1"));
  assert(check(isNumber, "-1", "-1"));
  assert(check(isNumber, "-. 1", "-0.1"));
  assert(check(isNumber, "- 1.", "-1.0"));
  assert(check(isNumber, "+1 .0 ", "1.0"));
  assert(check(isNumber, "+   1 ", "1"));
  assert(check(isNumber, "- 1 ", "-1"));

  // Integers
  assert(check(isInteger, "10", "10"));
  assert(check(isInteger, "+10", "10"));
  assert(check(isInteger, "-10", "-10"));
  assert(check(isInteger, "-101", "-101"));
  assert(check(isInteger, "- 1", "-1"));
  assert(check(isInteger, "+1 ", "1"));
  assert(check(isInteger, "+   1 ", "1"));
  assert(check(isInteger, "- 1 ", "-1"));

  // Floats
  assert(check(isFloat, ".1", "0.1"));
  assert(check(isFloat, "1.", "1.0"));
  assert(check(isFloat, "+.1", "0.1"));
  assert(check(isFloat, "-.1", "-0.1"));
  assert(check(isFloat, "+1.", "1.0"));
  assert(check(isFloat, "-1.", "-1.0"));
  assert(check(isFloat, "-1.1", "-1.1"));
  assert(check(isFloat, "+1.2", "1.2"));
  assert(check(isFloat, "-. 1", "-0.1"));
  assert(check(isFloat, "- 1.", "-1.0"));
  assert(check(isFloat, "+1 .0 ", "1.0"));

  // Characters
  assert(check(isChar, "", ""));
  assert(check(isChar, "..", ""));
  assert(check(isChar, " ", " "));
  assert(check(isChar, "A", "A"));
  assert(check(isChar, "a", "a"));

  // String
  assert(check(isString, "", ""));
  assert(check(isString, " ", " "));
  assert(check(isString, "A", "A"));
  assert(check(isString, "a", "a"));
  assert(check(isString, "..", ".."));
  assert(check(isString, "10", "10"));
  assert(check(isString, "-10.0", "-10.0"));

  // Dictionary
  assert(check(isDictionary, "", ""));
  assert(check(isDictionary, " ", ""));
  assert(check(isDictionary, ".", ""));
  assert(check(isDictionary, "A", ""));
  assert(check(isDictionary, "10", ""));
  assert(check(isDictionary, ":", ""));
  assert(check(isDictionary, ":10", ""));
  assert(check(isDictionary, "A:10", ""));
  assert(check(isDictionary, "'':10", ""));
  assert(check(isDictionary, "'A':'10'", "'A':'10'"));
  assert(check(isDictionary, "'A':-10", "'A':-10"));
  assert(check(isDictionary, "' A ' : - 10", "'A':-10"));
  assert(check(isDictionary, "' A ' : ' Hello '", "'A':'Hello'"));

  // Group
  assert(check(isGroup, "", ""));
  assert(check(isGroup, " ", ""));
  assert(check(isGroup, "A", ""));
  assert(check(isGroup, "{", ""));
  assert(check(isGroup, "}", ""));
  assert(check(isGroup, "}{", ""));
  assert(check(isGroup, "}{}{", ""));
  assert(check(isGroup, "[A]", ""));
  assert(check(isGroup, "{{}}", "{{}}"));
  assert(check(isGroup, "{A}", "{A}"));
}

auto test2() {
  auto typeOf = [](std::variant<int, float, bool> p) -> std::string {
    size_t index = p.index();
    std::vector<std::string> type{"int", "float", "undefined"};
    return index >= 0 && index < type.size() ? type.at(index) : type.back();
  };

  assert(typeOf(stringviewToNumber<int>("10")) == "int");
  assert(typeOf(stringviewToNumber<int>("-10")) == "int");
  assert(typeOf(stringviewToNumber<int>("+10")) == "int");
  assert(typeOf(stringviewToNumber<int>("+ 10")) == "int");
  assert(typeOf(stringviewToNumber<int>("10.5")) == "undefined");
  assert(typeOf(stringviewToNumber<int>("10.")) == "undefined");
  assert(typeOf(stringviewToNumber<float>("10")) == "int");
  assert(typeOf(stringviewToNumber<float>("-10")) == "int");
  assert(typeOf(stringviewToNumber<float>("+10")) == "int");
  assert(typeOf(stringviewToNumber<float>("+ 1 0")) == "int");
  assert(typeOf(stringviewToNumber<float>("+ 1 . 0")) == "int");
  assert(typeOf(stringviewToNumber<float>("10.5")) == "float");
  assert(typeOf(stringviewToNumber<float>("-10.1")) == "float");
  assert(typeOf(stringviewToNumber<float>("+ 10 . 5")) == "float");
}

auto test3() {
  // stringviewToDict(std::string_view input) -> std::variant<Dict, bool>
  auto typeOf = [](std::variant<Dict, bool> p) -> std::string {
    size_t index = p.index();
    std::vector<std::string> type{"dict", "false", "undefined"};
    return index >= 0 && index < type.size() ? type.at(index) : type.back();
  };

  assert(typeOf(stringviewToDict("a")) == "false");
  assert(typeOf(stringviewToDict("1")) == "false");
  assert(typeOf(stringviewToDict("1 : 'a")) == "false");
  assert(typeOf(stringviewToDict("'A' : 10")) == "dict");

  auto a = stringviewToDict("'A' : 'hello'");
  Dict b{"'A' ", " 'hello'"};
  assert(typeOf(a) == "dict");
  Dict c = std::get<Dict>(a);
  assert(b.compare(std::get<Dict>(a)));
}

auto test() {
  // Simple check
  test1();

  // Check stringviewToNumber
  test2();

  // Check stringviewToDict
  test3();
}

// --- Main ---
auto main() -> int {
  test();

  return 0;
}
