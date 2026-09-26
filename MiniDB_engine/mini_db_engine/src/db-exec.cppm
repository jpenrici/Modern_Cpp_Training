module;

#include <concepts>
#include <coroutine>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

// Partition :exec -- lazy scan/filter/project pipelines over a Table,
// built on a small hand-written coroutine generator.
//
// This deliberately does NOT use std::generator (<generator>, C++23):
// library support for it is still quite new across implementations, and
// writing a minimal one here is more didactic anyway -- it shows exactly
// how a generator's suspend/resume/iterator machinery works instead of
// hiding it behind the standard library.
//
// Nothing in this partition touches threads or atomics. Each Generator
// is single-owner and meant to be driven by one consumer at a time, like
// an ordinary iterator -- the concurrency story for this engine lives in
// :wal and :concurrency, not here.
export module db:exec;

import :core;
import :storage;

export namespace db::exec {

// A minimal single-pass, lazy generator. Values are produced one at a
// time via co_yield, each one suspending immediately (initial_suspend
// and every yield use suspend_always) until the consumer asks for the
// next one by advancing the iterator.
template <typename T>
class Generator {
public:
    struct promise_type {
        T current_value;

        [[nodiscard]] auto get_return_object() noexcept -> Generator
        {
            return Generator { std::coroutine_handle<promise_type>::from_promise(*this) };
        }
        [[nodiscard]] auto initial_suspend() noexcept -> std::suspend_always { return {}; }
        [[nodiscard]] auto final_suspend() noexcept -> std::suspend_always { return {}; }
        auto yield_value(T value) noexcept -> std::suspend_always
        {
            current_value = std::move(value);
            return {};
        }
        void return_void() noexcept { }
        void unhandled_exception() { std::terminate(); }
    };

    struct Iterator {
        std::coroutine_handle<promise_type> handle;

        auto operator++() -> Iterator&
        {
            handle.resume();
            return *this;
        }
        [[nodiscard]] auto operator*() const -> const T& { return handle.promise().current_value; }
        [[nodiscard]] auto operator==(std::default_sentinel_t) const -> bool { return !handle || handle.done(); }
        [[nodiscard]] auto operator!=(std::default_sentinel_t s) const -> bool { return !(*this == s); }
    };

    explicit Generator(std::coroutine_handle<promise_type> h)
        : handle_(h)
    {
    }
    ~Generator()
    {
        if (handle_) {
            handle_.destroy();
        }
    }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;
    Generator(Generator&& other) noexcept
        : handle_(std::exchange(other.handle_, {}))
    {
    }
    Generator& operator=(Generator&&) = delete;

    // Priming the first value happens here, on the first call to
    // begin() -- not at construction -- which is what makes an
    // unconsumed Generator do genuinely nothing until iterated.
    [[nodiscard]] auto begin() -> Iterator
    {
        if (handle_) {
            handle_.resume();
        }
        return Iterator { handle_ };
    }
    [[nodiscard]] auto end() -> std::default_sentinel_t { return {}; }

private:
    std::coroutine_handle<promise_type> handle_;
};

// Yields every RowId in a table, in order. Nothing is read or copied
// from any column -- this is purely positions, generated one at a time.
auto scan(const storage::Table& table) -> Generator<core::RowId>
{
    const auto n = storage::row_count(table);
    for (core::RowId row = 0; row < n; ++row) {
        co_yield row;
    }
}

// Passes through only the RowIds satisfying `predicate`. Composes with
// scan() (or another filter()) by consuming its input Generator lazily:
// nothing upstream runs further than whatever this loop has pulled so
// far.
template <std::predicate<core::RowId> Predicate>
auto filter(Generator<core::RowId> rows, Predicate predicate) -> Generator<core::RowId>
{
    for (auto row : rows) {
        if (predicate(row)) {
            co_yield row;
        }
    }
}

// Reads back the given columns for each incoming RowId, one row at a
// time. Still lazy at the row level -- no more of `rows` is consumed,
// and no more cells are read, than the caller actually iterates.
auto project(Generator<core::RowId> rows, const storage::Table& table, std::vector<std::size_t> columns)
    -> Generator<std::vector<core::Value>>
{
    for (auto row : rows) {
        std::vector<core::Value> values;
        values.reserve(columns.size());
        for (auto column_index : columns) {
            values.push_back(storage::get_cell(table, row, column_index));
        }
        co_yield std::move(values);
    }
}

} // namespace db::exec
