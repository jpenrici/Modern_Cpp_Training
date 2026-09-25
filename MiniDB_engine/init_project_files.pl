#!/usr/bin/env perl
# ---------------------------------------------------------------------------
# init_project_files.pl
#
# Populates a "mini_db_engine" project root (already scaffolded by
# generate_project_hierarchy.pl) with minimal, valid stub files:
#
#   - CMakeLists.txt (+ test/CMakeLists.txt)
#   - src/db.cppm and its partitions (:core, :storage, :index, :wal,
#     :concurrency, :exec, :engine) -- empty-ish placeholders, just enough
#     to prove the module graph configures and links with Ninja.
#   - src/main.cpp and test/smoke_test.cpp
#   - dev/README.md and dev/build.sh (a convenience configure+build+test
#     helper script)
#
# Intended to be called by generate_project_hierarchy.pl, but can also be
# run standalone:
#
#   perl init_project_files.pl /path/to/mini_db_engine
# ---------------------------------------------------------------------------
use v5.40;

use File::Spec;
use File::Path     qw(make_path);
use File::Basename qw(dirname);

main();

sub main {
    guard_against_root();

    my $project_root = $ARGV[0] or die "Usage: $0 <project_root>\n";

    die "Project root does not exist: $project_root\n" unless -d $project_root;

    say "==> Writing scaffold files into: $project_root";

    write_all_files( $project_root, file_manifest() );
    mark_executable( File::Spec->catfile( $project_root, 'dev', 'build.sh' ) );

    say "==> Scaffold files written.";

    return 0;
}

# Maps project-relative paths to their stub content.
sub file_manifest {
    return {
        'CMakeLists.txt'          => cmake_top_level_content(),
        'src/db.cppm'             => module_primary_content(),
        'src/db-core.cppm'        => module_core_content(),
        'src/db-storage.cppm'     => module_storage_content(),
        'src/db-index.cppm'       => module_index_content(),
        'src/db-wal.cppm'         => module_wal_content(),
        'src/db-concurrency.cppm' => module_concurrency_content(),
        'src/db-exec.cppm'        => module_exec_content(),
        'src/db-engine.cppm'      => module_engine_content(),
        'src/main.cpp'            => main_cpp_content(),
        'test/CMakeLists.txt'     => cmake_test_content(),
        'test/smoke_test.cpp'     => smoke_test_content(),
        'dev/README.md'           => dev_readme_content(),
        'dev/build.sh'            => dev_build_script_content(),
    };
}

sub write_all_files ( $project_root, $manifest ) {
    for my $relative_path ( sort keys %$manifest ) {
        my $full_path =
          File::Spec->catfile( $project_root, split m{/}, $relative_path );
        make_path( dirname($full_path) );
        write_file( $full_path, $manifest->{$relative_path} );
    }

    return;
}

sub write_file ( $path, $content ) {
    open my $fh, '>', $path or die "Cannot open '$path' for writing: $!\n";
    print {$fh} $content;
    close $fh or die "Cannot close '$path': $!\n";
    say "  wrote $path";

    return;
}

sub mark_executable ($path) {
    return unless -f $path;
    chmod 0755, $path or warn "Could not chmod '$path': $!\n";

    return;
}

# Refuses to proceed when run as root (real or effective uid 0), either
# directly or via sudo/su. This script can also be invoked standalone
# (not just via generate_project_hierarchy.pl), so it needs its own
# guard rather than relying on the caller's check.
sub guard_against_root {
    if ( $< == 0 || $> == 0 ) {
        die "Refusing to run as root (uid 0). Re-run as a regular user.\n";
    }

    return;
}

# ---- File content builders -------------------------------------------------
# Kept as plain single-quoted heredocs (no interpolation) for readability.

sub cmake_top_level_content {
    return <<'CMAKE';
cmake_minimum_required(VERSION 3.28)
project(mini_db_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C++20/23 module dependency scanning (P1689). Reliable support currently
# requires the Ninja generator (or MSVC) -- not Unix Makefiles.
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

if (NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(WARNING
        "mini_db_engine relies on C++ modules; configure with -G Ninja "
        "for reliable module dependency scanning.")
endif()

# All module partitions live in a single library target so that both the
# main executable and the test executable can `import db;` without the
# module graph being scanned/compiled twice.
add_library(mini_db_engine_modules STATIC)

target_sources(mini_db_engine_modules
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/db.cppm
            src/db-core.cppm
            src/db-storage.cppm
            src/db-index.cppm
            src/db-wal.cppm
            src/db-concurrency.cppm
            src/db-exec.cppm
            src/db-engine.cppm
)

target_compile_options(mini_db_engine_modules PRIVATE -Wall -Wextra -Wpedantic)

add_executable(mini_db_engine src/main.cpp)
target_link_libraries(mini_db_engine PRIVATE mini_db_engine_modules)

enable_testing()
add_subdirectory(test)
CMAKE
}

sub cmake_test_content {
    return <<'CMAKE';
add_executable(smoke_test smoke_test.cpp)
target_link_libraries(smoke_test PRIVATE mini_db_engine_modules)

add_test(NAME smoke_test COMMAND smoke_test)
CMAKE
}

sub module_primary_content {
    return <<'CPPM';
// Primary module interface unit -- re-exports every partition as the
// single public `db` module. Consumers only ever write `import db;`.
export module db;

export import :core;
export import :storage;
export import :index;
export import :wal;
export import :concurrency;
export import :exec;
export import :engine;
CPPM
}

sub module_core_content {
    return <<'CPPM';
module;

#include <cstddef>

// Partition :core -- fundamental vocabulary shared by every other
// partition (row identifiers, value types, schema descriptors, concepts).
// Stub content only: just enough for the module graph to link.
export module db:core;

export namespace db::core {

// Placeholder row identifier; will become the real row/slot addressing
// scheme once storage layout is designed.
using RowId = std::size_t;

} // namespace db::core
CPPM
}

sub module_storage_content {
    return <<'CPPM';
module;

#include <cstddef>

// Partition :storage -- PMR-based columnar storage (Data-Oriented Design).
// Stub content only: real columns will be std::pmr::vector<T> per field.
export module db:storage;

import :core;

export namespace db::storage {

struct PlaceholderColumn {
    std::size_t row_count = 0;
};

} // namespace db::storage
CPPM
}

sub module_index_content {
    return <<'CPPM';
// Partition :index -- lock-free lookup structure (RCU-lite candidate).
// Stub content only.
export module db:index;

import :core;

export namespace db::index {

struct PlaceholderIndex {};

} // namespace db::index
CPPM
}

sub module_wal_content {
    return <<'CPPM';
// Partition :wal -- lock-free ring buffer + coroutine-based flusher.
// Stub content only.
export module db:wal;

import :core;

export namespace db::wal {

struct PlaceholderWal {};

} // namespace db::wal
CPPM
}

sub module_concurrency_content {
    return <<'CPPM';
// Partition :concurrency -- thread pool and task<T> coroutine scheduler.
// Stub content only.
export module db:concurrency;

export namespace db::concurrency {

struct PlaceholderScheduler {};

} // namespace db::concurrency
CPPM
}

sub module_exec_content {
    return <<'CPPM';
// Partition :exec -- lazy scan/filter/project pipelines built on
// coroutine generators. Stub content only.
export module db:exec;

import :core;
import :storage;

export namespace db::exec {

struct PlaceholderPipeline {};

} // namespace db::exec
CPPM
}

sub module_engine_content {
    return <<'CPPM';
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
CPPM
}

sub main_cpp_content {
    return <<'CPP';
#include <print>

import db;

auto main() -> int {
    const db::engine::Database engine;
    std::println("mini_db_engine bootstrap OK: {}", engine.is_ready());
    return 0;
}
CPP
}

sub smoke_test_content {
    return <<'CPP';
#include <cassert>
#include <print>

import db;

// Minimal smoke test: only checks that the module graph links and the
// facade type is constructible. Real behavioral tests come later.
auto main() -> int {
    const db::engine::Database engine;
    assert(engine.is_ready());
    std::println("smoke_test passed");
    return 0;
}
CPP
}

sub dev_readme_content {
    return <<'MD';
# dev/

Scratch space for local development tooling: build helpers, sanitizer
configs, benchmarking notes, and similar. Nothing here is part of the
shipped project structure described by the top-level CMakeLists.txt.

- `build.sh` -- configure, build and run tests in one step, using Ninja.
MD
}

sub dev_build_script_content {
    return <<'SH';
#!/usr/bin/env bash
set -euo pipefail

# Convenience script: configure with Ninja, build, then run the smoke test.
# Adjust CMAKE_CXX_COMPILER if g++-16 is not the default `g++` on this system.

cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++-16
cmake --build build
ctest --test-dir build --output-on-failure
SH
}
