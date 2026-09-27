module;

#include <string>
#include <vector>

export module db:exec;

import :storage;

export namespace db::exec {

auto insert_into(storage::Table& table, std::vector<std::string> values) -> int
{
    return table.insert(std::move(values));
}

[[nodiscard]] auto select_all(storage::Table const& table) -> std::vector<storage::Row>
{
    return table.all();
}

template <typename Predicate>
[[nodiscard]] auto select_where(storage::Table const& table, Predicate&& predicate)
    -> std::vector<storage::Row>
{
    std::vector<storage::Row> result;
    for (auto const& row : table.all()) {
        if (predicate(row)) {
            result.push_back(row);
        }
    }
    return result;
}

auto delete_by_id(storage::Table& table, int id) -> bool
{
    return table.remove(id);
}

[[nodiscard]] auto is_ready() noexcept -> bool { return true; }

} // namespace db::exec
