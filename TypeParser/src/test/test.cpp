/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */

#include "../typeParser/typeParser.hpp"

#include <functional>

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

  std::println("Test completed!");
}

// --- Main ---
auto main() -> int {

  test();

  return 0;
}
