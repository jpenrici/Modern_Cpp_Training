#!/usr/bin/env perl
# ---------------------------------------------------------------------------
# init_project_files.pl
#
# run standalone:
#
#   perl init_project_files.pl /path/to/mini_db_engine
# ---------------------------------------------------------------------------
use v5.40;

use File::Spec;
use File::Path     qw(make_path);
use File::Basename qw(dirname);
use POSIX          qw(strftime);

main();

sub main {
    guard_against_root();

    my $project_root = $ARGV[0]
      or logger( "error", "Usage: $0 <project_root>\n" );

    logger( "error", " Project root does not exist: $project_root" )
      unless -d $project_root;

    logger( "info", "Writing files into: $project_root" );

    write_all_files( $project_root, file_manifest() );
    mark_executable( File::Spec->catfile( $project_root, 'dev', 'build.sh' ) );

    logger( "info", "Files written." );

    return 0;
}

sub guard_against_root {
    if ( $< == 0 || $> == 0 ) {
        logger( "error",
            "Refusing to run as root (uid 0). Re-run as a regular user." );
    }
}

sub logger ( $level, $message ) {
    $level = uc($level);

    my $timestamp = strftime( "%Y-%m-%d %H:%M:%S", localtime );
    my $log       = "[$timestamp] [$level] $message";

    if ( $level eq "ERROR" ) {
        say STDERR $log;
        exit 1;
    }

    say $log;
}

sub file_manifest {
    return {
        'CMakeLists.txt'      => cmake_top_level_content(),
        'src/main.cpp'        => main_cpp_content(),
        'src/db.cppm'         => module_primary_content(),
        'src/db-exec.cppm'    => module_exec_content(),
        'src/db-storage.cppm' => module_storage_content(),
        'dev/build.sh'        => dev_build_script_content(),
        'dev/README.md'       => dev_readme_content(),
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
    open my $fh, '>', $path
      or logger( "Error", "Cannot open '$path' for writing: $!" );
    print {$fh} $content;
    close $fh or logger( "Error", "Cannot close '$path': $!" );
    logger( "info", "Wrote $path" );

    return;
}

sub mark_executable ($path) {
    return unless -f $path;
    chmod 0755, $path or warn "Could not chmod '$path': $!\n";

    return;
}

# ---- File content builders -------------------------------------------------

sub cmake_top_level_content {
    return <<'CMAKE';
cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

project(mini_db_engine VERSION 1.0.0 DESCRIPTION "Mini database engine for study purposes." LANGUAGES CXX)

set(PROGRAM_NAME minidb)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if (NOT CMAKE_GENERATOR MATCHES "Ninja")
    message(WARNING
        "${PROGRAM_NAME} relies on C++ modules; configure with -G Ninja "
        "for reliable module dependency scanning.")
endif()

add_library(${PROGRAM_NAME}_modules STATIC)

target_sources(${PROGRAM_NAME}_modules
    PUBLIC
        FILE_SET CXX_MODULES FILES
            src/db.cppm
            src/db-exec.cppm
            src/db-storage.cppm
)

add_executable(${PROGRAM_NAME} src/main.cpp)

target_link_libraries(${PROGRAM_NAME} PRIVATE ${PROGRAM_NAME}_modules Threads::Threads)
target_compile_options(${PROGRAM_NAME}_modules PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(${PROGRAM_NAME} PRIVATE -Wall -Wextra -Wpedantic)
target_link_options(${PROGRAM_NAME} PRIVATE -static-libstdc++ -static-libgcc)
CMAKE
}

sub dev_readme_content {
    return <<'MD';
# dev/

Scratch space for local development tooling: build helpers, sanitizer
configs, benchmarking notes, and similar. Nothing here is part of the
shipped project structure described by the top-level CMakeLists.txt.

- `build.sh` -- configure and build, using Ninja.
MD
}

sub dev_build_script_content {
    return <<'SH';
#!/usr/bin/env bash

set -euo pipefail

[[ ${EUID:-$(id -u)} -eq 0 ]] && { echo "Refusing to run as root (uid 0)." >&2; exit 1; }

usage() {
    cat <<EOF
Usage: $(basename "$0") [clean|rebuild|-h|--help]

Options:
  clean         Removes 'build' and 'bin' directories.
  rebuild       Clean and compile again.
  -h, --help    Displays this help message.

No arguments: Configures (CMake) and compiles the project.
EOF
}

cd "$(dirname "$(readlink -f "$0")")/.."

if [[ $# -gt 0 ]]; then
    case "$1" in
        clean|rebuild)
            echo "Cleaning directories..."
            rm -rfv build bin
            [[ "$1" == "clean" ]] && exit 0
            ;;
        -h|--help) usage; exit 0 ;;
        *) echo "Invalid option: $1" >&2; echo; usage; exit 1 ;;
    esac
fi

cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=26
cmake --build build
SH
}

sub main_cpp_content {
    return <<'CPP';
#include <print>

import db;

auto main() -> int {

    std::println("module db::exec {}", db::exec::is_ready());
    std::println("module db::storage {}", db::storage::is_ready());

    return 0;
}
CPP
}

sub module_primary_content {
    return <<'CPPM';
export module db;

export import :exec;
export import :storage;

CPPM
}

sub module_exec_content {
    return <<'CPPM';
export module db:exec;

export namespace db::exec {
    [[nodiscard]] auto is_ready() noexcept -> bool { return true; }
} // namespace db::exec
CPPM
}

sub module_storage_content {
    return <<'CPPM';
export module db:storage;

export namespace db::storage {
    [[nodiscard]] auto is_ready() noexcept -> bool { return true; }
} // namespace db::storage
CPPM
}
