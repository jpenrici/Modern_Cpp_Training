#include <expected>
#include <print>

using Permission = bool;
using PermissionError = std::string;

auto checkPermissionByAge(int age)
    -> std::expected<Permission, PermissionError> {

  // Invalid input!
  if (age < 0) {
    return std::unexpected("Invalid value!");
  }

  // Returns permission if of legal age.
  return age >= 18;
}

auto test1(int age) {
  auto result = checkPermissionByAge(age);
  if (result.has_value()) {
    std::println("Value is {} year old. {}", age,
                 result.value() ? "Permission granted. Access is allowed"
                                : "Permission denied. Access is restricted");
  } else {
    std::println("An error occurred: {}", result.error());
  }
}

auto test2(int age) {
  auto result = checkPermissionByAge(age);
  try {
    std::println("Value is {} year old. {}", age,
                 result.value() ? "Permission granted. Access is allowed"
                                : "Permission denied. Access is restricted");
  } catch (const std::bad_expected_access<PermissionError> &e) {
    std::println("An error occurred: {}", e.error());
  }
}

auto main() -> int {

  test1(25); // Value is 25 year old. Permission granted. Access is allowed
  test1(16); // Value is 16 year old. Permission denied. Access is restricted
  test1(-5); // An error occurred: Invalid value!

  test2(-1); // An error occurred: Invalid value!

  return 0;
}
