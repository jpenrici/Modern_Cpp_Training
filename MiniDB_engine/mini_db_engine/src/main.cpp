#include <print>
#include <span>
#include <thread>
#include <variant>
#include <vector>

import db;

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

    return 0;
}
