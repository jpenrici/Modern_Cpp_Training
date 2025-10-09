#include <bitset>
#include <concepts>
#include <format>
#include <print>

// Rule to accept only int or std::string
template <typename T>
concept Valid = std::same_as<T, int> || std::same_as<T, std::string>;

template <Valid T1, Valid T2> void analyze(T1 a, T2 b) {
  // Type definitions for fixed-size bitsets
  using BitNumber8 = std::bitset<8>;
  using BitNumber16 = std::bitset<16>;
  using BitNumber32 = std::bitset<32>;

  // Lambda - Print
  auto process = [&](auto val_a, auto val_b) {
    auto str = std::string(76, '-');
    std::println("{}", str);
    std::println("| {: <7}| {: <10}| {: <16} | {: <32} |", "OP", "Bits (8)",
                 "Bits (16)", "Bits (32)");
    std::println("{}", str);

    std::println("| A      | {:<8}  | {:<10} | {:<10} |",
                 BitNumber8(val_a).to_string(), BitNumber16(val_a).to_string(),
                 BitNumber32(val_a).to_string());

    std::println("| B      | {:<8}  | {:<10} | {:<10} |",
                 BitNumber8(val_b).to_string(), BitNumber16(val_b).to_string(),
                 BitNumber32(val_b).to_string());

    std::println("{}", str);

    // Bitwise Operations
    // A & B (AND)
    std::println("| A & B  | {:<8}  | {:<10} | {:<10} |",
                 (BitNumber8(val_a) & BitNumber8(val_b)).to_string(),
                 (BitNumber16(val_a) & BitNumber16(val_b)).to_string(),
                 (BitNumber32(val_a) & BitNumber32(val_b)).to_string());

    // A | B (OR)
    std::println("| A | B  | {:<8}  | {:<10} | {:<10} |",
                 (BitNumber8(val_a) | BitNumber8(val_b)).to_string(),
                 (BitNumber16(val_a) | BitNumber16(val_b)).to_string(),
                 (BitNumber32(val_a) | BitNumber32(val_b)).to_string());

    // A ^ B (XOR)
    std::println("| A ^ B  | {:<8}  | {:<10} | {:<10} |",
                 (BitNumber8(val_a) ^ BitNumber8(val_b)).to_string(),
                 (BitNumber16(val_a) ^ BitNumber16(val_b)).to_string(),
                 (BitNumber32(val_a) ^ BitNumber32(val_b)).to_string());

    // ~A (NOT / One's Complement)
    std::println("| ~A     | {:<8}  | {:<10} | {:<10} |",
                 (~BitNumber8(val_a)).to_string(),
                 (~BitNumber16(val_a)).to_string(),
                 (~BitNumber32(val_a)).to_string());

    std::println("{}", str);
  };

  // Main Logic
  std::println("--- Analyzing Types: {} / {} ---",
               (std::is_same_v<T1, int> ? "int" : "std::string"),
               (std::is_same_v<T2, int> ? "int" : "std::string"));

  std::println("Input A: {}", a);
  std::println("Input B: {}", b);

  auto checkString = [](std::string str) {
    auto result = str.find_first_not_of("01") != std::string::npos;
    if (result) {
      std::println(
          "Error: Binary string error! Character different from 0 and 1.");
    }
    return result;
  };

  // Conditional Bitset Creation
  if constexpr (std::is_integral_v<T1> && std::is_integral_v<T2>) {
    process(static_cast<unsigned long long>(a),
            static_cast<unsigned long long>(b));
  } else if constexpr (std::is_integral_v<T1> &&
                       std::same_as<T2, std::string>) {
    if (!checkString(b)) {
      process(static_cast<unsigned long long>(a), b);
    }
  } else if constexpr (std::same_as<T1, std::string> &&
                       std::is_integral_v<T2>) {
    if (!checkString(a)) {
      process(a, static_cast<unsigned long long>(b));
    }
  } else {
    if (!checkString(a) && !checkString(b)) {
      process(a, b);
    }
  }
}

auto main() -> int {

  analyze<std::string, std::string>("00000001", "10000000");
  analyze(255, 64);

  analyze<std::string, int>("00000001", 255);

  // String error
  analyze<std::string, std::string>("0000000a", "1000000b");

  // analyze('0', '1'); // Not accepted by Concepts.

  analyze(0, 99999);     // Accepted, int
  analyze(0, 999999999); // Accepted, int
  // analyze(0, 9999999999); // Not accepted by Concepts, long long

  return 0;
}
