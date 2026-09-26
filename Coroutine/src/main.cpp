#include <chrono>
#include <coroutine>
#include <cstdlib>
#include <print>
#include <thread>

struct Task {
    struct promise_type {
        Task get_return_object()
        {
            return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { std::exit(1); }
    };

    std::coroutine_handle<promise_type> handle;

    ~Task()
    {
        if (handle) {
            handle.destroy();
        }
    }

    bool step()
    {
        if (!handle.done()) {
            handle.resume();
        }
        return !handle.done();
    }
};

struct WaitForFrame {
    int frames_left;

    bool await_ready() const noexcept { return frames_left <= 0; }
    void await_suspend(std::coroutine_handle<>) noexcept { }
    void await_resume() noexcept { }
};

auto npc_behavior(const std::string_view npc_name) -> Task
{
    std::println("[{}] State: Idle (Waiting for player...)", npc_name);
    co_await WaitForFrame { 3 };

    std::println("[{}] State: Walking towards the target position...", npc_name);
    co_await WaitForFrame { 5 };

    std::println("[{}] State: ATTACK! Dealing damage.", npc_name);
    co_await WaitForFrame { 2 };

    std::println("[{}] State: Returning to patrol.", npc_name);
}

// Simulates Game Loop Main
auto main() -> int
{
    std::println("=== Game Engine Start ===");

    // Instantiates the behavior of our NPC
    Task npc = npc_behavior("Globin_Warrior");

    int frame_count = 1;

    // Simulates the game loop running frame by frame
    while (npc.step()) {
        std::println("[Engine] Frame {} rendered", frame_count++);

        // Brief pause to simulate frame rate (e.g. ~16ms per frame)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return EXIT_SUCCESS;
}
