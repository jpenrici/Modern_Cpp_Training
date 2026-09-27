#include <print>

import db;

auto main() -> int
{
    std::println("module db::exec {}", db::exec::is_ready());
    std::println("module db::storage {}", db::storage::is_ready());

    db::storage::Table users({ "name", "email" });

    auto const ana_id = db::exec::insert_into(users, { "Ana", "ana26@example.com" });
    db::exec::insert_into(users, { "Bruno", "bruno2026@example.com" });
    db::exec::insert_into(users, { "Carla", "carla_26@example.com" });

    std::println("\n-- All users --");
    for (auto const& row : db::exec::select_all(users)) {
        std::println("id={} name={} email={}", row.id, row.columns[0], row.columns[1]);
    }

    std::println("\n-- Filtering by name starting with 'B' --");
    for (auto const& row : db::exec::select_where(users, [](auto const& row) {
             return row.columns[0].starts_with("B");
         })) {
        std::println("id={} name={}", row.id, row.columns[0]);
    }

    std::println("\n-- Removing id={} (Ana) --", ana_id);
    db::exec::delete_by_id(users, ana_id);

    std::println("\n-- Final state --");
    for (auto const& row : db::exec::select_all(users)) {
        std::println("id={} name={} email={}", row.id, row.columns[0], row.columns[1]);
    }

    return 0;
}
