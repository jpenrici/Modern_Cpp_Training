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

auto test4() {

  auto typeOf =
      [](const ParsedData &p) -> std::string { // Pass by const reference
    size_t index = p.index();
    std::vector<std::string> type_names{"integer", "fractional", "character",
                                        "string",  "dictionary", "empty"};
    // Ensure index is valid before accessing
    if (index < type_names.size()) {
      return type_names.at(index);
    }
    return "undefined";
  };

  // Input
  std::vector<std::vector<std::string_view>> data{
      {"0", "integer"},      // integer
      {"-1", "integer"},     // negative integer
      {".5", "fractional"},  // fractional number (leading dot)
      {"5.", "integer"},     // integer, number (trailing dot)
      {"+.5", "fractional"}, // fractional number (with sign and leading dot)
      {"2.5", "fractional"}, // fractional number
      {"'A'", "character"},  // character
      {"'Hello'", "string"}, // string
      {"'Hello World'", "string"},                       // string with space
      {"{}", "empty"},                                   // Empty group
      {"{10,11,12,13,14,15}", "integer"},                // vector of integers
      {"{0.1,1.2,2.3,3.4,4.5,5.6}", "fractional"},       // vector of floats
      {"{-1, -2, +3, +4, 5, -10}", "integer"},           // vector of integers
      {"{1, 2.5, +3, 4, -5, 6, 7.9, 10}", "fractional"}, // vector of fractional
      {"{'A', 'B', 'C', '1.5', '+10.5'}", "string"},     // vector of strings
      {"{'a' : '1', 'b' : 'Hi'}", "dictionary"},         // dictionary
      {"{'K1' : 10, 'K2' : -2.5, 'K3' : 'Hi'}", "dictionary"}, // dictionary
      {" '  some string  ' ", "string"},  // String with external whitespace
      {" {-1 , +2.5, 'test'} ", "string"} // Mixed
  };

  // Processing and output
  std::println("--- Processing Inputs ---");
  for (auto item : data) {
    auto itemValue = item.at(0);
    ParsedData p = process(item.at(0));
    auto itemType = typeOf(p);
    auto expected = item.at(1);
    std::println("Input: \"{}\"", itemValue);
    std::print("Output: ");
    view(p);
    std::println("Type: {}", itemType);
    std::println("---");
    if (itemType != expected) {
      std::println("Expected: '{}'", expected);
      break;
    }
  }
}

auto test() {
  // Simple check
  test1();

  // Check stringviewToNumber
  test2();

  // Check stringviewToDict
  test3();

  // Check processing
  test4();

  std::println("Test completed!");
}

// --- Main ---
auto main() -> int {

  test4();

  return 0;
}
