// Partition :engine -- public facade tying every subsystem together.
// Stub content only: real Database will expose create_table / insert /
// get / scan / flush.
export module db:engine;

import :core;
import :storage;
import :index;
import :wal;
import :concurrency;
import :exec;

export namespace db::engine {

struct Database {
    [[nodiscard]] auto is_ready() const noexcept -> bool { return true; }
};

} // namespace db::engine
