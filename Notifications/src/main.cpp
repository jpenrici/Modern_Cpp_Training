#include <chrono>
#include <format>
#include <functional>
#include <map>
#include <print>
#include <string>
#include <vector>

// --- Main Idea - Notification and Notification Center Classes ---
class Notification {
public:
  enum class NotificationType { Info, Warning, Error, Success };

  Notification(NotificationType type, std::string_view msg)
      : notificationType(type), message(msg),
        timestamp(std::chrono::system_clock::now()) {}

  auto notificationTypeToString() const -> std::string {
    switch (notificationType) {
    case NotificationType::Info:
      return "INFO";
    case NotificationType::Warning:
      return "WARNING";
    case NotificationType::Error:
      return "ERROR";
    case NotificationType::Success:
      return "SUCCESS";
    default:
      return "UNKNOWN";
    }
  }

  auto getMessage() const -> const std::string & { return message; }
  auto getType() const -> NotificationType { return notificationType; }
  auto getTimestamp() const -> std::chrono::system_clock::time_point {
    return timestamp;
  }

  // *** Untested - Using newer version of GCC and Operating System ***
  // auto toString() const -> std::string {
  //   std::string formatted_time = std::format("{:%Y-%m-%d %H:%M:%S}",
  //   timestamp); return "[" + formatted_time + "] [" +
  //   notificationTypeToString() +
  //          "]: " + message;
  // }

  auto toString() const -> std::string {
    std::time_t c_time = std::chrono::system_clock::to_time_t(timestamp);
    std::tm tm_buf;
#ifdef _MSC_VER
    localtime_s(&tm_buf, &c_time);
#else
    std::localtime(&c_time);
    tm_buf = *std::localtime(&c_time);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return "[" + ss.str() + "] [" + notificationTypeToString() +
           "]: " + message;
  }

private:
  NotificationType notificationType;
  std::string message;
  std::chrono::system_clock::time_point timestamp;
};

class NotificationCenter {
public:
  // Alias for a listener function, which takes a const Notification reference
  using Listener = std::function<void(const Notification &)>;

  // Registers a listener for a specific notification type
  void registerListener(Notification::NotificationType type,
                        Listener listener) {
    listenersByType[type].push_back(listener);
  }

  // Registers a listener that will be notified for ALL types of notifications
  void registerGlobalListener(Listener listener) {
    globalListeners.push_back(listener);
  }

  // Dispatches a notification to all relevant listeners
  void notify(Notification::NotificationType type, std::string_view message) {
    Notification notification(type, message); // Create the notification object

    // Notify type-specific listeners
    if (listenersByType.count(type)) {
      for (const auto &listener : listenersByType[type]) {
        listener(notification);
      }
    }

    // Notify global listeners
    for (const auto &listener : globalListeners) {
      listener(notification);
    }
  }

private:
  // Map to store type-specific listeners
  std::map<Notification::NotificationType, std::vector<Listener>>
      listenersByType;
  // Vector to store global listeners
  std::vector<Listener> globalListeners;
};

// --- Test - Simulation 1 ---
class FileProcessor {
public:
  // Constructor takes a reference to the NotificationCenter
  explicit FileProcessor(NotificationCenter &nc) : notificationCenter(nc) {}

  void performFileOperation(const std::string &filename, bool success) {
    if (success) {
      notificationCenter.notify(Notification::NotificationType::Success,
                                "File '" + filename + "' saved successfully.");
    } else {
      notificationCenter.notify(Notification::NotificationType::Error,
                                "Failed to save file '" + filename +
                                    "'. Permission denied.");
    }
  }

private:
  NotificationCenter &notificationCenter;
};

// --- Test - Simulation 2 ---
class DataAnalyzer {
public:
  // Constructor takes a reference to the NotificationCenter
  explicit DataAnalyzer(NotificationCenter &nc) : notificationCenter(nc) {}

  void processData(int value) {
    if (value < 0) {
      notificationCenter.notify(
          Notification::NotificationType::Warning,
          "Negative value detected: " + std::to_string(value) +
              ". Processing continued with caution.");
    } else if (value == 0) {
      notificationCenter.notify(Notification::NotificationType::Info,
                                "Processing zero value.");
    } else {
      notificationCenter.notify(
          Notification::NotificationType::Info,
          "Processing positive value: " + std::to_string(value) + ".");
    }
  }

private:
  NotificationCenter &notificationCenter;
};

// --- Test ---
void test() {
  NotificationCenter nc;

  nc.registerGlobalListener([](const Notification &n) {
    std::println(">>> GLOBAL LOG: {}", n.toString());
  });

  nc.registerListener(Notification::NotificationType::Error,
                      [](const Notification &n) {
                        std::println("CRITICAL ERROR: {}", n.getMessage());
                      });

  nc.registerListener(Notification::NotificationType::Warning,
                      [](const Notification &n) {
                        std::println("WARNING: {}", n.getMessage());
                      });

  nc.registerListener(
      Notification::NotificationType::Success, [](const Notification &n) {
        std::println("SUCCESS! {} ({})", n.getMessage(), n.toString());
      });

  std::println();
  std::println("--- Starting Operations ---");

  FileProcessor fileProcessor(nc);
  DataAnalyzer dataAnalyzer(nc);

  fileProcessor.performFileOperation("report.txt", true);
  fileProcessor.performFileOperation("config.ini", false);
  dataAnalyzer.processData(100);
  dataAnalyzer.processData(-5);
  dataAnalyzer.processData(0);

  nc.notify(Notification::NotificationType::Info,
            "Application successfully initialized.");

  std::println();
  std::println("--- Operations Completed ---");
}

// --- Main ---
auto main() -> int {

  test();

  return 0;
}
