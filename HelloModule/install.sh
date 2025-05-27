#!/bin/bash

if [[ ! -f CMakeLists.txt ]]; then
  echo "CMakeLists.txt not found!"
  exit 1
fi

if [[ -d  build ]]; then
  echo "Remove 'build' directory."
  rm -rf build
fi

# Configuring to use GCC 15
export CXX="/opt/gcc-15/bin/g++"

mkdir build && cmake -B build .
cd build && make

if [[ ! -f App ]]; then
  echo "Executable not found!"
  exit 1
fi

echo "--------"
./App
echo "--------"

exit 0
