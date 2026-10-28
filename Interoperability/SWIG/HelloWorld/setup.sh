#!/bin/bash

set -euo pipefail

if [[ "${EUID}" -eq 0 ]]; then
    echo "Not recommended for use as root!"
    exit 1
fi

cd "$(dirname "$0")"

mkdir -p build bin lib/auto/module_calc

echo "[1/5] Generates the wrapper (.cxx e .pm)"
swig -perl5 -c++ -outdir lib -o build/calc_wrap.cxx src/calc.i

echo "[2/5] Compile the wrapper"
g++ -std=c++23 -fPIC -c build/calc_wrap.cxx -o build/calc_wrap.o \
    -Isrc $(perl -MExtUtils::Embed -e ccopts)

echo "[3/5] Generates the .so file in the location expected by DynaLoader"
g++ -shared build/calc_wrap.o -o lib/auto/module_calc/module_calc.so

echo "[4/5] Pure C++ test"
g++ -std=c++23 src/test.cpp -o bin/test && ./bin/test

echo "[5/5] Test via Perl"
perl -Ilib -Mmodule_calc -e 'print "Perl: ", module_calc::calc(10, 5.5), "\n"'
