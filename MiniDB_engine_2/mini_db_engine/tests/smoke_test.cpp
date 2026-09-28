#include <atomic>
#include <cassert>
#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <optional>
#include <print>
#include <span>
#include <thread>
#include <variant>
#include <vector>

import db;

auto main() -> int
{
    using namespace db::core;
    using namespace db::storage;

    // db::core
    // enum class ColumnType : std::uint8_t { Int64, Double, String };
    // struct ColumnDescriptor { std::pmr::string name; ColumnType type; };
    // using Schema = std::pmr::vector<ColumnDescriptor>;
    // using Value = std::variant<std::int64_t, double, std::pmr::string>;

    const Schema schema {
        ColumnDescriptor { .name = "id", .type = ColumnType::Int64 },
        ColumnDescriptor { .name = "name", .type = ColumnType::String },
    };

    assert(schema.size() == 2);
    assert(schema.front().type == ColumnType::Int64); // "id"

    assert(type_of(Value { std::int64_t { 1 } }) == ColumnType::Int64);
    assert(type_of(Value { 3.14 }) == ColumnType::Double);
    assert(type_of(Value { std::pmr::string { "x" } }) == ColumnType::String);

    // db::storage
    // struct Table {
    //  core::Schema schema;
    //  TrackingResource resource;
    //  std::pmr::monotonic_buffer_resource arena;
    //  std::pmr::vector<ColumnStorage> columns;
    //
    //  explicit Table(core::Schema table_schema, std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
    //  : schema(std::move(table_schema)), resource(upstream), arena(256, &resource), columns(&arena) {...}
    Table table(schema);

    // "id" and "name"
    const Value row0[] = { Value { std::int64_t { 1 } }, Value { std::pmr::string { "alice" } } };
    const Value row1[] = { Value { std::int64_t { 2 } }, Value { std::pmr::string { "bob" } } };

    // Insert rows
    const auto id0 = insert_row(table, row0);
    assert(id0.has_value() && *id0 == 0);

    const auto id1 = insert_row(table, row1);
    assert(id1.has_value() && *id1 == 1);

    assert(row_count(table) == 2);
    assert(std::get<std::pmr::string>(get_cell(table, 0, 1)) == "alice");
    assert(std::get<std::int64_t>(get_cell(table, 1, 0)) == 2);

    // struct MemoryStats { std::size_t bytes_in_use; std::size_t bytes_allocated_total;
    //                      std::size_t allocation_count; std::size_t deallocation_count; };
    const auto memory_stats = stats(table);
    assert(memory_stats.bytes_in_use > 0);
    assert(memory_stats.deallocation_count == 0); // monotonic: no mid-flight frees

    // Rejected rows
    const Value wrong_arity[] = { Value { std::int64_t { 3 } } };
    const auto arity_error = insert_row(table, wrong_arity);
    assert(!arity_error.has_value());
    assert(arity_error.error() == InsertError::ColumnCountMismatch);

    const Value wrong_type[] = { Value { std::pmr::string { "nope" } }, Value { std::pmr::string { "x" } } };
    const auto type_error = insert_row(table, wrong_type);
    assert(!type_error.has_value());
    assert(type_error.error() == InsertError::TypeMismatch);

    assert(row_count(table) == 2); // rejected rows must not have partially inserted

    return 0;
}
