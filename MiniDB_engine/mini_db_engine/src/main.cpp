#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <print>
#include <span>
#include <thread>
#include <variant>
#include <vector>

import db;

// A tiny Task<int> that hops onto the thread pool before computing its
// result -- everything after the co_await runs on a pool worker.
auto square_on(db::concurrency::ThreadPool& pool, int n) -> db::concurrency::Task<int>
{
    co_await db::concurrency::ScheduleOn { pool };
    co_return n* n;
}

auto main() -> int
{
    using namespace db::core;

    const db::engine::Database engine;
    std::println("mini_db_engine bootstrap OK: {}", engine.is_ready());

    // Exercise the :core partition: describe a small table shape and
    // inspect a couple of Values against it.
    const Schema schema {
        ColumnDescriptor { .name = "id", .type = ColumnType::Int64 },
        ColumnDescriptor { .name = "name", .type = ColumnType::String },
    };

    for (const auto& column : schema) {
        std::println("column '{}' -> {}", column.name, to_string(column.type));
    }

    const Value sample_id = std::int64_t { 42 };
    const Value sample_name = std::pmr::string { "alice" };
    std::println("type_of(sample_id)   = {}", to_string(type_of(sample_id)));
    std::println("type_of(sample_name) = {}", to_string(type_of(sample_name)));

    // Exercise the :storage partition: build a table from that same
    // schema, insert a couple of rows, and inspect both the data and the
    // arena's allocation behavior.
    using namespace db::storage;

    Table table(schema);

    const Value row0[] = { sample_id, sample_name };
    const Value row1[] = { Value { std::int64_t { 7 } }, Value { std::pmr::string { "bob" } } };

    for (const auto& row : { std::span<const Value> { row0 }, std::span<const Value> { row1 } }) {
        if (const auto inserted = insert_row(table, row); inserted) {
            std::println("inserted row {}", *inserted);
        } else {
            std::println("insert failed");
        }
    }

    std::println("row_count = {}", row_count(table));
    std::println("row 1, column 'name' = {}", std::get<std::pmr::string>(get_cell(table, 1, 1)));

    const auto memory_stats = stats(table);
    std::println("bytes_in_use = {}, allocation_count = {}",
        memory_stats.bytes_in_use, memory_stats.allocation_count);

    // Exercise the :exec partition: a few more rows so filtering has
    // something to do, then a lazy scan -> filter -> project pipeline.
    // Nothing here is materialized ahead of time -- each row is only
    // touched as the range-for below actually pulls it through.
    using namespace db::exec;

    const char* extra_names[] = { "carol", "dave", "erin", "frank" };
    for (int i = 0; i < 4; ++i) {
        const Value extra_row[] = { Value { std::int64_t { 10 + i } }, Value { std::pmr::string { extra_names[i] } } };
        (void)insert_row(table, extra_row);
    }
    std::println("row_count after extra inserts = {}", row_count(table));

    std::println("even-id rows (scan -> filter -> project):");
    auto even_ids = filter(scan(table), [&table](RowId row) {
        return std::get<std::int64_t>(get_cell(table, row, 0)) % 2 == 0;
    });
    for (const auto& values : project(std::move(even_ids), table, { 0, 1 })) {
        std::println("  id={}, name={}", std::get<std::int64_t>(values[0]),
            std::get<std::pmr::string>(values[1]));
    }

    // Exercise the :index partition: a lock-free (Value -> RowId) lookup.
    // First single-threaded, then under real concurrency, to actually
    // observe contention (retry_count) and the leak-by-design tradeoff
    // (leaked_snapshot_count) described in db-index.cppm.
    using namespace db::index;

    Index index;
    std::println("index.is_lock_free = {}", index.current.is_lock_free());

    upsert(index, sample_id, 0);
    upsert(index, Value { std::int64_t { 7 } }, 1);

    if (const auto found = lookup(index, sample_id)) {
        std::println("lookup(sample_id) -> row {}", *found);
    }

    constexpr int thread_count = 8;
    constexpr int keys_per_thread = 500;

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (int t = 0; t < thread_count; ++t) {
        workers.emplace_back([&index, t]() {
            for (int i = 0; i < keys_per_thread; ++i) {
                const auto key = static_cast<std::int64_t>(1'000 + t * keys_per_thread + i);
                upsert(index, Value { key }, static_cast<RowId>(key));
            }
        });
    }
    for (auto& worker : workers) {
        worker.join();
    }

    const auto index_stats = stats(index);
    std::println("index entry_count           = {}", index_stats.entry_count);
    std::println("index publish_count         = {}", index_stats.publish_count);
    std::println("index retry_count           = {}", index_stats.retry_count);
    std::println("index leaked_snapshot_count = {}", index_stats.leaked_snapshot_count);

    // Exercise the :wal partition: a lock-free MPSC ring buffer feeding a
    // coroutine flusher. Producers publish concurrently; the flusher
    // coroutine drains them, resumed directly by whichever producer
    // thread wakes it -- watch it hop threads over its lifetime.
    using namespace db::wal;

    Wal wal(64); // small on purpose: makes wraparound (and, under load,
    // full_rejection_count) actually observable.
    run_flusher(wal); // starts eagerly; parks immediately since wal is empty

    constexpr int wal_thread_count = 8;
    constexpr int records_per_thread = 2000;
    std::atomic<int> total_rejected { 0 };

    std::vector<std::thread> wal_producers;
    wal_producers.reserve(wal_thread_count);
    for (int t = 0; t < wal_thread_count; ++t) {
        wal_producers.emplace_back([&wal, &total_rejected, t]() {
            for (int i = 0; i < records_per_thread; ++i) {
                const auto row = static_cast<RowId>(t * records_per_thread + i);
                while (!try_append(wal, WalRecord { .row = row, .payload = Value { static_cast<std::int64_t>(row) } })) {
                    total_rejected.fetch_add(1, std::memory_order_relaxed);
                    std::this_thread::yield(); // buffer full; back off and retry
                }
            }
        });
    }
    for (auto& producer : wal_producers) {
        producer.join();
    }

    request_stop(wal);

    // request_stop() only guarantees the flusher wakes up and drains
    // what's already published -- it runs asynchronously (possibly on
    // whichever thread last touched it), so give it a moment to finish.
    while (stats(wal).durable_count < static_cast<std::size_t>(wal_thread_count * records_per_thread)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    const auto wal_stats = stats(wal);
    std::println("wal durable_count        = {}", wal_stats.durable_count);
    std::println("wal enqueue_retry_count  = {}", wal_stats.enqueue_retry_count);
    std::println("wal full_rejection_count = {}", wal_stats.full_rejection_count);

    // Exercise the :concurrency partition: a Task<T> that hops onto a
    // thread pool via ScheduleOn, and sync_wait() bridging back to this
    // ordinary (non-coroutine) main() function.
    using namespace db::concurrency;

    ThreadPool pool(4);

    const int squared = sync_wait(square_on(pool, 7));
    std::println("square_on(pool, 7) -> {}", squared);

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

    const auto pool_stats = stats(pool);
    std::println("pool tasks_processed     = {}", pool_stats.tasks_processed);
    std::println("pool enqueue_retry_count = {}", pool_stats.enqueue_retry_count);
    std::println("pool dequeue_retry_count = {}", pool_stats.dequeue_retry_count);
    std::println("pool mismatches          = {}", mismatches.load());

    return 0;
}
