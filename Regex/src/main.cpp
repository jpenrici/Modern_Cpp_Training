/*
 * Training Regex and ReDoS (Regular Expression Denial of Service)
 *
 * Reference:
 *  https://en.cppreference.com/w/cpp/header/regex.html
 *  https://en.wikipedia.org/wiki/ReDoS
 */

#include <chrono>
#include <future>
#include <mutex>
#include <print>
#include <regex>
#include <thread>

// -- Main Functions --

// Global mutex to protect access to regex operation
std::mutex g_regex_mutex;

// Function checks for errors in the pattern that could be problematic
auto checkRgx(std::string &pattern) -> bool {
  if (pattern.empty()) {
    std::println("Empty pattern!");
    return false;
  }

  // Known problematic patterns for ReDoS
  std::vector<std::regex> dangerous_patterns = {
      std::regex(R"(\(.+\)\+)"),           // Groups with nested +: (a+)+
      std::regex(R"(\(.+\)\*)"),           // Groups with nested *: (a*)*
      std::regex(R"(\(.+\)\{.+\})"),       // Groups with quantifiers: (a+){2,}
      std::regex(R"(\([^)]*\|[^)]*\)\*)"), // Alternation with * (a|a)*
  };

  for (const auto &dangerous : dangerous_patterns) {
    if (std::regex_search(pattern, std::regex(dangerous))) {
      std::println("Warning: Potentially dangerous pattern detected!");
      return false;
    }
  }

  return true;
}

// Function that executes the Regex
auto find(std::string &text, std::string &pattern, std::string &response,
          std::chrono::milliseconds timeout_duration, bool check_pattern = true)
    -> bool {

  if (check_pattern) {
    if (!checkRgx(pattern)) {
      std::println("Potentially problematic pattern!");
      return false;
    }
  }

  // Initialize protection
  std::lock_guard<std::mutex> lock(g_regex_mutex);

  // Time control
  std::promise<std::string> promise_result;
  std::future<std::string> future_result = promise_result.get_future();

  std::jthread regex_thread([&](std::stop_token st) {
    std::smatch m;
    try {
      auto rgx = std::regex(pattern);
      auto found = std::regex_search(text, m, rgx);
      if (found) {
        // Match found, assign found value
        promise_result.set_value(m.str(0));
      } else {
        // If nothing found, assign empty
        promise_result.set_value({});
      }
    } catch (const std::regex_error &e) {
      std::println("Regex error in jthread: {}", e.what());
      promise_result.set_value({});
    } catch (...) {
      std::println("Unknown error in jthread.");
      promise_result.set_value({});
    }
  });

  // Wait for response
  std::future_status status = future_result.wait_for(timeout_duration);
  if (status == std::future_status::ready) {
    // The regex operation finished within the timeout
    std::string result = future_result.get();
    // Handle response
    response = result;
    return !result.empty();
  } else if (status == std::future_status::timeout) {
    std::println("Regex operation timed out after {} ms!",
                 timeout_duration.count());
    regex_thread.request_stop();
    return false;
  } else {
    std::println("Regex operation did not finish or was deferred (unexpected "
                 "with jthread).");
    return false;
  }
}

// Function makes indirect call to the find function
auto process(std::string &text, std::string &pattern,
             std::chrono::milliseconds timeout_duration,
             bool check_pattern = true) -> std::string {
  std::string response;
  std::string status_msg;
  status_msg = find(text, pattern, response, timeout_duration, check_pattern)
                   ? response
                   : "No, nothing found or execution aborted!";
  return status_msg;
}

// Function displays searches
auto view(
    std::string text, std::string pattern,
    std::chrono::milliseconds timeout_duration = std::chrono::milliseconds(100),
    bool check_pattern = true) {
  auto status = process(text, pattern, timeout_duration, check_pattern);
  std::println("Input: {}\nPattern: {}\nStatus: {}", text, pattern, status);
  std::println();
}

// --- Test ---
void test() {
  std::string text =
      "Hello, my phone number is +55 (01) 98765-4321 and my email "
      "is exemplo@dominio.com.br.";

  std::println("--- Normal tests (default timeout 100ms) ---");
  view(text, "phone");    // phone
  view(text, "...phone"); // my phone
  view(text, "\\d+");     // 55
  view(text,
       "\\+\\d{2}\\s\\(\\d{2}\\)\\s\\d{4,5}-\\d{4}"); //+55 (01) 98765-4321
  view(text, "");                                     // No, nothing found!

  auto short_timeout = std::chrono::milliseconds(10);
  auto longest_timeout = std::chrono::milliseconds(5000);

  std::println("--- Simulated ReDoS test (short timeout) ---");
  std::string redos_pattern = "(a+)+";
  std::string redos_text = std::string(30, 'a');
  // if checking disabled: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
  view(redos_text, redos_pattern, short_timeout, false);

  std::println("--- Simulated ReDoS test (longest timeout) ---");
  // if checking disabled: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa
  view(redos_text, redos_pattern, longest_timeout, false);

  std::println("-- Simulated ReDoS Test (Email Validation) ---");
  redos_pattern =
      "([a-zA-Z0-9])(([\\-.]|[_]+)?([a-zA-Z0-9]+))*(@){1}[a-z0-9]+[.]{1}(([a-"
      "z]{2,3})|([a-z]{2,3}[.]{1}[a-z]{2,3}))";
  // if checking disabled: exemplo@dominio.com
  view(text, redos_pattern, short_timeout, false);
  // if checking disabled: Regex operation timed out after 10 ms!
  view("aaaaaaaaaaaaaaaaaaaaaaaa!", redos_pattern, short_timeout, false);

  // Other tests for different types of ReDoS
  std::println("-- Simulated ReDoS Test (Others) ---");
  std::vector<std::pair<std::string, std::string>> redos_tests = {
      {"(a+)+", std::string(20, 'a')},
      {"(a*)*", std::string(20, 'a')},
      {"(a|a)*", std::string(20, 'a')},
      {"(a|b)*a", std::string(20, 'b') + "c"}};

  for (const auto &[pattern, text] : redos_tests) {
    std::println("Testing ReDoS pattern: {}", pattern);
    view(text, pattern, short_timeout);
  }
}

// --- Main ---
auto main() -> int {
  test();
  return 0;
}
