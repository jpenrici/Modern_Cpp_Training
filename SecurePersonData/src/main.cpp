/*
 * Train Mutex
 *
 * References:
 *  https://en.cppreference.com/w/cpp/thread/mutex.html
 *
 */

#include <ctime>
#include <iomanip>
#include <mutex>
#include <print>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

class Person {

  struct PersonData {
    int id;
    std::string user;
    std::string password;
    int age;
  } data;

  std::chrono::time_point<std::chrono::system_clock> created;
  std::chrono::time_point<std::chrono::system_clock> modified;
  std::mutex personMutex;

  auto date_str(std::chrono::time_point<std::chrono::system_clock> time)
      -> std::string {
    auto c_time = std::chrono::system_clock::to_time_t(time);
    std::tm tm_buf;
#ifdef _MSV_VER // Visual Studio
    localtime_s(&tm_buf, &c_time);
#else // POSIX
    localtime_r(&c_time, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%d/%m/%Y %H:%M:%S");
    return ss.str();
  }

public:
  Person(int id, const std::string_view user, const std::string_view password,
         unsigned age)
      : data(id, std::string(user), std::string(password), age),
        created(std::chrono::system_clock::now()), modified(created) {}

  auto id() -> int {
    std::lock_guard<std::mutex> lock(personMutex);
    return data.id;
  }

  auto user() -> std::string {
    std::lock_guard<std::mutex> lock(personMutex);
    return data.user;
  }

  auto password() -> std::string {
    std::lock_guard<std::mutex> lock(personMutex);
    return data.password;
  }

  auto age() -> unsigned {
    std::lock_guard<std::mutex> lock(personMutex);
    return data.age;
  }

  void updatePassword(std::string_view newPassword) {
    if (newPassword.empty()) {
      std::println("Empty password! Changes not executed!");
      return;
    }
    std::lock_guard<std::mutex> lock(personMutex);
    data.password = newPassword;
    modified = std::chrono::system_clock::now();
    std::println("[Thread {}] {}, password changed successfully ...",
                 std::this_thread::get_id(), data.user);
  }

  void updateAge(int newAge) {
    if (newAge < 0) {
      std::println("Invalid age! Changes not executed!");
      return;
    }
    std::lock_guard<std::mutex> lock(personMutex);
    data.age = newAge;
    modified = std::chrono::system_clock::now();
    std::println("[Thread {}] {}, age changed successfully ...",
                 std::this_thread::get_id(), data.user);
  }

  void summary() {
    std::lock_guard<std::mutex> lock(personMutex);
    std::println();
    std::println("{}", std::string(80, '-'));
    std::println("Name: {}", data.user);
    std::println("Age : {}", data.age);
    std::println("Id  : {} [Created: {}, Last change: {}]", data.id,
                 date_str(created), date_str(modified));
    std::println("{}", std::string(80, '-'));
  }

  // Manual 'Traffic Light' Demonstration - Construction not recommended!
  void acquireAccess() {
    std::println("[Thread {}] Trying to acquire access to {} ...",
                 std::this_thread::get_id(), data.user);
    personMutex.lock();
    std::println("[Thread {}] Access acquired to {} ...",
                 std::this_thread::get_id(), data.user);
  }

  // Manual 'Traffic Light' Demonstration - Construction not recommended!
  void releaseAccess() {
    std::println("[Thread {}] Trying to acquire access to {} ...",
                 std::this_thread::get_id(), data.user);
    personMutex.unlock();
    std::println("[Thread {}] Access granted to {} ...",
                 std::this_thread::get_id(), data.user);
  }
};

void thread_task(Person &p, std::string_view newPassword, unsigned newAge) {
  auto ml = std::chrono::milliseconds(5);
  // Update
  p.updatePassword(newPassword);
  std::this_thread::sleep_for(ml); // It simulates a long work.
  p.acquireAccess();
  std::println(
      "[Thread {}] Performing critical operation with data from {} ...",
      std::this_thread::get_id(), p.user());
  std::this_thread::sleep_for(ml); // It simulates a long work.
  p.updateAge(newAge);
  p.releaseAccess();
}

auto main() -> int {

  // 1. Create an instance of the Person class
  Person person1(101, "Peter", "Password123", 23);
  person1.summary();

  // 2. Update
  std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait to update
  person1.updatePassword("Password321");
  person1.updateAge(28);
  person1.summary();

  // 3. Test concurrency
  // Using 'Semaphore' and threads is not recommended, it may cause 'deadlock'.
  // std::thread t1(thread_task, std::ref(person1), "thread_pass_1", 18);
  // t1.join();

  return 0;
}
