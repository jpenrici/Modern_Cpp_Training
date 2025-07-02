/*
 * This code focuses on training modern C++ and manipulating types and strings.
 */

#include "../typeParser/typeParser.hpp"

#include <print>

// --- Main ---
auto main() -> int {

  std::println("Example:");
  view(process("{'K1' : 10, 'K2' : -2.5, 'K3' : 'Hi'}"));

  return 0;
}
