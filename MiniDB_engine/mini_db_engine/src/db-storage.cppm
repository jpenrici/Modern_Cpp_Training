module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory_resource>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// Partition :storage -- columnar, Data-Oriented storage backed by PMR.
//
// A Table never holds "rows" as objects. Each column is its own
// contiguous std::pmr::vector<T> (struct-of-arrays), and all columns of a
// table share one arena (a std::pmr::monotonic_buffer_resource) so that
// related data stays close together and grows in predictable steps.
//
// That arena itself allocates through a TrackingResource, a small custom
// std::pmr::memory_resource whose only job is to count bytes and calls --
// it exists purely so the behavior of PMR allocation is observable
// (see storage::stats), not because it changes how allocation works.
export module db:storage;

import :core;

export namespace db::storage {

// A std::pmr::memory_resource that forwards every allocation to an
// upstream resource while counting bytes and calls. This is the
// observability hook for the arena below: watch allocation_count() go up
// as a table grows past its initial buffer, and bytes_in_use() track the
// live footprint.
class TrackingResource final : public std::pmr::memory_resource {
public:
    explicit TrackingResource(std::pmr::memory_resource* upstream = std::pmr::get_default_resource()) noexcept
        : upstream_(upstream)
    {
    }

    [[nodiscard]] auto bytes_in_use() const noexcept -> std::size_t { return bytes_in_use_; }
    [[nodiscard]] auto bytes_allocated_total() const noexcept -> std::size_t { return bytes_allocated_total_; }
    [[nodiscard]] auto allocation_count() const noexcept -> std::size_t { return allocation_count_; }
    [[nodiscard]] auto deallocation_count() const noexcept -> std::size_t { return deallocation_count_; }

private:
    auto do_allocate(std::size_t bytes, std::size_t alignment) -> void* override
    {
        void* ptr = upstream_->allocate(bytes, alignment);
        bytes_in_use_ += bytes;
        bytes_allocated_total_ += bytes;
        ++allocation_count_;
        return ptr;
    }

    auto do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) -> void override
    {
        upstream_->deallocate(ptr, bytes, alignment);
        bytes_in_use_ -= bytes;
        ++deallocation_count_;
    }

    [[nodiscard]] auto do_is_equal(const std::pmr::memory_resource& other) const noexcept -> bool override
    {
        return this == &other;
    }

    std::pmr::memory_resource* upstream_;
    std::size_t bytes_in_use_ = 0;
    std::size_t bytes_allocated_total_ = 0;
    std::size_t allocation_count_ = 0;
    std::size_t deallocation_count_ = 0;
};

// Snapshot of a table's arena usage, read out of its TrackingResource.
struct MemoryStats {
    std::size_t bytes_in_use;
    std::size_t bytes_allocated_total;
    std::size_t allocation_count;
    std::size_t deallocation_count;
};

// Why insert_row() rejected a row.
enum class InsertError : std::uint8_t {
    ColumnCountMismatch,
    TypeMismatch,
};

// One column's storage, tagged by the scalar type it holds. Which
// alternative is active always matches the corresponding
// core::ColumnDescriptor::type in the table's schema.
using ColumnStorage = std::variant<
    std::pmr::vector<std::int64_t>,
    std::pmr::vector<double>,
    std::pmr::vector<std::pmr::string>>;

// A single table: a schema plus one SoA column per descriptor, all
// sharing one arena. Data members are public on purpose -- this follows
// the same "plain data + free functions" style as the rest of the engine,
// and lets callers inspect the arena directly if they want to.
struct Table {
    core::Schema schema;
    TrackingResource resource;
    std::pmr::monotonic_buffer_resource arena;
    std::pmr::vector<ColumnStorage> columns;

    // Builds one empty, correctly-typed column per schema entry, all
    // allocated from `arena`. `upstream` is where the arena itself grows
    // from when its initial buffer is exhausted.
    explicit Table(core::Schema table_schema,
        std::pmr::memory_resource* upstream = std::pmr::get_default_resource())
        : schema(std::move(table_schema))
        , resource(upstream)
        , arena(256, &resource)
        , columns(&arena)
    {
        columns.reserve(schema.size());
        for (const auto& descriptor : schema) {
            switch (descriptor.type) {
            case core::ColumnType::Int64:
                columns.emplace_back(std::in_place_type<std::pmr::vector<std::int64_t>>, &arena);
                break;
            case core::ColumnType::Double:
                columns.emplace_back(std::in_place_type<std::pmr::vector<double>>, &arena);
                break;
            case core::ColumnType::String:
                columns.emplace_back(std::in_place_type<std::pmr::vector<std::pmr::string>>, &arena);
                break;
            }
        }
    }
};

// Number of rows currently stored (all columns are always kept in sync,
// so any column's size works -- the first is used as a representative).
[[nodiscard]] auto row_count(const Table& table) noexcept -> std::size_t
{
    if (table.columns.empty()) {
        return 0;
    }
    return std::visit([](const auto& column) { return column.size(); }, table.columns.front());
}

// Appends one row, validating it against the schema before touching any
// column. Either every column gets its new value, or none do.
[[nodiscard]] auto insert_row(Table& table, std::span<const core::Value> row)
    -> std::expected<core::RowId, InsertError>
{
    if (row.size() != table.columns.size()) {
        return std::unexpected(InsertError::ColumnCountMismatch);
    }

    for (std::size_t i = 0; i < row.size(); ++i) {
        if (core::type_of(row[i]) != table.schema[i].type) {
            return std::unexpected(InsertError::TypeMismatch);
        }
    }

    const auto new_row_id = static_cast<core::RowId>(row_count(table));

    for (std::size_t i = 0; i < row.size(); ++i) {
        std::visit(
            [&table, i]<typename T>(const T& value) {
                std::get<std::pmr::vector<T>>(table.columns[i]).push_back(value);
            },
            row[i]);
    }

    return new_row_id;
}

// Reads a single cell back out as a Value, regardless of the column's
// underlying type. Out-of-range `row` throws (via std::pmr::vector::at),
// same as the standard library container it wraps.
[[nodiscard]] auto get_cell(const Table& table, core::RowId row, std::size_t column_index) -> core::Value
{
    return std::visit(
        [row]<typename T>(const std::pmr::vector<T>& column) -> core::Value { return column.at(row); },
        table.columns[column_index]);
}

// Snapshot of the table's arena usage, for observing PMR behavior.
[[nodiscard]] auto stats(const Table& table) noexcept -> MemoryStats
{
    return MemoryStats {
        .bytes_in_use = table.resource.bytes_in_use(),
        .bytes_allocated_total = table.resource.bytes_allocated_total(),
        .allocation_count = table.resource.allocation_count(),
        .deallocation_count = table.resource.deallocation_count(),
    };
}

} // namespace db::storage
