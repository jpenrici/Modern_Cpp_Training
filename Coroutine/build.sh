#!/usr/bin/env bash

set -euo pipefail

if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    echo "Refusing to run as root (uid 0). Re-run as a regular user." >&2
    exit 1
fi

if [[ $# -gt 0 ]]; then
    case "$1" in
    rebuild)
        echo "Cleaning the build and bin directories and rebuild..."
        rm -rfv build bin
        ;;
    *)
        echo "Invalid option: $1" >&2
        echo ""
        usage
        exit 1
        ;;
    esac
fi

cmake -S . -B build \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=26
cmake --build build
