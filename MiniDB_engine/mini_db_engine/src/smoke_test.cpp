#include <atomic>
#include <cassert>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <optional>
#include <print>
#include <span>
#include <thread>
#include <variant>
#include <vector>

import db;

auto square_on(db::concurrency::ThreadPool& pool, int n) -> db::concurrency::Task<int>
{
    co_await db::concurrency::ScheduleOn { pool };
    co_return n* n;
}

// Smoke test: checks that the module graph links, the facade type is
// constructible, :core's vocabulary behaves as expected, and :storage's
// Table correctly inserts, reads back, and rejects malformed rows. Still
// not a real behavioral test suite -- that comes once :index/:wal/:exec
// have actual logic to test.
auto main() -> int
{
    using namespace db::core;
    using namespace db::storage;

    const db::engine::Database engine;
    assert(engine.is_ready());

    const Schema schema {
        ColumnDescriptor { .name = "id", .type = ColumnType::Int64 },
        ColumnDescriptor { .name = "name", .type = ColumnType::String },
    };
    assert(schema.size() == 2);
    assert(schema.front().type == ColumnType::Int64);

    assert(type_of(Value { std::int64_t { 1 } }) == ColumnType::Int64);
    assert(type_of(Value { 3.14 }) == ColumnType::Double);
    assert(type_of(Value { std::pmr::string { "x" } }) == ColumnType::String);

    // :storage -- happy path
    Table table(schema);

    const Value row0[] = { Value { std::int64_t { 1 } }, Value { std::pmr::string { "alice" } } };
    const Value row1[] = { Value { std::int64_t { 2 } }, Value { std::pmr::string { "bob" } } };

    const auto id0 = insert_row(table, row0);
    const auto id1 = insert_row(table, row1);
    assert(id0.has_value() && *id0 == 0);
    assert(id1.has_value() && *id1 == 1);

    assert(row_count(table) == 2);
    assert(std::get<std::pmr::string>(get_cell(table, 0, 1)) == "alice");
    assert(std::get<std::int64_t>(get_cell(table, 1, 0)) == 2);

    const auto memory_stats = stats(table);
    assert(memory_stats.bytes_in_use > 0);
    assert(memory_stats.deallocation_count == 0); // monotonic: no mid-flight frees

    // :storage -- rejected rows
    const Value wrong_arity[] = { Value { std::int64_t { 3 } } };
    const auto arity_error = insert_row(table, wrong_arity);
    assert(!arity_error.has_value());
    assert(arity_error.error() == InsertError::ColumnCountMismatch);

    const Value wrong_type[] = { Value { std::pmr::string { "nope" } }, Value { std::pmr::string { "x" } } };
    const auto type_error = insert_row(table, wrong_type);
    assert(!type_error.has_value());
    assert(type_error.error() == InsertError::TypeMismatch);

    assert(row_count(table) == 2); // rejected rows must not have partially inserted

    // :index -- single-threaded correctness
    using namespace db::index;

    Index index;
    assert(index.current.is_lock_free()); // provably lock-free on this platform, not assumed

    upsert(index, Value { std::int64_t { 10 } }, 0);
    upsert(index, Value { std::int64_t { 5 } }, 1);
    upsert(index, Value { std::int64_t { 10 } }, 2); // update, not a new entry

    assert(lookup(index, Value { std::int64_t { 10 } }) == std::optional<RowId> { 2 });
    assert(lookup(index, Value { std::int64_t { 5 } }) == std::optional<RowId> { 1 });
    assert(lookup(index, Value { std::int64_t { 99 } }) == std::nullopt);
    assert(stats(index).entry_count == 2);

    // :index -- concurrent stress test. Threads use disjoint key ranges,
    // so the final entry_count is fully deterministic even though the
    // CAS loop genuinely races internally (see retry_count below).
    constexpr int thread_count = 8;
    constexpr int keys_per_thread = 500;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (int t = 0; t < thread_count; ++t) {
        workers.emplace_back([&index, t]() {
            for (int i = 0; i < keys_per_thread; ++i) {
                const auto key = static_cast<std::int64_t>(t * keys_per_thread + i);
                upsert(index, Value { key }, static_cast<RowId>(key));
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    // Keys 5 and 10 fall inside thread 0's range (0..499) and get
    // overwritten there, not added -- so the total is exactly
    // thread_count * keys_per_thread, not that plus 2.
    const auto index_stats = stats(index);
    assert(index_stats.entry_count == thread_count * keys_per_thread);
    assert(index_stats.publish_count == index_stats.leaked_snapshot_count);

    for (int t = 0; t < thread_count; ++t) {
        for (int i = 0; i < keys_per_thread; ++i) {
            const auto key = static_cast<std::int64_t>(t * keys_per_thread + i);
            assert(lookup(index, Value { key }) == std::optional<RowId> { static_cast<RowId>(key) });
        }
    }

    // :wal -- lock-free MPSC ring buffer + coroutine flusher, under real
    // concurrency. This exercises the same producer/consumer pattern as
    // :index's stress test, but with the flusher coroutine as the single
    // consumer instead of a CAS loop.
    using namespace db::wal;

    Wal wal(64); // small on purpose: forces wraparound.
    run_flusher(wal);

    constexpr int wal_thread_count = 8;
    constexpr int records_per_thread = 2000;

    std::vector<std::thread> wal_producers;
    wal_producers.reserve(wal_thread_count);
    for (int t = 0; t < wal_thread_count; ++t) {
        wal_producers.emplace_back([&wal, t]() {
            for (int i = 0; i < records_per_thread; ++i) {
                const auto row = static_cast<RowId>(t * records_per_thread + i);
                while (!try_append(wal, WalRecord { .row = row, .payload = Value { static_cast<std::int64_t>(row) } })) {
                    std::this_thread::yield(); // buffer full; back off and retry
                }
            }
        });
    }
    for (auto& producer : wal_producers) {
        producer.join();
    }

    request_stop(wal);

    // Asynchronous drain: poll until the flusher (running on whichever
    // thread last touched it) has caught up, instead of assuming
    // request_stop() itself waits for that.
    constexpr auto expected_durable_count = static_cast<std::size_t>(wal_thread_count * records_per_thread);
    for (int i = 0; i < 2000 && stats(wal).durable_count < expected_durable_count; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const auto wal_stats = stats(wal);
    assert(wal_stats.durable_count == expected_durable_count);

    // :concurrency -- Task<T> hopping onto a thread pool, driven via
    // sync_wait from this ordinary (non-coroutine) function, under real
    // concurrent load from multiple calling threads.
    using namespace db::concurrency;

    ThreadPool pool(4);

    assert(sync_wait(square_on(pool, 7)) == 49);

    constexpr int concurrency_thread_count = 8;
    constexpr int tasks_per_thread = 300;
    std::atomic<int> mismatches { 0 };

    std::vector<std::thread> callers;
    callers.reserve(concurrency_thread_count);
    for (int t = 0; t < concurrency_thread_count; ++t) {
        callers.emplace_back([&pool, &mismatches, t]() {
            for (int i = 0; i < tasks_per_thread; ++i) {
                const int n = t * tasks_per_thread + i;
                if (sync_wait(square_on(pool, n)) != n * n) {
                    mismatches.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& caller : callers) {
        caller.join();
    }

    assert(mismatches.load() == 0);
    assert(stats(pool).tasks_processed == static_cast<std::size_t>(1 + concurrency_thread_count * tasks_per_thread));

    std::println("smoke_test passed");
    return 0;
}
