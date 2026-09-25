module;

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

// Partition :core -- fundamental vocabulary shared by every other
// partition: row identifiers, the closed set of scalar column types, a
// single-cell Value variant used at API boundaries, and the Schema that
// describes a table's shape.
//
// Nothing here is Data-Oriented in itself (there is no bulk row storage
// in this partition -- that belongs to :storage). This is deliberately
// just the small, shared vocabulary every other partition builds on.
export module db:core;

export namespace db::core {

// A stable, dense row identifier. Rows are addressed by position, not by
// pointer -- consistent with a Data-Oriented Design where "the row" is
// just an index into a set of parallel column arrays (see :storage).
using RowId = std::size_t;

// Sentinel meaning "no row" / "not found".
inline constexpr RowId npos = static_cast<RowId>(-1);

// The set of scalar types a column can hold. Kept intentionally small and
// closed -- this is a teaching engine, not a general-purpose type system.
enum class ColumnType : std::uint8_t {
    Int64,
    Double,
    String,
};

// Human-readable name for a ColumnType, mainly for logging/debugging.
[[nodiscard]] constexpr auto to_string(ColumnType type) noexcept -> std::string_view
{
    switch (type) {
    case ColumnType::Int64:
        return "Int64";
    case ColumnType::Double:
        return "Double";
    case ColumnType::String:
        return "String";
    }
    return "Unknown";
}

// A single cell value, used at the API boundary (e.g. inserting a row or
// reading one back). Internally, :storage never keeps rows as vectors of
// Value -- each column is its own contiguous std::pmr::vector<T>. Value
// only exists where heterogeneous, single-cell data needs to cross a
// function boundary (insert() parameters, get() results, ...).
using Value = std::variant<std::int64_t, double, std::pmr::string>;

// Maps a Value's active alternative to its ColumnType, so :storage can
// validate that an inserted Value matches the column it's being written
// into, without either side depending on the other's internals.
[[nodiscard]] constexpr auto type_of(const Value& value) noexcept -> ColumnType
{
    return std::visit(
        []<typename T>(const T&) -> ColumnType {
            if constexpr (std::same_as<T, std::int64_t>) {
                return ColumnType::Int64;
            } else if constexpr (std::same_as<T, double>) {
                return ColumnType::Double;
            } else {
                return ColumnType::String;
            }
        },
        value);
}

// Describes one column of a table: its name and scalar type.
struct ColumnDescriptor {
    std::pmr::string name;
    ColumnType type;
};

// A table's shape: an ordered list of column descriptors. Row layout is
// implied by this order -- there is no per-row metadata beyond RowId.
using Schema = std::pmr::vector<ColumnDescriptor>;

} // namespace db::core
