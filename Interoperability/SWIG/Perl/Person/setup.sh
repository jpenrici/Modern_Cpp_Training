#!/bin/bash

set -euo pipefail

if [[ "${EUID}" -eq 0 ]]; then
    echo "Not recommended for use as root!"
    exit 1
fi

cd "$(dirname "$0")"

# Use: ./setup.sh [perl|lua|all]   (default: all)
TARGET="${1:-all}"

mkdir -p build bin lib/auto/module_person lib/lua

test_cpp() {
    echo "[C++] Pure C++ test"
    g++ -std=c++23 src/test.cpp -o bin/test && ./bin/test
}

build_perl() {
    echo "[Perl 1/3] Generates the wrapper (.cxx e .pm)"
    swig -perl5 -c++ -outdir lib -o build/person_perl_wrap.cxx src/person.i

    echo "[Perl 2/3] Compile the wrapper"
    g++ -std=c++23 -fPIC -c build/person_perl_wrap.cxx -o build/person_perl_wrap.o \
        -Isrc $(perl -MExtUtils::Embed -e ccopts)

    echo "[Perl 3/3] Generates the .so in the location expected by DynaLoader"
    # DynaLoader requires <path>/auto/module_person/module_person.so
    g++ -shared build/person_perl_wrap.o -o lib/auto/module_person/module_person.so

    echo "[Perl] Test"
    perl -Ilib src/test.pl
}

build_lua() {
    local pc=""
    for name in lua5.4 lua-5.4 lua; do
        if pkg-config --exists "${name}" 2>/dev/null; then
            pc="${name}"
            break
        fi
    done
    if [[ -z "${pc}" ]]; then
        echo "Lua development files not found (try: sudo apt install liblua5.4-dev pkg-config)"
        exit 1
    fi

    # Interpreter binary
    local lua_bin
    lua_bin="$(command -v lua5.4 || command -v lua)"

    echo "[Lua 1/3] Generates the wrapper (.cxx)"
    swig -lua -c++ -o build/person_lua_wrap.cxx src/person.i

    echo "[Lua 2/3] Compile the wrapper"
    g++ -std=c++23 -fPIC -c build/person_lua_wrap.cxx -o build/person_lua_wrap.o \
        -Isrc $(pkg-config --cflags "${pc}")

    echo "[Lua 3/3] Generates the .so (luaopen_module_person is resolved by require)"
    g++ -shared build/person_lua_wrap.o -o lib/lua/module_person.so

    echo "[Lua] Test"
    LUA_CPATH="lib/lua/?.so;;" "${lua_bin}" src/test.lua
}

case "${TARGET}" in
perl)
    test_cpp
    build_perl
    ;;
lua)
    test_cpp
    build_lua
    ;;
all)
    test_cpp
    build_perl
    build_lua
    ;;
*)
    echo "Usage: $0 [perl|lua|all]"
    exit 1
    ;;
esac
