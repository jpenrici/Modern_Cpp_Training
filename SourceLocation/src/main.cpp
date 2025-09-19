#include <print>
#include <source_location>

/*
 * <source_location> is especially useful for logging,
 *  debugging, and code validation, providing detailed context
 *  without the need for preprocessor macros like __FILE__ or __LINE__.
 */

// Define log levels for better filtering
enum class LogLevel { Debug, Info, Warning, Error };

// Helper function to convert LogLevel to a string
std::string to_string(LogLevel level) {
  switch (level) {
  case LogLevel::Debug:
    return "DEBUG";
  case LogLevel::Info:
    return "INFO";
  case LogLevel::Warning:
    return "WARNING";
  case LogLevel::Error:
    return "ERROR";
  }
  return "UNKNOWN";
}

// Defining concepts for using log_message
template <typename T>
concept Rules =
    std::is_arithmetic_v<T> || // Numeric and bool types
    std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> ||
    std::is_convertible_v<T, const char *>;

template <typename T>
  requires Rules<T>
void log_message(
    LogLevel level, const T &message,
    const std::source_location location = std::source_location::current()) {
  std::println("[{}] {}: {} ({}) {}", to_string(level), location.file_name(),
               location.line(), location.function_name(), message);
}

auto main() -> int {
  std::println("Modern C++");

  log_message(LogLevel::Info, "Application started.");

  // Other messages
  log_message(LogLevel::Info, "This is a const char* message");
  log_message(LogLevel::Info, std::string_view("This is a string_view"));
  log_message(LogLevel::Error, 505);
  log_message(LogLevel::Warning, false);

  try {
    log_message(LogLevel::Debug, "Attempting to connect to database ...");

    throw std::runtime_error(
        "Database connection failed!"); // Simulate an error

  } catch (const std::exception &e) {
    log_message(LogLevel::Error, "Exception caught: " + std::string(e.what()));
  }

  log_message(LogLevel::Info, "Application finished.");

  return 0;
}
