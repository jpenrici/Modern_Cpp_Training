module;

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <semaphore>
#include <thread>
#include <utility>
#include <vector>

// Partition :concurrency -- a small fixed-size thread pool plus a Task<T>
// coroutine type that can hop onto it.
//
// The task queue itself is lock-free: the same Vyukov bounded-queue
// algorithm as :wal's ring buffer, but now genuinely MPMC (both ends use
// CAS), because here several worker threads dequeue concurrently --
// unlike :wal's single flusher, which only ever needed CAS on the
// producer side.
//
// Parking idle workers, though, uses std::counting_semaphore -- which is
// NOT lock-free; acquire() can put a thread to sleep via the OS. This is
// a deliberate, honest trade-off: a purely lock-free "wake me up" scheme
// (like :wal's single coroutine-handle exchange) doesn't generalize
// cleanly to multiple waiting consumers, and busy-spinning every idle
// worker forever would waste real CPU for no benefit in a teaching
// engine. Contrast this with :index and :wal, which stay lock-free
// end-to-end -- the difference is a deliberate design choice, not an
// oversight.
export module db:concurrency;

export namespace db::concurrency {

[[nodiscard]] constexpr auto next_power_of_two(std::size_t n) noexcept -> std::size_t
{
    std::size_t p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

// Bounded lock-free MPMC queue of coroutine handles. Deliberately not
// generic/templated on a payload type (unlike a general-purpose task
// queue) -- this partition only ever schedules coroutines, so it queues
// coroutine_handle<> directly rather than type-erasing through something
// like std::function.
struct TaskQueue {
    struct Cell {
        std::atomic<std::size_t> sequence;
        std::coroutine_handle<> handle;
    };

    std::vector<Cell> cells;
    std::size_t mask;
    alignas(64) std::atomic<std::size_t> enqueue_pos { 0 };
    alignas(64) std::atomic<std::size_t> dequeue_pos { 0 };
    std::atomic<std::size_t> enqueue_retry_count { 0 };
    std::atomic<std::size_t> dequeue_retry_count { 0 };

    explicit TaskQueue(std::size_t requested_capacity)
        : cells(next_power_of_two(requested_capacity))
        , mask(cells.size() - 1)
    {
        for (std::size_t i = 0; i < cells.size(); ++i) {
            cells[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
};

[[nodiscard]] auto try_push(TaskQueue& queue, std::coroutine_handle<> handle) -> bool
{
    std::size_t pos = queue.enqueue_pos.load(std::memory_order_relaxed);
    for (;;) {
        auto& cell = queue.cells[pos & queue.mask];
        const std::size_t seq = cell.sequence.load(std::memory_order_acquire);
        const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
        if (diff == 0) {
            if (queue.enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                cell.handle = handle;
                cell.sequence.store(pos + 1, std::memory_order_release);
                return true;
            }
            queue.enqueue_retry_count.fetch_add(1, std::memory_order_relaxed);
        } else if (diff < 0) {
            return false; // full
        } else {
            pos = queue.enqueue_pos.load(std::memory_order_relaxed);
        }
    }
}

// Unlike :wal's try_dequeue, this CASes dequeue_pos too: several worker
// threads may race to pop the same slot, so simple load/store isn't
// enough here.
[[nodiscard]] auto try_pop(TaskQueue& queue) -> std::coroutine_handle<>
{
    std::size_t pos = queue.dequeue_pos.load(std::memory_order_relaxed);
    for (;;) {
        auto& cell = queue.cells[pos & queue.mask];
        const std::size_t seq = cell.sequence.load(std::memory_order_acquire);
        const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
        if (diff == 0) {
            if (queue.dequeue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                auto handle = cell.handle;
                cell.sequence.store(pos + queue.cells.size(), std::memory_order_release);
                return handle;
            }
            queue.dequeue_retry_count.fetch_add(1, std::memory_order_relaxed);
        } else if (diff < 0) {
            return nullptr; // empty
        } else {
            pos = queue.dequeue_pos.load(std::memory_order_relaxed);
        }
    }
}

// Fixed-size worker pool. Deliberately not copyable or movable, same
// rationale as every other partition's core structure: the atomic and
// thread members can't sensibly be copied or relocated while workers
// hold references into it.
struct ThreadPool {
    TaskQueue queue;
    std::counting_semaphore<> available { 0 };
    std::atomic<bool> stop_requested { false };
    std::atomic<std::size_t> tasks_processed { 0 };
    std::vector<std::thread> workers;

    explicit ThreadPool(std::size_t worker_count, std::size_t queue_capacity = 1024)
        : queue(queue_capacity)
    {
        workers.reserve(worker_count);
        for (std::size_t i = 0; i < worker_count; ++i) {
            workers.emplace_back([this] { worker_loop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool()
    {
        stop_requested.store(true, std::memory_order_release);
        // Wake every worker at least once so each one gets a chance to
        // notice stop_requested even if it's currently parked. Workers
        // still drain whatever is left in the queue first (see
        // worker_loop) before actually honoring the stop request.
        for (std::size_t i = 0; i < workers.size(); ++i) {
            available.release();
        }
        for (auto& worker : workers) {
            worker.join();
        }
    }

private:
    void worker_loop()
    {
        for (;;) {
            if (auto handle = try_pop(queue)) {
                handle.resume();
                tasks_processed.fetch_add(1, std::memory_order_relaxed);
                continue; // keep draining without parking
            }
            if (stop_requested.load(std::memory_order_acquire)) {
                return;
            }
            available.acquire(); // park until schedule() or the destructor wakes us
        }
    }
};

// Pushes a handle onto the pool's queue and wakes a worker. Spins
// briefly on a full queue rather than blocking -- a "mini" pool's
// backpressure story; a production one might grow the queue instead.
auto schedule(ThreadPool& pool, std::coroutine_handle<> handle) -> void
{
    while (!try_push(pool.queue, handle)) {
        std::this_thread::yield();
    }
    pool.available.release();
}

// Awaiter that hands the rest of the calling coroutine off to the pool.
// `co_await ScheduleOn{pool};` as a coroutine's first statement means
// everything after that line runs on a pool worker thread instead of
// wherever the coroutine was originally called from.
struct ScheduleOn {
    ThreadPool& pool;

    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
    auto await_suspend(std::coroutine_handle<> handle) const -> void { schedule(pool, handle); }
    void await_resume() const noexcept { }
};

// A lazy, awaitable, single-result task. Doesn't run until first
// awaited (initial_suspend is suspend_always); on completion, resumes
// whichever coroutine awaited it via symmetric transfer (final_suspend
// hands control straight to promise.continuation instead of returning
// through an extra suspend/resume round trip).
template <typename T>
struct Task {
    struct promise_type {
        std::optional<T> result;
        std::exception_ptr exception;
        std::coroutine_handle<> continuation;

        [[nodiscard]] auto get_return_object() -> Task
        {
            return Task { std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        auto final_suspend() noexcept
        {
            struct Awaiter {
                [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
                auto await_suspend(std::coroutine_handle<promise_type> h) const noexcept -> std::coroutine_handle<>
                {
                    auto continuation = h.promise().continuation;
                    return continuation ? continuation : std::noop_coroutine();
                }
                void await_resume() const noexcept { }
            };
            return Awaiter {};
        }
        void return_value(T value) { result = std::move(value); }
        void unhandled_exception() { exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> handle;

    explicit Task(std::coroutine_handle<promise_type> h)
        : handle(h)
    {
    }
    ~Task()
    {
        if (handle) {
            handle.destroy();
        }
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    Task(Task&& other) noexcept
        : handle(std::exchange(other.handle, {}))
    {
    }
    Task& operator=(Task&&) = delete;

    // Lets a Task<T> itself be `co_await`ed by another coroutine.
    [[nodiscard]] auto await_ready() const noexcept -> bool { return false; }
    [[nodiscard]] auto await_suspend(std::coroutine_handle<> awaiting) noexcept -> std::coroutine_handle<>
    {
        handle.promise().continuation = awaiting;
        return handle; // symmetric transfer straight into this task's body
    }
    [[nodiscard]] auto await_resume() -> T
    {
        if (handle.promise().exception) {
            std::rethrow_exception(handle.promise().exception);
        }
        return std::move(*handle.promise().result);
    }
};

// Minimal eager, self-cleaning "fire and forget" coroutine, same shape
// as :wal's FlushTask -- used only internally by sync_wait() below to
// drive a Task<T> to completion and signal when it's done.
struct Detached {
    struct promise_type {
        [[nodiscard]] auto get_return_object() noexcept -> Detached { return {}; }
        [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        [[nodiscard]] auto final_suspend() noexcept -> std::suspend_never { return {}; }
        void return_void() noexcept { }
        void unhandled_exception() { std::terminate(); }
    };
};

template <typename T>
auto drive_to_completion(Task<T> task, std::binary_semaphore& done, T& out) -> Detached
{
    out = co_await std::move(task);
    done.release();
}

// Bridges ordinary (non-coroutine) code -- like main() -- to a Task<T>:
// blocks the calling thread until the task completes, however many
// thread hops it takes to get there, and returns its result. Requires T
// to be default-constructible, a minor simplification acceptable for a
// teaching engine's bridging helper.
template <typename T>
[[nodiscard]] auto sync_wait(Task<T> task) -> T
{
    std::binary_semaphore done { 0 };
    T out {};
    drive_to_completion(std::move(task), done, out);
    done.acquire();
    return out;
}

struct PoolStats {
    std::size_t tasks_processed;
    std::size_t enqueue_retry_count;
    std::size_t dequeue_retry_count;
};

[[nodiscard]] auto stats(const ThreadPool& pool) -> PoolStats
{
    return PoolStats {
        .tasks_processed = pool.tasks_processed.load(std::memory_order_relaxed),
        .enqueue_retry_count = pool.queue.enqueue_retry_count.load(std::memory_order_relaxed),
        .dequeue_retry_count = pool.queue.dequeue_retry_count.load(std::memory_order_relaxed),
    };
}

} // namespace db::concurrency
