#!/usr/bin/env bash
set -euo pipefail

# Refuse to run as root (real or effective uid 0), whether invoked
# directly or via sudo/su. This script shells out to cmake, ninja and
# ctest, which would in turn spawn the compiler and every build/test
# process as root too -- a cascade of unnecessary root privilege that
# this guard exists to stop at the very first step.
if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
    echo "Refusing to run as root (uid 0). Re-run as a regular user." >&2
    exit 1
fi

# Convenience script: configure with Ninja, build, then run the smoke test.
# CMAKE_CXX_STANDARD=26 is passed explicitly here (rather than pinning a
# versioned compiler binary like g++-16) so this keeps working as long as
# whichever `g++` is first on PATH supports C++26.

cmake -S . -B build -G Ninja \
    -DCMAKE_CXX_COMPILER=g++ \
    -DCMAKE_CXX_STANDARD=26
cmake --build build
ctest --test-dir build --output-on-failure
