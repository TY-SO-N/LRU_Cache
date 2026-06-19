# HFT LRU Cache Simulator

An ultra-low latency, zero-allocation **Least Recently Used (LRU) Cache** implementation in Modern C++.

Built as a masterclass project for Senior Low-Latency C++, HFT Infrastructure, and Systems Engineering interviews.

---

## What It Does

An interactive command-line simulator for an LRU cache with strict O(1) operations that avoids dynamic memory allocation at runtime.

When the cache is full and a new entry is inserted, the **least recently used** item is evicted.

---

## Quick Start

### Build

```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run

```powershell
.\build\lru_cache_simulator.exe
```

### Test

```powershell
.\build\lru_cache_tests.exe
```

### Benchmark

```powershell
.\build\lru_cache_benchmark.exe
```

---

## Project Structure

```
In-Memory Key-Value Store/
  CMakeLists.txt
  README.md
  include/
    lru_cache.h           Class declaration (Memory Pool & Hash Map)
  src/
    lru_cache.cpp          Core zero-allocation implementation
    main.cpp               Interactive CLI with UI string registry
  tests/
    test_lru_cache.cpp     Unit tests (31 tests)
  benchmarks/
    benchmark_lru_cache.cpp  Performance benchmarks (Percentiles & Throughput)
```

---

## Architecture & Data Structures

The project has been refactored for HFT (High-Frequency Trading) environments to eliminate `std::list` and `std::unordered_map`.

- **Contiguous Memory Pool**: A single `std::vector<Node>` pre-allocated at startup acts as an index-based Doubly Linked List for tracking LRU order.
- **Open-Addressing Hash Table**: A flat `std::vector<int32_t>` implements a custom linear-probing hash map. Table size is fixed to a power-of-two for 1-cycle bitwise modulo lookups.
- **Zero Allocations**: Operations like PUT, GET, and EVICT use a lock-free Free List array and backward shifting to guarantee O(1) processing with exactly **zero runtime heap allocations**.

---

## Complexity

| Operation | Time | Space | Allocations |
|---|---|---|---|
| PUT | O(1) strict | O(n) total | 0 |
| GET | O(1) strict | | 0 |
| DELETE | O(1) strict | | 0 |
| EVICT | O(1) strict | | 0 |

---

## Requirements

- C++17 compatible compiler
- CMake 3.20+
- Windows 10/11

---

## License

This project is for educational and interview preparation purposes.
