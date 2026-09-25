#include <cassert>
#include <print>
#include <span>

import db;

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

    std::println("smoke_test passed");
    return 0;
}
