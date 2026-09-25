module;

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

// Partition :index -- a lock-free lookup structure over (Value -> RowId).
//
// Readers atomically load a raw pointer to an immutable, sorted snapshot
// and binary-search it: one atomic load plus a dereference, nothing else.
// This is genuinely lock-free, not just "atomic-looking" -- pointer-sized
// atomics are lock-free on essentially every real platform, and this is
// asserted at compile time below rather than assumed.
//
// (A tempting alternative is std::atomic<std::shared_ptr<T>>, which reads
// nicer but is usually NOT actually lock-free: most standard library
// implementations serialize its operations behind an internal mutex or
// spinlock table to manage the reference count safely. Using a raw
// pointer here is a deliberate choice to keep the reader path provably
// lock-free.)
//
// Writers build a whole new snapshot (copy-on-write) and publish it with
// a compare-and-swap loop, so concurrent writers never block each other
// -- they retry instead (see IndexStats::retry_count).
//
// This is a deliberately simplified relative of RCU: there is no epoch or
// hazard-pointer scheme to reclaim replaced snapshots, so a published
// snapshot is intentionally never freed while the Index is alive (only
// released at Index destruction). Real RCU reclaims a snapshot once no
// reader can still be using it; that reclamation machinery is a whole
// topic on its own and is left out on purpose here, to keep the reader
// path simple and provably lock-free. Watch
// IndexStats::leaked_snapshot_count grow with every write to see this
// trade-off happen.
export module db:index;

import :core;

export namespace db::index {

// One (key, row) pair in a snapshot.
struct Entry {
    core::Value key;
    core::RowId row;
};

// Sorted by key, so lookup can binary search it. Comparing core::Value
// relies on std::variant's built-in lexicographic ordering (by
// alternative index, then by value) -- fine as long as a given Index is
// only ever used with keys of one consistent type, which is the expected
// usage (one column's primary-key values).
using Snapshot = std::vector<Entry>;

// Deliberately not copyable or movable: the atomic member can't be
// copied, and moving a structure that other threads may be concurrently
// reading through wouldn't make sense anyway.
struct Index {
    std::atomic<const Snapshot*> current { new Snapshot() };
    std::atomic<std::size_t> publish_count { 0 };
    std::atomic<std::size_t> retry_count { 0 };
    std::atomic<std::size_t> leaked_snapshot_count { 0 };

    static_assert(std::atomic<const Snapshot*>::is_always_lock_free,
        "expected a genuinely lock-free pointer-sized atomic on this platform");

    Index() = default;
    Index(const Index&) = delete;
    Index& operator=(const Index&) = delete;

    ~Index() { delete current.load(std::memory_order_relaxed); }
};

// Lock-free read path: one atomic load, then an ordinary binary search
// over an immutable snapshot that can never change underneath the caller.
[[nodiscard]] auto lookup(const Index& index, const core::Value& key) -> std::optional<core::RowId>
{
    const Snapshot* snap = index.current.load(std::memory_order_acquire);
    const auto it = std::ranges::lower_bound(*snap, key, {}, &Entry::key);
    if (it != snap->end() && it->key == key) {
        return it->row;
    }
    return std::nullopt;
}

// Inserts a new key or updates an existing one. Builds a full copy of
// the current snapshot with the change applied, then races to publish it
// via compare-and-swap; on failure (someone else published first) it
// retries against the fresh value, so this always makes forward progress
// even under contention.
auto upsert(Index& index, core::Value key, core::RowId row) -> void
{
    const Snapshot* expected = index.current.load(std::memory_order_acquire);
    for (;;) {
        auto next = std::make_unique<Snapshot>(*expected);
        auto it = std::ranges::lower_bound(*next, key, {}, &Entry::key);
        if (it != next->end() && it->key == key) {
            it->row = row;
        } else {
            next->insert(it, Entry { key, row });
        }

        if (index.current.compare_exchange_weak(
                expected, next.get(), std::memory_order_acq_rel, std::memory_order_acquire)) {
            index.publish_count.fetch_add(1, std::memory_order_relaxed);
            // Ownership of `next` moves to the atomic. `expected` still
            // points at the just-replaced snapshot -- intentionally not
            // deleted; see the partition-level comment on reclamation.
            index.leaked_snapshot_count.fetch_add(1, std::memory_order_relaxed);
            (void)next.release();
            return;
        }

        // Lost the race: `next` (a unique_ptr) frees our candidate here
        // as it goes out of scope, and `expected` was refreshed to the
        // current value by compare_exchange_weak, so the loop retries
        // against up-to-date data.
        index.retry_count.fetch_add(1, std::memory_order_relaxed);
    }
}

// Snapshot of an index's size and lock-free/contention behavior, for
// observing how it behaves under load.
struct IndexStats {
    std::size_t entry_count;
    std::size_t publish_count;
    std::size_t retry_count;
    std::size_t leaked_snapshot_count;
    bool is_lock_free;
};

[[nodiscard]] auto stats(const Index& index) -> IndexStats
{
    const Snapshot* snap = index.current.load(std::memory_order_acquire);
    return IndexStats {
        .entry_count = snap->size(),
        .publish_count = index.publish_count.load(std::memory_order_relaxed),
        .retry_count = index.retry_count.load(std::memory_order_relaxed),
        .leaked_snapshot_count = index.leaked_snapshot_count.load(std::memory_order_relaxed),
        .is_lock_free = index.current.is_lock_free(),
    };
}

} // namespace db::index
