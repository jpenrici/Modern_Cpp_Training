module;

#include <algorithm>
#include <string>
#include <vector>

export module db:storage;

export namespace db::storage {

struct Row {
    int id; // auto-increment
    std::vector<std::string> columns;
};

class Table {
public:
    explicit Table(std::vector<std::string> column_names)
        : column_names_ { std::move(column_names) }
    {
    }

    auto insert(std::vector<std::string> values) -> int
    {
        int const id = next_id_++;
        rows_.push_back(Row { id, std::move(values) });
        return id;
    }

    [[nodiscard]] auto find(int id) const -> Row const*
    {
        auto const it = std::ranges::find_if(rows_, [id](Row const& row) { return row.id == id; });
        return it != rows_.end() ? &*it : nullptr;
    }

    auto remove(int id) -> bool
    {
        auto const it = std::ranges::find_if(rows_, [id](Row const& row) { return row.id == id; });
        if (it == rows_.end()) {
            return false;
        }
        rows_.erase(it);
        return true;
    }

    [[nodiscard]] auto all() const -> std::vector<Row> const& { return rows_; }
    [[nodiscard]] auto column_names() const -> std::vector<std::string> const& { return column_names_; }

private:
    std::vector<std::string> column_names_;
    std::vector<Row> rows_;
    int next_id_ = 1;
};

[[nodiscard]] auto is_ready() noexcept -> bool { return true; }

} // namespace db::storage
