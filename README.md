<div align="center">
  
# 🚀 Ultra-Low Latency LRU Cache Simulator

**A Zero-Allocation, HFT-Optimized Least Recently Used (LRU) Cache in Modern C++**

[![C++17](https://img.shields.io/badge/C++-17-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![Windows](https://img.shields.io/badge/Windows-10%20|%2011-0078D6.svg?style=for-the-badge&logo=windows)](https://microsoft.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

</div>

---

## 📖 Overview

Built as a masterclass project for Senior Low-Latency C++, High-Frequency Trading (HFT) Infrastructure, and Systems Engineering interviews, this project implements a strict **O(1) LRU Cache** without relying on standard library containers like `std::list` or `std::unordered_map`.

By utilizing contiguous memory pools and open-addressing hash maps, the system achieves predictable nanosecond latency with **exactly zero runtime heap allocations**.

---

## ⚡ Key Features & HFT Optimizations

* **Zero-Allocation Hot Path:** Completely avoids `new`, `delete`, `malloc`, and `free` after the initial startup phase.
* **Array-Based Linked List:** Implements a Doubly Linked List using `int32_t` indices inside a pre-allocated `std::vector`, slashing pointer overhead and preventing CPU cache misses.
* **Open-Addressing Hash Map:** Resolves collisions via linear probing in a flat array, discarding chaining allocations.
* **Backward-Shift Deletions:** Eliminates tombstone pollution in the hash table, keeping load factors mathematically optimal.
* **Bitwise Modulo:** Forces table capacity to powers-of-two, replacing the slow modulo instruction (`%` ~15 cycles) with a lightning-fast bitwise AND (`&` ~1 cycle).
* **32-Byte Cache Line Alignment:** Nodes are `alignas(32)` to guarantee exactly two nodes fit perfectly into a 64-byte x86 CPU cache line, preventing false sharing.

---

## 🧠 Architecture Architecture

```mermaid
graph TD
    UI[CLI Translation Registry] -->|uint64_t Hash| Core[Zero-Allocation Core Engine]
    
    subgraph Core Engine
        HM[Flat Array Hash Map<br/>std::vector<int32_t>] -->|int32_t index| MP
        MP[Contiguous Memory Pool<br/>std::vector<Node>] 
        FL[Free List Array] -->|pop/push| MP
    end
```

---

## ⏱️ Performance Metrics

| Operation | Algorithmic Time | Space Complexity | Runtime Heap Allocations |
|-----------|------------------|------------------|--------------------------|
| **PUT**   | O(1) Strict      | O(N) Total       | **0**                    |
| **GET**   | O(1) Strict      | -                | **0**                    |
| **REMOVE**| O(1) Strict      | -                | **0**                    |
| **EVICT** | O(1) Strict      | -                | **0**                    |

> **Benchmark Results:** Achieves ~6 Million Operations/sec on standard consumer hardware, with average operation latency sitting comfortably at **~160 nanoseconds**.

---

## 🚀 Quick Start Guide

### 1. Requirements
* C++17 Compatible Compiler (MSVC, GCC, Clang)
* CMake 3.20+
* Ninja Build System (optional but recommended)

### 2. Build Instructions

```powershell
# Generate build files
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Compile the project
cmake --build build
```

### 3. Execution

```powershell
# Run the Interactive Simulator
.\build\lru_cache_simulator.exe

# Run the Catch2-style Unit Tests (31/31 Passing)
.\build\lru_cache_tests.exe

# Run the High-Resolution Benchmarks
.\build\lru_cache_benchmark.exe
```

---

## 💻 CLI Simulator Usage

The project includes an interactive terminal simulator to manually test the cache behavior:

```text
  ============================================
      HFT LRU Cache Simulator v2.0
      Ultra Low-Latency Key-Value Store
  ============================================

  Enter cache capacity (1-10000) : 3
  [OK] Cache created with capacity 3.

  1.  Put Item
  2.  Get Item
  3.  Remove Item
  4.  Display Cache
  5.  Cache Statistics
  9.  Exit

  Enter Choice : 1
  Enter key   : GOOG
  Enter value : 1500

  [OK] Stored: GOOG = 1500
```

---

## 📜 Project Structure

```text
📁 In-Memory Key-Value Store/
 ├── 📄 CMakeLists.txt
 ├── 📄 README.md
 ├── 📁 include/
 │    └── 📄 lru_cache.h           (Memory Pool & Hash Map Declarations)
 ├── 📁 src/
 │    ├── 📄 lru_cache.cpp         (Core Zero-Allocation Implementation)
 │    └── 📄 main.cpp              (Interactive CLI & UI String Mapper)
 ├── 📁 tests/
 │    └── 📄 test_lru_cache.cpp    (Automated API and Eviction Unit Tests)
 └── 📁 benchmarks/
      └── 📄 benchmark_lru_cache.cpp (Throughput & Latency Measuring Tool)
```

---

