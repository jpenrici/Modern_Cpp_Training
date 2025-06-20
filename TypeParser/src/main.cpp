/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */

#include <charconv>
#include <print>
#include <regex>
#include <variant>
#include <vector>

// --- Structs and Type Aliases ---
struct Status {
  bool status = false;
  std::string message;
};

// Define a variant type for the possible return types of parsed data
using ParsedData = std::variant<std::vector<int>, std::vector<float>,
                                std::vector<std::string>>;

// --- Helper Functions ---
auto match(std::string_view value, std::string_view pattern,
           std::string_view message) -> Status {
  auto rgx = std::regex(std::string(pattern));
  auto result = std::regex_match(std::string(value), rgx);
  return {result, result ? "message" : "not " + std::string(message)};
}

auto isInteger(std::string_view value) -> Status {
  return match(value, std::string_view("^[+,-]?\\d*"), "integer");
}

auto isFloat(std::string_view value) -> Status {
  // Matches integers (e.g., "123") and floats (e.g., "123.45", ".5", "5.")
  return match(value, "^[+-]?(\\d+\\.?\\d*|\\d*\\.\\d+)$", "float");
}

auto isCharacter(std::string_view value) -> Status {
  return match(value, std::string_view("^'.'$"), "character");
}

auto isString(std::string_view value) -> Status {
  return match(value, std::string_view("^'.*'$"), "string");
}

auto isGroup(std::string_view value) -> Status {
  return match(value, std::string_view("^\\{.*\\}$"), "group");
}

// Function to trim leading/trailing whitespace
std::string_view trim(std::string_view s) {
  const auto pattern = " \t\n\r\f\v";
  s.remove_prefix(std::min(s.find_first_not_of(pattern), s.size()));
  s.remove_suffix(
      std::min(s.size() - s.find_last_not_of(pattern) - 1, s.size()));
  return s;
}

// Function to split
// To do

// --- Core Processing Function ---
ParsedData process(std::string_view input) {
  // First check
  if (input.empty()) {
    return std::vector<std::string>{};
  }

  // Trim whitespace from the input
  input = trim(input);

  Status result;

  // 1. Check if it is a group of values
  result = isGroup(input);
  if (result.status) {
    // Remove leading '{' and trailing '}'
    std::string_view content = input.substr(1, input.length() - 2);

    // Find individual elements
    std::vector<std::string_view> elements;
    size_t start = 0;
    size_t comma_pos = 0;
    int brace_count =
        0; // To handle nested groups, though not fully implemented here
    int quote_count = 0; // To handle commas inside strings

    for (size_t i = 0; i < content.length(); ++i) {
      if (content[i] == '\'') {
        quote_count++;
      } else if (content[i] == '{') {
        brace_count++;
      } else if (content[i] == '}') {
        brace_count--;
      } else if (content[i] == ',' && brace_count == 0 &&
                 quote_count % 2 == 0) {
        elements.push_back(trim(content.substr(start, i - start)));
        start = i + 1;
      }
    }
    elements.push_back(trim(content.substr(start))); // Add the last element

    // Try as integers
    std::vector<int> int_vec;
    bool all_integers = true;
    for (const auto &elem_str : elements) {
      int val;
      auto [ptr, ec] = std::from_chars(elem_str.data(),
                                       elem_str.data() + elem_str.size(), val);
      if (ec == std::errc() && ptr == elem_str.data() + elem_str.size()) {
        int_vec.push_back(val);
      } else {
        all_integers = false;
        break;
      }
    }
    if (all_integers && !int_vec.empty()) {
      return int_vec;
    }

    // Try as floats
    std::vector<float> float_vec;
    bool all_floats = true;
    for (const auto &elem_str : elements) {
      float val;
      auto [ptr, ec] = std::from_chars(elem_str.data(),
                                       elem_str.data() + elem_str.size(), val);
      if (ec == std::errc() && ptr == elem_str.data() + elem_str.size()) {
        float_vec.push_back(val);
      } else {
        all_floats = false;
        break;
      }
    }
    if (all_floats && !float_vec.empty()) {
      return float_vec;
    }

    // Default to strings if not all integers or floats
    std::vector<std::string> string_vec;
    for (const auto &elem_str : elements) {
      Status str_status = isString(elem_str);
      if (str_status.status) {
        // Remove quotes from string literals
        string_vec.push_back(
            std::string(elem_str.substr(1, elem_str.length() - 2)));
      } else {
        string_vec.push_back(
            std::string(elem_str)); // Keep as is if not a string literal
      }
    }
    return string_vec;
  }

  // 2. Check if it is a number (float or integer)
  // 2.1 Check if it is a float
  result = isFloat(input);
  if (result.status) {
    float val;
    auto [ptr, ec] =
        std::from_chars(input.data(), input.data() + input.size(), val);
    if (ec == std::errc() && ptr == input.data() + input.size()) {
      return std::vector<float>{val};
    }
  }

  // 2.2 Check if it is a integer
  result = isInteger(input);
  if (result.status) {
    int val;
    auto [ptr, ec] =
        std::from_chars(input.data(), input.data() + input.size(), val);
    if (ec == std::errc() && ptr == input.data() + input.size()) {
      return std::vector<int>{val};
    }
  }

  // 3. Check if it is a string or character
  result = isString(input);
  if (result.status) {
    // Remove quotes from string literals
    return std::vector<std::string>{
        std::string(input.substr(1, input.length() - 2))};
  }

  // If nothing matches, return an empty vector of strings as a fallback
  return std::vector<std::string>{};
}

// --- Viewer Function (uses std::visit for variant) ---
void view(const ParsedData &data) {
  std::visit(
      [](const auto &vec) {
        if (vec.empty()) {
          std::println("Empty or unrecognized input.");
          return;
        }
        for (const auto &item : vec) {
          std::print("{} ", item);
        }
        std::println(); // Newline after printing the vector
      },
      data);
}

auto main() -> int {

  // Input
  std::vector<std::string_view> data{
      "0",                                     // integer
      "-1",                                    // negative integer
      "2.5",                                   // fractional number
      "'A'",                                   // character
      "'Hello'",                               // string
      "'Hello World'",                         // string with space
      "{-1, -2, +3, +4, 5, 6, 7, -8, 9, -10}", // vector of integers
      "{1, 2.5, +3, 4, -5, 6, 7.9, 8, 9, 10}", // vector of fractional number
      "{'A', 'B', 'C', '1.5', '+10.5'}",       // vector of strings
      "{'A1 : 10', 'A2' : 'Hello'}"};          // dictionary

  // Processing and output
  std::println("--- Processing Inputs ---");
  for (auto item : data) {
    std::println("Input: \"{}\"", item);
    ParsedData p = process(item);
    std::print("Output: ");
    view(p);
    std::println("---");
  }

  return 0;
}
