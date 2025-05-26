#!/bin/bash

if [[ -d  build ]]; then
  rm -rfv build
fi

mkdir build && \
cmake -B build .
cd build && \
make

if [[ ! -f App ]]; then
  echo "Something went wrong!"
  exit 0
fi

echo "--------"
./App
echo "--------"