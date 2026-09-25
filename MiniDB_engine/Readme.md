# MiniDB_engine — Scaffold Scripts

Two Perl scripts that generate the initial directory layout and stub files
for the **mini_db_engine** project: a minimal, in-memory database engine
used to exercise modern C++26 features (coroutines, PMR, lock-free
concurrency, and modules) orchestrated by CMake.

Neither script contains any real engine logic. Their only job is to
produce a directory tree and a set of placeholder files that are just
enough to prove the CMake + C++ modules build configures and links
correctly, before any real implementation work begins.

## Scripts

| Script                        | Role                                                                 |
|--------------------------------|-----------------------------------------------------------------------|
| `generate_project.pl` | Entry point. Creates `mini_db_engine/{src,test,dev}` and calls `init_project_files.pl`. |
| `init_project_files.pl`         | Writes the stub `CMakeLists.txt`, `.cppm` module partitions, `main.cpp`, smoke test, and `dev/` tooling. Can also be run standalone. |

You normally only ever run `generate_project.pl` directly; it locates and 
invokes `init_project_files.pl` for you.

## Requirements

- Perl **v5.40** or later (the scripts declare `use v5.40;`, which enables
  `strict`, `warnings`, and subroutine signatures automatically).
- Only core modules are used: `File::Spec`, `File::Path`, `FindBin`,
  `File::Basename`. No CPAN dependencies to install.
- A regular (non-root) user account — see [Root guard](#root-guard) below.
- To actually build the generated project afterward: CMake ≥ 3.28, Ninja,
  and GCC 16.1.0 (or later) with C++26/modules support.

## Usage

Keep both `.pl` files in the same directory (the first locates the second
via `FindBin`, not via `$PATH` or the current working directory).

```bash
# Generates ./mini_db_engine/ in the current directory
perl generate_project.pl

# Or generate it under a specific base directory instead
perl generate_project.pl /path/to/workspace
```

Running `init_project_files.pl` on its own is also supported, e.g. to
re-populate an existing project root without recreating directories:

```bash
perl init_project_files.pl /path/to/mini_db_engine
```

### Root guard

Both scripts refuse to run as root (checking both the real and effective
UID) and exit immediately with an error, even under `sudo` or `su`. This
avoids accidentally creating root-owned files in what is normally a
regular user's workspace. Run them as your normal user account.

## What gets generated

```
mini_db_engine/
├── CMakeLists.txt            # top-level build: modules library + main exe + tests
├── src/
│   ├── main.cpp                # entry point, `import db;`
│   ├── db.cppm                 # primary module interface, re-exports all partitions
│   ├── db-core.cppm            # :core        — RowId and shared vocabulary (stub)
│   ├── db-storage.cppm         # :storage     — PMR / columnar storage (stub)
│   ├── db-index.cppm           # :index       — lock-free lookup structure (stub)
│   ├── db-wal.cppm             # :wal         — lock-free ring buffer + flusher (stub)
│   ├── db-concurrency.cppm     # :concurrency — thread pool / task<T> scheduler (stub)
│   ├── db-exec.cppm            # :exec        — coroutine-based scan/filter/project (stub)
│   └── db-engine.cppm          # :engine      — public Database facade (stub)
├── test/
│   ├── CMakeLists.txt
│   └── smoke_test.cpp          # links the modules library, asserts it's usable
└── dev/
    ├── README.md
    └── build.sh                # convenience: configure (Ninja) + build + ctest
```

All `.cppm` files contain minimal, valid placeholder types (e.g.
`PlaceholderIndex`, `PlaceholderWal`) — enough for the module graph to
compile and link, nothing more. Real subsystem logic is added later, on
top of this scaffold.

## Building the generated project

```bash
cd mini_db_engine
bash dev/build.sh
```

This configures with `-G Ninja` (required for reliable C++ modules
dependency scanning), builds, and runs `ctest`.

Two things worth checking against your actual environment before relying
on this:

- `dev/build.sh` passes `-DCMAKE_CXX_COMPILER=g++-16`. Adjust this if
  `g++` already resolves to GCC 16 on your system.
- `db-core.cppm` and `db-storage.cppm` use a global module fragment
  (`module; #include <cstddef>;`) instead of `import std;`, to sidestep
  known instabilities around the standard library module in recent GCC
  versions. Switch to `import std;` once you've confirmed it's stable in
  your toolchain.

## Regenerating

Both scripts are idempotent in the sense that re-running them simply
overwrites the stub files in place (`make_path` skips directories that
already exist, and `write_file` truncates/overwrites on each run). There
is no diffing or "skip if exists" behavior — treat re-running as "reset
the scaffold to its stub state."
