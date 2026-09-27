# Initial Project Generator

Two Perl scripts that generate the initial directory structure and skeleton files for **mini_db_engine**.

None of the scripts contain the project's actual logic. Their sole function is to create a directory structure and a set of files sufficient to test the build and linking configuration using CMake and C++ modules, before any actual implementation work begins.

## Scripts

| Script                        | Role                                                                 |
|--------------------------------|-----------------------------------------------------------------------|
| `generate_project.pl` | Entry point. Creates `mini_db_engine/{src, dev}` and calls `init_project_files.pl`. |
| `init_project_files.pl` | Writes the stub `CMakeLists.txt`, `.cppm` module partitions, `main.cpp`, and `dev/` tooling. Can also be run standalone. |

You normally only ever run `generate_project.pl` directly; it locates and invokes `init_project_files.pl` for you.

## Requirements

- Perl **v5.40** or later.
- To actually build the generated project afterward: CMake ≥ 3.28, Ninja, and GCC 16.1.0 (or later) with C++26/modules support.

## Usage

```bash
# Generates ./mini_db_engine/ in the current directory
perl generate_project.pl

# Or generate it under a specific base directory instead
perl generate_project.pl /path/to/workspace
```

Running `init_project_files.pl` on its own is also supported, e.g. to re-populate an existing project root without recreating directories:

```bash
perl init_project_files.pl /path/to/mini_db_engine
```

## Building the generated project

```bash
cd mini_db_engine
bash dev/build.sh
```

Once built, both binaries are directly runnable without entering `build/`:

```bash
./bin/mini_db_engine
```
