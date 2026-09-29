#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <string_view>
#include <syncstream>
#include <thread>
#include <vector>

// CAS (Compare-And-Swap) example

std::atomic<int> g_value { 100 };

void log(std::string_view thread_name, std::string_view message)
{
    std::osyncstream { std::cout } << std::format("[{}] {}\n", thread_name, message);
}

void task(int id)
{
    auto t_name = std::format("Worker-{}", id);
    int expected = g_value.load(std::memory_order_relaxed);
    int desired = expected + 50;

    log(t_name, std::format("Attempting to update value from expected: {}", expected));

    std::this_thread::sleep_for(std::chrono::milliseconds(10 * id));

    bool success = false;
    int attempts = 0;

    while (attempts < 5) {
        if (g_value.compare_exchange_strong(expected, desired,
                std::memory_order_acq_rel, std::memory_order_acquire)) {
            success = true;
            break;
        }
        // Em caso de falha, 'expected' já contém o valor atual
        log(t_name, std::format("CAS failed on attempt {}. "
                                "Current actual value is now: {}. "
                                "Retrying with new desired target.",
                        attempts + 1, expected));
        desired = expected + 50;
        ++attempts;
    }

    log(t_name, success ? "SUCCESS!" : "ABORTED!");
}

auto main() -> int
{
    std::osyncstream { std::cout } << std::format("Initial value: {}\n\n", g_value.load());

    constexpr int num_threads = 10;
    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    for (int i = 1; i <= num_threads; ++i) {
        threads.emplace_back(task, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::osyncstream { std::cout } << std::format("Final value: {}\n", g_value.load());
}

// g++ -std=c++23 main.cpp -o main -pthread -static-libstdc++ -static-libgcc
