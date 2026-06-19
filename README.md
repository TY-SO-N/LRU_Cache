# LRU Cache Simulator

A beginner-friendly, interview-ready **Least Recently Used (LRU) Cache** implementation in Modern C++.

Built for freshers preparing for C++ Developer, Systems, and Backend engineering roles.

---

## What It Does

An interactive command-line simulator for an LRU cache with O(1) operations.

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

## Commands

| Command | Description |
|---|---|
| `PUT <key> <value>` | Insert or update a key-value pair |
| `GET <key>` | Retrieve value by key (promotes to MRU) |
| `DISPLAY` | Show cache contents from MRU to LRU |
| `STATS` | Show hit/miss/eviction statistics |
| `CLEAR` | Clear all entries and reset stats |
| `HELP` | Show available commands |
| `EXIT` | Quit the simulator |

---

## Project Structure

```
In-Memory Key-Value Store/
  CMakeLists.txt
  README.md
  include/
    lru_cache.h           Class declaration
  src/
    lru_cache.cpp          Core implementation
    main.cpp               Interactive CLI
  tests/
    test_lru_cache.cpp     Unit tests (31 tests)
  benchmarks/
    benchmark_lru_cache.cpp  Performance benchmarks
```

---

## Data Structures

```
  unordered_map                doubly-linked list (std::list)
  key -> iterator  ───────>   {key, value} nodes

  O(1) lookup                 O(1) insert/remove/reorder
```

- **std::unordered_map** maps keys to list iterators for O(1) lookup
- **std::list** maintains usage order; front = MRU, back = LRU
- **std::list::splice** moves nodes in O(1) without copying

---

## Complexity

| Operation | Time | Space |
|---|---|---|
| PUT | O(1) amortized | O(n) total |
| GET | O(1) amortized | |
| DELETE | O(1) amortized | |
| EVICT | O(1) | |
| DISPLAY | O(n) | |

---

## Requirements

- C++17 compatible compiler
- CMake 3.20+
- Windows 10/11

---

## License

This project is for educational and interview preparation purposes.
