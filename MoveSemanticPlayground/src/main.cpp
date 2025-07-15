/*
 *  References:
 *      https://learnmoderncpp.com/
 *      https://en.cppreference.com/w/cpp/utility/move.html
 */

#include <print>
#include <string>
#include <utility>

template <typename T> struct MyStruct {
  T value;

  // Default Constructor
  MyStruct() : value(T{}) {
    std::println("Default Constructor <{}>: value = {}", typeid(T).name(),
                 value);
  }

  // Constructor with initial value
  MyStruct(const T &val) : value(val) {
    std::println("Value Constructor <{}>: value = {}", typeid(T).name(), value);
  }

  // Copy Constructor
  MyStruct(const MyStruct &other) : value(other.value) {
    std::println("Copy Constructor <{}>: value = {} (copied from {})",
                 typeid(T).name(), value, other.value);
  }

  // Move Constructor
  MyStruct(MyStruct &&other) noexcept : value(std::move(other.value)) {
    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
      other.value = T{-1}; // Sentinel for numeric types
    } else if constexpr (std::is_same_v<T, std::string>) {
      // std::string::operator= (string&&) leaves source empty or valid but
      // unspecified We'll just print its state later
    }
    std::println("Move Constructor <{}>: value = {} (moved from old value: {})",
                 typeid(T).name(), value, other.value);
  }

  // Copy Assignment Operator
  MyStruct &operator=(const MyStruct &other) {
    if (this != &other) {
      value = other.value;
      std::println("Copy Assignment Operator <{}>: value = {} (copied from {})",
                   typeid(T).name(), value, other.value);
    }
    return *this;
  }

  // Move Assignment Operator
  MyStruct &operator=(MyStruct &&other) noexcept {
    if (this != &other) {
      value = std::move(other.value);
      if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
        other.value = T{-1}; // Sentinel for numeric types
      } else if constexpr (std::is_same_v<T, std::string>) {
        // std::string::operator= (string&&) leaves source empty or valid but
        // unspecified We'll just print its state later
      }
      std::println("Move Assignment Operator <{}>: value = {} (moved from old "
                   "value: {})",
                   typeid(T).name(), value, other.value);
    }
    return *this;
  }

  // Destructor
  virtual ~MyStruct() {
    std::println("Destructor <{}>: value = {}", typeid(T).name(), value);
  }

  // Ref-qualified overload for non-const lvalues
  void show() & {
    std::println("show() & <{}> - called on an lvalue. Current value: {}",
                 typeid(T).name(), value);
  }

  // Ref-qualified overload for const lvalues
  void show() const & {
    std::println(
        "show() const & <{}> - called on a const lvalue. Current value: {}",
        typeid(T).name(), value);
  }

  // Ref-qualified overload for rvalues
  void show() && {
    std::println("show() && <{}> - called on an rvalue. Current value: {}",
                 typeid(T).name(), value);
    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>) {
      value = T{-2}; // Indicate it's been used as an rvalue and modified
    } else if constexpr (std::is_same_v<T, std::string>) {
      value = "MOVED_RVALUE_USED"; // Example for string
    }
  }
};

template <typename T>
void testMyStruct(const std::string &typeName, const T &initialValue) {
  std::println();
  std::println("--- Testing MyStruct with type: {} ---", typeName);

  MyStruct<T> obj1(initialValue);
  MyStruct<T> &obj2 = obj1;

  const MyStruct<T> obj3(initialValue);
  const MyStruct<T> &obj4 = obj3;

  std::println("--- Calling show() on lvalues and const lvalues (Type: {}) ---",
               typeName);
  obj1.show(); // Calls show() &
  obj2.show(); // Calls show() &
  obj3.show(); // Calls show() const &
  obj4.show(); // Calls show() const &

  std::println("--- Demonstrating std::move (Type: {}) ---", typeName);
  std::println("obj1 BEFORE std::move().show(): value = {}", obj1.value);
  std::move(obj1).show();
  std::println("obj1 AFTER std::move().show(): value = {}", obj1.value);

  std::println("--- Demonstrating temporary rvalue (Type: {}) ---", typeName);
  MyStruct<T> temporary(initialValue + initialValue);
  temporary.show();

  // Test move assignment
  std::println("--- Testing Move Assignment (Type: {}) ---", typeName);
  MyStruct<T> obj_src(initialValue + initialValue + initialValue);
  MyStruct<T> obj_dest;
  std::println("obj_src BEFORE move assignment: value = {}", obj_src.value);
  std::println("obj_dest BEFORE move assignment: value = {}", obj_dest.value);
  obj_dest = std::move(obj_src);
  std::println("obj_src AFTER move assignment: value = {}", obj_src.value);
  std::println("obj_dest AFTER move assignment: value = {}", obj_dest.value);

  std::println("-- End of testing for {} ---", typeName);
  std::println();
}

auto main() -> int {
  std::println("Modern C++ - Training");

  // Test with various types
  testMyStruct<int>("int", 10);
  testMyStruct<float>("float", 20.5f);
  testMyStruct<double>("double", 30.75);
  testMyStruct<char>("char", 'A');
  testMyStruct<std::string>("std::string", "Hello Template");

  std::println("--- All tests completed. Objects being destroyed ---");
  return 0;
}
