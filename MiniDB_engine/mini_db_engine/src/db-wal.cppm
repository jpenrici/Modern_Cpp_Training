module;

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <optional>
#include <utility>
#include <vector>

// Partition :wal -- a lock-free MPSC (multiple-producer, single-consumer)
// ring buffer feeding a coroutine "flusher". This is where :index's
// lock-free technique and coroutines meet: producers publish records with
// no locking, and the single consumer is a suspended coroutine that gets
// resumed directly by whichever producer thread happens to wake it --
// genuinely hopping between OS threads over its lifetime.
//
// The ring buffer itself is Dmitry Vyukov's bounded MPMC queue algorithm,
// used here with a single consumer (only the flusher ever dequeues).
// Capacity is rounded up to the next power of two so slot indexing is a
// bitmask instead of a modulo.
export module db:wal;

import :core;

export namespace db::wal {

// One logged operation. Deliberately generic (not tied to a specific
// table) -- this partition only cares about durably recording a
// (row, payload) pair in order, not about what storage did with it.
struct WalRecord {
    core::RowId row;
    core::Value payload;
};

[[nodiscard]] constexpr auto next_power_of_two(std::size_t n) noexcept -> std::size_t
{
    std::size_t p = 1;
    while (p < n) {
        p <<= 1;
    }
    return p;
}

// Deliberately not copyable or movable, same rationale as storage::Table
// and index::Index: the atomic members can't be copied, and nothing
// outside this partition should be juggling RingBuffer by value while
// producers and the flusher hold references into it.
struct RingBuffer {
    struct Cell {
        std::atomic<std::size_t> sequence;
        WalRecord data;
    };

    std::vector<Cell> cells;
    std::size_t mask;
    alignas(64) std::atomic<std::size_t> enqueue_pos { 0 };
    alignas(64) std::atomic<std::size_t> dequeue_pos { 0 };
    std::atomic<std::size_t> enqueue_retry_count { 0 };
    std::atomic<std::size_t> full_rejection_count { 0 };
    // The flusher coroutine parks its handle here while waiting for
    // data. A producer that successfully enqueues claims it (atomic
    // exchange, so exactly one producer ever wins a given handle) and
    // resumes it directly -- no OS-level condition variable involved.
    std::atomic<std::coroutine_handle<>> waiter { nullptr };

    explicit RingBuffer(std::size_t requested_capacity)
        : cells(next_power_of_two(requested_capacity))
        , mask(cells.size() - 1)
    {
        for (std::size_t i = 0; i < cells.size(); ++i) {
            cells[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;
};

// Cheap, approximate "is there probably something to read" check. Used
// only as a heuristic (to skip suspension, or to decide whether to
// re-claim a just-published handle) -- try_dequeue() is the actual
// source of truth.
[[nodiscard]] auto approx_empty(const RingBuffer& buffer) noexcept -> bool
{
    return buffer.dequeue_pos.load(std::memory_order_relaxed) == buffer.enqueue_pos.load(std::memory_order_relaxed);
}

// Resumes whichever coroutine is parked waiting for data, if any. The
// atomic exchange guarantees that if several producers race here,
// exactly one of them wins and resumes the coroutine -- resuming the
// same handle twice is undefined behavior, so this must stay exact.
auto wake_waiter(RingBuffer& buffer) -> void
{
    if (auto handle = buffer.waiter.exchange(nullptr, std::memory_order_acq_rel)) {
        handle.resume();
    }
}

// Lock-free multi-producer enqueue (Vyukov's algorithm). Returns false
// if the buffer is full rather than blocking -- callers decide whether
// to retry, drop, or back off.
[[nodiscard]] auto try_enqueue(RingBuffer& buffer, WalRecord record) -> bool
{
    std::size_t pos = buffer.enqueue_pos.load(std::memory_order_relaxed);
    for (;;) {
        auto& cell = buffer.cells[pos & buffer.mask];
        const std::size_t seq = cell.sequence.load(std::memory_order_acquire);
        const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
        if (diff == 0) {
            if (buffer.enqueue_pos.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                cell.data = std::move(record);
                cell.sequence.store(pos + 1, std::memory_order_release);
                wake_waiter(buffer);
                return true;
            }
            buffer.enqueue_retry_count.fetch_add(1, std::memory_order_relaxed);
        } else if (diff < 0) {
            buffer.full_rejection_count.fetch_add(1, std::memory_order_relaxed);
            return false; // buffer full
        } else {
            pos = buffer.enqueue_pos.load(std::memory_order_relaxed);
        }
    }
}

// Single-consumer dequeue: only ever safe to call from the flusher
// coroutine, so dequeue_pos needs no CAS -- ordinary atomic load/store
// is enough since no other thread ever touches it.
[[nodiscard]] auto try_dequeue(RingBuffer& buffer) -> std::optional<WalRecord>
{
    const std::size_t pos = buffer.dequeue_pos.load(std::memory_order_relaxed);
    auto& cell = buffer.cells[pos & buffer.mask];
    const std::size_t seq = cell.sequence.load(std::memory_order_acquire);
    const auto diff = static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos + 1);
    if (diff == 0) {
        WalRecord record = std::move(cell.data);
        cell.sequence.store(pos + buffer.cells.size(), std::memory_order_release);
        buffer.dequeue_pos.store(pos + 1, std::memory_order_relaxed);
        return record;
    }
    return std::nullopt; // empty
}

// Awaitable that suspends the flusher until the ring buffer looks
// non-empty. Closes the classic coroutine "lost wakeup" race: data can
// arrive in the gap between await_ready()'s optimistic check and the
// point where the waiter handle actually becomes visible to producers,
// so await_suspend re-checks right after publishing and, if it lost
// that race against a producer, reclaims the handle itself instead of
// suspending forever.
struct BufferHasData {
    RingBuffer& buffer;

    [[nodiscard]] auto await_ready() const noexcept -> bool { return !approx_empty(buffer); }

    [[nodiscard]] auto await_suspend(std::coroutine_handle<> handle) noexcept -> bool
    {
        // Capture into a LOCAL reference -- on this thread's real stack,
        // not the coroutine frame -- before publishing the handle. The
        // instant that store below becomes visible, a producer may
        // resume the coroutine on another thread; if that resumption
        // runs the coroutine to completion, its frame is freed right
        // then (final_suspend is suspend_never). This object (`this`,
        // and its `buffer` member) lives inside that frame, so touching
        // `this->buffer` again after publishing would race with (or
        // read freed memory from) that concurrent resumption. `buf`
        // sidesteps this: it's an ordinary stack local.
        RingBuffer& buf = buffer;

        buf.waiter.store(handle, std::memory_order_release);
        if (!approx_empty(buf)) {
            auto expected = handle;
            if (buf.waiter.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel)) {
                return false; // reclaimed it ourselves: resume immediately, no thread hop
            }
            // Otherwise a producer already claimed the handle (and may
            // already be resuming it on another thread right now) --
            // don't touch anything else here.
        }
        return true; // stay suspended; some producer's wake_waiter() will resume us
    }

    void await_resume() const noexcept { }
};

// An eager, self-driving, "fire and forget" coroutine task. Runs
// synchronously up to its first suspension point as soon as it's called;
// from then on it resumes only when woken, possibly on a different
// thread each time, until it decides to stop. final_suspend is
// suspend_never, so the frame cleans itself up when the coroutine body
// returns -- nothing external needs to hold or destroy a handle.
struct FlushTask {
    struct promise_type {
        [[nodiscard]] auto get_return_object() noexcept -> FlushTask { return {}; }
        [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_never { return {}; }
        [[nodiscard]] auto final_suspend() noexcept -> std::suspend_never { return {}; }
        void return_void() noexcept { }
        void unhandled_exception() { std::terminate(); }
    };
};

// A write-ahead log: the ring buffer producers publish into, plus the
// "durable" sink the flusher drains into (in-memory, for this teaching
// engine -- a real WAL would fsync a file here) and a stop flag.
struct Wal {
    RingBuffer buffer;
    std::atomic<bool> stop_requested { false };
    std::pmr::vector<WalRecord> durable_log;

    explicit Wal(std::size_t capacity, std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
        : buffer(capacity)
        , durable_log(upstream)
    {
    }
};

[[nodiscard]] auto try_append(Wal& wal, WalRecord record) -> bool
{
    return try_enqueue(wal.buffer, std::move(record));
}

// Asks the flusher to stop and wakes it if it's currently parked waiting
// for data -- otherwise it would never notice stop_requested until more
// data arrived (or never, at program shutdown).
auto request_stop(Wal& wal) -> void
{
    wal.stop_requested.store(true, std::memory_order_release);
    wake_waiter(wal.buffer);
}

// The flusher itself. Call once to start it; it keeps itself alive and
// resumes itself via producers' wake_waiter() calls until stopped.
auto run_flusher(Wal& wal) -> FlushTask
{
    while (!wal.stop_requested.load(std::memory_order_acquire)) {
        co_await BufferHasData { wal.buffer };
        while (auto record = try_dequeue(wal.buffer)) {
            wal.durable_log.push_back(std::move(*record));
        }
    }
    // Stop was requested -- drain whatever is still sitting in the
    // buffer one last time before actually finishing.
    while (auto record = try_dequeue(wal.buffer)) {
        wal.durable_log.push_back(std::move(*record));
    }
}

// Snapshot of the WAL's throughput and contention behavior, for
// observing it under load.
struct WalStats {
    std::size_t durable_count;
    std::size_t enqueue_retry_count;
    std::size_t full_rejection_count;
};

[[nodiscard]] auto stats(const Wal& wal) -> WalStats
{
    return WalStats {
        .durable_count = wal.durable_log.size(),
        .enqueue_retry_count = wal.buffer.enqueue_retry_count.load(std::memory_order_relaxed),
        .full_rejection_count = wal.buffer.full_rejection_count.load(std::memory_order_relaxed),
    };
}

} // namespace db::wal
