#include <iostream>
#include <syncstream>
#include <thread>

auto main() -> int {

  auto worker = [](int id, std::string_view message = "No message.") {
    std::osyncstream sync_out(std::cout);
    sync_out << "Thread [" << id << "] " << message << "\n";
  };

  std::thread t1(worker, 1, "This thread has finished writing message 1.");
  std::thread t2(worker, 2, "This thread has finished writing message 2.");
  std::thread t3(worker, 3);

  t1.join();
  t2.join();
  t3.join();

  return 0;
}
