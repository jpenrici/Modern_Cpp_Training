# Modern C++ Training

A collection of focused C++ examples and experiments for learning and practicing modern C++ features.

## Topics covered

This repository includes exercises and samples for:

- concepts and generic programming
- lambdas and functional patterns
- ranges and views
- optional, variant, tuples, and type utilities
- coroutines and generators
- modules and reflection
- performance, memory, and low-level C++ patterns
- practical mini projects and toy implementations

## Requirements

- CMake 3.25 or newer
- A C++23-compatible compiler
  - GCC 15+
  - Clang 20+
  - MSVC with modern C++23 support

## Build

```bash
cmake -S . -B build
cmake --build build
```

Some directories may contain standalone examples with their own build setup.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE).
