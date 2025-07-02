/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */
#pragma once

#include <cassert>
#include <print>
#include <string_view>
#include <variant>
#include <vector>

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
                 std::vector<std::string>, std::vector<Dict>, bool>;

// --- Constants ---
const std::string_view WHITESPACE = " \t\n\r\f\v";

// --- Helper Functions ---

// Function removes character to the left
auto trim_left(std::string_view input,
               std::string_view chars_to_trim = WHITESPACE) -> std::string;

// Function removes character to the left
auto trim_right(std::string_view input,
                std::string_view chars_to_trim = WHITESPACE) -> std::string;

// Function removes characters at the ends
auto trim(std::string_view input, std::string_view chars_to_trim = WHITESPACE)
    -> std::string;

// Function removes all characters found
auto remove_char(std::string_view input, char char_to_remove) -> std::string;

// Function removes all spaces
auto clean(std::string_view input) -> std::string;

// Funtion to split string
auto split(const std::string &str, const char &delimiter)
    -> std::vector<std::string>;

// Funtion to split string_view
auto split(std::string_view sv, std::string_view delimiter)
    -> std::vector<std::string_view>;

// Function checks regular expression
auto match(std::string_view value, std::string_view pattern) -> Data;

// Function checks if string can be number
auto isNumber(std::string_view input) -> Data;

// Function checks if string can be integer
auto isInteger(std::string_view input) -> Data;

// Function to check if string can be a fractional number
auto isFloat(std::string_view input) -> Data;

// Function to check if string can be character
auto isChar(std::string_view input) -> Data;

// Function checks if string is string
auto isString(std::string_view input) -> Data;

// Function checks if string can be a key and value structure
auto isDictionary(std::string_view input) -> Data;

// Function checks if string can be a set of strings
auto isGroup(std::string_view input) -> Data;

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

// Function converts string into key and value structure
auto stringviewToDict(std::string_view input) -> std::variant<Dict, bool>;

// --- Viewer Function (uses std::visit for variant) ---
template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

// Displays the converted dataset
void view(const ParsedData &data);

// Process input and convert type
auto process(std::string_view input) -> ParsedData;
