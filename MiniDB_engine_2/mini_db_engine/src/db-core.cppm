module;

#include <cstddef>
#include <cstdint>
#include <memory_resource>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

export module db:core;

export namespace db::core {

// The row is just an index into a set of parallel column arrays (db:storage).
using RowId = std::size_t;

// Sentinel meaning "no row" / "not fount".
inline constexpr RowId npos = static_cast<RowId>(-1);

// Scalar types.
enum class ColumnType : std::uint8_t {
    Int64,
    Double,
    String,
};

// Human-readable name for a ColumnType.
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
    return "Unknow";
}

// Single cell value.
using Value = std::variant<std::int64_t, double, std::pmr::string>;

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

// A table's shape: an ordered list of column descriptors.
using Schema = std::pmr::vector<ColumnDescriptor>;

} // namespace db::core

/*
 * std::pmr (Polymorphic Memory Resources), introduced in C++17 under the <memory_resource> header,
 * is a standard library feature that separates container logic from how and where memory is allocated
 * by using runtime polymorphism.
 *
 * see more: https://en.cppreference.com/cpp/memory/polymorphic_allocator
 */
