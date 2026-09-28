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

if [[ -f build/CTestTestfile.cmake ]]; then
    ctest --test-dir build --output-on-failure
fi
