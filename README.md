<div align="center">
  
# 🚀 Ultra-Low Latency LRU Cache Simulator

**A Zero-Allocation, HFT-Optimized Least Recently Used (LRU) Cache in Modern C++**

[![C++17](https://img.shields.io/badge/C++-17-blue.svg?style=for-the-badge&logo=c%2B%2B)](https://isocpp.org/)
[![CMake](https://img.shields.io/badge/CMake-3.20+-green.svg?style=for-the-badge&logo=cmake)](https://cmake.org/)
[![Windows](https://img.shields.io/badge/Windows-10%20|%2011-0078D6.svg?style=for-the-badge&logo=windows)](https://microsoft.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)](https://opensource.org/licenses/MIT)

<br/>

![CLI Demo Animation](./cli_demo.gif)

</div>

---

## 📖 What is this project?

This project is an interactive command-line simulator for an **LRU (Least Recently Used) Cache**, built entirely from scratch. 

A "Cache" is a fast, temporary storage system. Because it has a limited size (capacity), when it gets full, it has to delete an old item to make room for a new one. An **LRU Cache** solves this by deleting the item that hasn't been used for the longest amount of time.

While building an LRU Cache is a common LeetCode problem, **this project goes much further.** It is designed specifically for **High-Frequency Trading (HFT)** environments where systems must respond in nanoseconds. To achieve this, we had to throw away standard C++ tools and build highly specialized, hardware-sympathetic data structures.

---

## 🛑 Why not use standard C++ libraries?

A standard LRU cache uses a `std::unordered_map` for fast lookups and a `std::list` to track the order of items. However, in an ultra-low latency environment, these standard libraries are dangerously slow for two reasons:

1. **Heap Allocation (`new` / `delete`):** Every time you add an item to a `std::list`, the system has to ask the Operating System for memory. This causes unpredictable latency spikes (context switches). 
2. **CPU Cache Misses:** A standard linked list uses pointers. Pointers scatter your data randomly across the computer's RAM. When the CPU tries to read the next item, it has to wait hundreds of cycles to fetch it from RAM because it isn't sequentially loaded in the CPU's L1/L2 cache.

---

## ⚡ How we achieved HFT speeds (The Concepts)

To hit nanosecond latencies, we applied the following advanced engineering concepts:

### 1. The Zero-Allocation Memory Pool
Instead of asking the OS for memory every time a user types a new key, we pre-allocate one massive array (`std::vector<Node>`) the moment the program starts. This is our **Memory Pool**. 
Once the program is running, the words `new`, `delete`, `malloc`, and `free` are **never** used again. If we need a node, we grab an empty slot from the pool. When an item is evicted, we just mark its slot as empty again.

### 2. Array-Based Linked List (No Pointers)
Because we use a single flat array for all our nodes, we completely removed memory pointers (`*`). Instead of a node pointing to a memory address, our nodes just store the integer `index` of the next node in the array (e.g., "my next node is at index 4"). This perfectly preserves CPU cache locality.

### 3. Open-Addressing Hash Map (Linear Probing)
Standard hash maps resolve collisions by creating a "chain" (a linked list hanging off the hash bucket). This destroys cache locality. 
Instead, we use a single flat array of integers (`std::vector<int32_t>`). If two keys hash to the same bucket, we simply check the very next bucket in the array until we find an empty one. This is called **Linear Probing** and it is incredibly fast because the CPU automatically prefetches adjacent array slots.

### 4. Backward-Shift Deletions (No Tombstones)
When you delete an item from a Linear Probing hash map, it creates a "hole" that breaks the collision chain. Most developers fix this by leaving a "tombstone" (a fake node marking it as deleted). However, tombstones eventually fill up the map and ruin performance. 
We implemented a highly advanced **Backward-Shift Algorithm** that slides subsequent collision elements backward into the hole, keeping the array perfectly dense and load factors mathematically optimal forever.

### 5. Bitwise Modulo Hack
Normally, a hash map finds a bucket using the modulo operator: `bucket = hash % capacity`. However, the division/modulo instruction (`DIV`) is one of the slowest operations on a CPU, taking ~15 clock cycles. 
We force our hash table to always have a capacity that is a strict power of two (8, 16, 32...). This allows us to replace the slow modulo with a lightning-fast bitwise AND: `bucket = hash & (capacity - 1)`. This takes exactly **1 clock cycle**.

### 6. Cache Line Alignment
Modern x86 CPUs load memory in 64-byte chunks called "Cache Lines". If a struct is an awkward size (like 40 bytes), it might accidentally span across two different cache lines, forcing the CPU to do twice the work to read it. We used `alignas(32)` to force our `Node` structs to be exactly 32 bytes. This guarantees that exactly two nodes fit perfectly into a single 64-byte CPU cache line.

---

## 🧠 Architecture Diagram

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

## 🔍 Algorithm Dry Run (Capacity = 3)

Let's walk through how this engine manages memory using **integer indices** instead of pointers, ensuring zero heap allocations happen at runtime.

### Initial State & Data Structures
| Component | Implementation | State at Startup |
|---|---|---|
| **Memory Pool** | `std::vector<Node> nodes_` | Sized to exactly `3` elements. |
| **Hash Table** | `std::vector<int32_t> hash_table_` | Sized to `8` (next power of 2 >= `3*2`). Initialized to `-1` (empty). |
| **Free List** | `int32_t free_head_` | Starts at index `0`. (Links: `0 -> 1 -> 2 -> -1`) |

### 1. `PUT(A, 10)`
- **Free List Pop:** We grab index `0` from the pre-allocated memory pool.
- **Insert Node:** `nodes_[0] = {key: A, val: 10}`
- **Hash Table:** Hash `A`, map its bucket to index `0`.
- **LRU Order:** `[A]` (MRU=0, LRU=0)

### 2. `PUT(B, 20)` & `PUT(C, 30)`
- **Free List Pop:** We grab indices `1` and `2`.
- **LRU Order:** `[C] -> [B] -> [A]` (MRU=2, LRU=0)
- *The cache is now completely full (3/3).*

### 3. `GET(A)` (Hit!)
- **Hash Lookup:** Hash `A` -> instantly returns index `0`.
- **Promote:** Unlink index `0` from the tail and relink it to the head.
- **LRU Order:** `[A] -> [C] -> [B]` (MRU=0, LRU=1)
- *Notice: Zero memory was allocated. We just swapped 32-bit integers (`prev`/`next`)!*

### 4. `PUT(D, 40)` (Eviction Triggered!)
- **Evict LRU:** Identify LRU index `1` (which currently holds `B`).
- **Backward Shift:** Remove `B` from the flat hash array and execute a backward shift on any collision probes to smoothly fill the hole.
- **Reuse Memory:** We do NOT call `delete`. Index `1` is simply pushed back to the Free List.
- **Insert New:** We pop index `1` from the Free List and overwrite it with `D`.
- **LRU Order:** `[D] -> [A] -> [C]` (MRU=1, LRU=2)

---

## ⏱️ Performance Metrics

| Operation | Algorithmic Time | Space Complexity | Runtime Heap Allocations |
|-----------|------------------|------------------|--------------------------|
| **PUT**   | O(1) Strict      | O(N) Total       | **0**                    |
| **GET**   | O(1) Strict      | -                | **0**                    |
| **REMOVE**| O(1) Strict      | -                | **0**                    |
| **EVICT** | O(1) Strict      | -                | **0**                    |

> **Benchmark Results:** Achieves ~6 Million Operations/sec on standard consumer hardware, with average operation latency sitting comfortably at **~160 nanoseconds**.

### 🌪️ Extreme Stress Testing Methodology
To verify the architecture's resilience against memory-bandwidth bottlenecks and L3 cache misses, the benchmark suite includes an adversarial stress test:
* **Massive Allocation:** Inflates cache capacity to **1,000,000** nodes.
* **Adversarial Chaos:** Uses a Pseudo-Random Number Generator (`std::mt19937_64`) to fire **50 Million** randomized `PUT` and `GET` requests across a huge domain, artificially forcing maximal hash table collisions and continuous backward-shifting evictions.

**Stress Test Results:** Under absolute maximum duress, the zero-allocation architecture maintained **2.24 Million Operations/sec** with a raw cache-access time of exactly **349 nanoseconds**.

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

## 🤝 License
This project is open-source and intended for educational and interview preparation purposes.
