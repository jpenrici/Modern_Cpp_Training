#include "typeParser.hpp"

#include <regex>

auto trim_left(std::string_view input, std::string_view chars_to_trim)
    -> std::string {
  size_t first_char = input.find_first_not_of(chars_to_trim);
  if (std::string_view::npos == first_char) {
    return {};
  }
  return std::string(input.substr(first_char));
}

auto trim_right(std::string_view input, std::string_view chars_to_trim)
    -> std::string {
  size_t last_char = input.find_last_not_of(chars_to_trim);
  if (std::string_view::npos == last_char) {
    return {};
  }
  return std::string(input.substr(0, last_char + 1));
}

auto trim(std::string_view input, std::string_view chars_to_trim)
    -> std::string {
  auto trimmed_left = trim_left(input, chars_to_trim);
  return trim_right(trimmed_left, chars_to_trim);
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
      [](bool v) { std::println("{}", v ? "Empty" : "Unable to parse."); },
  };
  std::visit(visitor, data);
}

auto process(std::string_view input) -> ParsedData {
  // First check
  if (input.empty()) {
    return false; // Unable to parse
  }

  // Trim whitespace from the input
  input = trim(input);

  // Data structure to hold elements found, initially just the input itself
  std::vector<std::string_view> elements;
  Data data;

  // 1. Check if it is a group of values
  data = isGroup(input);
  if (data.status) {
    // Remove leading '{' and trailing '}'
    std::string_view content = input.substr(1, input.length() - 2);
    // Split the content by comma for individual elements
    elements = split(content, ",");
    if (elements.empty()) {
      return true; // Nothing to do
    }
  } else {
    // If not a group, the input itself is the single element to process
    elements.push_back(input);
  }

  // Vectors to collect parsed types
  std::vector<int> integers;
  std::vector<float> floats;
  std::vector<char> characters;
  std::vector<std::string> strings;
  std::vector<Dict> dictionary;

  for (auto &e : elements) {
    auto trimmed_e = trim(e);

    Data number = isNumber(trimmed_e);
    if (number.status) {
      data = isInteger(number.value);
      if (data.status) {
        auto num_variant = stringviewToNumber<int>(data.value);
        if (std::holds_alternative<int>(num_variant)) {
          integers.push_back(std::get<int>(num_variant));
          continue;
        }
      }
      data = isFloat(number.value);
      if (data.status) {
        auto num_variant = stringviewToNumber<float>(data.value);
        if (std::holds_alternative<float>(num_variant)) {
          floats.push_back(std::get<float>(num_variant));
          continue;
        }
        if (std::holds_alternative<int>(num_variant)) {
          integers.push_back(std::get<int>(num_variant));
          continue;
        }
      }
    }

    data = isDictionary(trimmed_e);
    if (data.status) {
      std::println("Dict: {}", data.value);
      auto num_variant = stringviewToDict(data.value);
      if (std::holds_alternative<Dict>(num_variant)) {
        dictionary.push_back(std::get<Dict>(num_variant));
      }
      continue;
    }

    data = isChar(trim(trimmed_e, "'"));
    if (data.status) {
      characters.push_back(data.value[0]);
      continue;
    }

    data = isString(trimmed_e);
    if (data.status) {
      auto s = trim(data.value, "'");
      strings.push_back(s);
    }
  }

  size_t expectedSize = elements.size();
  if (dictionary.size() == expectedSize) {
    return dictionary;
  }
  if (integers.size() == expectedSize) {
    return integers;
  }
  if (floats.size() == expectedSize) {
    return floats;
  }
  if (characters.size() == expectedSize) {
    return characters;
  }
  if (strings.size() == expectedSize) {
    return strings;
  }

  return false; // Unable to parse
}
