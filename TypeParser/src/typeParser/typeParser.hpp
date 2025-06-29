/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */
#pragma once

#include <cassert>
#include <print>
#include <string_view>
#include <vector>
#include <variant>

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
               std::string_view chars_to_trim = WHITESPACE) -> std::string;

auto trim_right(std::string_view input,
                std::string_view chars_to_trim = WHITESPACE) -> std::string ;

auto trim(std::string_view input, std::string_view chars_to_trim = WHITESPACE)
    -> std::string ;

// Funtion to split string_view
auto split(std::string_view sv, std::string_view delimiter)
    -> std::vector<std::string_view> ;

auto remove_char(std::string_view input, char char_to_remove) -> std::string ;

auto clean(std::string_view input) -> std::string ;

auto match(std::string_view value, std::string_view pattern) -> Data ;

auto isNumber(std::string_view input) -> Data ;

auto isInteger(std::string_view input) -> Data ;

auto isFloat(std::string_view input) -> Data ;

auto isChar(std::string_view input) -> Data ;
auto isString(std::string_view input) -> Data ;

auto isDictionary(std::string_view input) -> Data ;

auto isGroup(std::string_view input) -> Data ;

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

// Function convert string to struct
auto stringviewToDict(std::string_view input) -> std::variant<Dict, bool> ;

// --- Viewer Function (uses std::visit for variant) ---
template <class... Ts> struct overloads : Ts... {
  using Ts::operator()...;
};

void view(const ParsedData &data) ;

auto process(std::string_view input) -> ParsedData ;
