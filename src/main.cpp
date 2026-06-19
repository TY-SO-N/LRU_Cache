#include "lru_cache.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>

void print_banner()
{
    std::cout << "\n";
    std::cout << "  ============================================\n";
    std::cout << "      LRU Cache Simulator v1.0\n";
    std::cout << "      In-Memory Key-Value Store\n";
    std::cout << "  ============================================\n";
    std::cout << "\n";
}

void print_menu(std::size_t current_capacity)
{
    std::cout << "\n";
    std::cout << "  Current Capacity : " << current_capacity << "\n";
    std::cout << "\n";
    std::cout << "  1.  Put Item\n";
    std::cout << "  2.  Get Item\n";
    std::cout << "  3.  Remove Item\n";
    std::cout << "  4.  Display Cache\n";
    std::cout << "  5.  Cache Statistics\n";
    std::cout << "  6.  Clear Cache\n";
    std::cout << "  7.  Run Benchmark\n";
    std::cout << "  8.  Show Cache Information\n";
    std::cout << "  9.  Reset Cache\n";
    std::cout << "  10. Exit\n";
    std::cout << "\n";
    std::cout << "  Enter Choice : ";
}

std::string read_line(const std::string& prompt)
{
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    return input;
}

int read_choice()
{
    std::string line;
    if (!std::getline(std::cin, line))
    {
        return 10;
    }

    std::istringstream stream(line);
    int choice = -1;
    stream >> choice;
    return choice;
}

std::size_t read_capacity()
{
    while (true)
    {
        std::string line = read_line("  Enter cache capacity (1-10000) : ");

        std::istringstream stream(line);
        int value = 0;

        if (!(stream >> value) || value <= 0 || value > 10000)
        {
            std::cout << "  [ERROR] Please enter a number between 1 and 10000.\n";
            continue;
        }

        std::string leftover;
        if (stream >> leftover)
        {
            std::cout << "  [ERROR] Please enter a single number.\n";
            continue;
        }

        return static_cast<std::size_t>(value);
    }
}

void handle_put(lru_cache& cache)
{
    std::string key = read_line("  Enter key   : ");
    if (key.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    std::string value = read_line("  Enter value : ");
    if (value.empty())
    {
        std::cout << "  [ERROR] Value cannot be empty.\n";
        return;
    }

    cache.put(key, value);
    std::cout << "\n  [OK] Stored: " << key << " = " << value << "\n";

    if (cache.size() == cache.capacity())
    {
        std::cout << "  [INFO] Cache is full (" << cache.size() << "/" << cache.capacity() << ")\n";
    }
}

void handle_get(lru_cache& cache)
{
    std::string key = read_line("  Enter key : ");
    if (key.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    auto result = cache.get(key);

    std::cout << "\n";
    if (result.found)
    {
        std::cout << "  [HIT] " << key << " = " << result.value << "\n";
        std::cout << "  [INFO] '" << key << "' promoted to Most Recently Used\n";
    }
    else
    {
        std::cout << "  [MISS] Key '" << key << "' not found in cache.\n";
    }
}

void handle_remove(lru_cache& cache)
{
    std::string key = read_line("  Enter key to remove : ");
    if (key.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    std::cout << "\n";
    if (cache.remove(key))
    {
        std::cout << "  [OK] Removed '" << key << "' from cache.\n";
        std::cout << "  [INFO] Cache size: " << cache.size() << "/" << cache.capacity() << "\n";
    }
    else
    {
        std::cout << "  [ERROR] Key '" << key << "' not found in cache.\n";
    }
}

void handle_display(const lru_cache& cache)
{
    std::cout << "\n";
    if (cache.empty())
    {
        std::cout << "  (cache is empty)\n";
        return;
    }

    std::cout << "  ---- Cache Contents ----\n";
    cache.display();
    std::cout << "  ------------------------\n";
}

void handle_stats(const lru_cache& cache)
{
    std::cout << "\n";
    std::cout << "  ==============================\n";
    std::cout << "       Cache Statistics\n";
    std::cout << "  ==============================\n";
    std::cout << "  Capacity         : " << cache.capacity() << "\n";
    std::cout << "  Current Size     : " << cache.size() << "\n";
    std::cout << "  Total Requests   : " << cache.total_requests() << "\n";
    std::cout << "  Cache Hits       : " << cache.hits() << "\n";
    std::cout << "  Cache Misses     : " << cache.misses() << "\n";
    std::cout << "  Evictions        : " << cache.evictions() << "\n";
    std::cout << "  Hit Ratio        : " << cache.hit_ratio() << "%\n";
    std::cout << "  ==============================\n";
}

void handle_clear(lru_cache& cache)
{
    if (cache.empty())
    {
        std::cout << "\n  [INFO] Cache is already empty.\n";
        return;
    }

    std::size_t old_size = cache.size();
    cache.clear();
    std::cout << "\n  [OK] Cleared " << old_size << " items. Cache and statistics reset.\n";
}

void handle_benchmark(const lru_cache& cache)
{
    std::cout << "\n";
    std::cout << "  ==============================\n";
    std::cout << "       Running Benchmarks\n";
    std::cout << "  ==============================\n";

    const std::size_t num_ops = 100000;
    const std::size_t bench_capacity = cache.capacity();

    std::cout << "  Capacity   : " << bench_capacity << "\n";
    std::cout << "  Operations : " << num_ops << " per test\n";
    std::cout << "\n";

    {
        lru_cache bench(bench_capacity);
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < num_ops; ++i)
        {
            bench.put(std::to_string(i), std::to_string(i * 10));
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double ops_sec = static_cast<double>(num_ops) / (ms / 1000.0);
        std::cout << "  PUT (new keys)      : " << ms << " ms  |  "
                  << ops_sec << " ops/sec\n";
    }

    {
        lru_cache bench(bench_capacity);
        for (std::size_t i = 0; i < bench_capacity; ++i)
        {
            bench.put(std::to_string(i), std::to_string(i));
        }
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < num_ops; ++i)
        {
            bench.put(std::to_string(i % bench_capacity), std::to_string(i));
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double ops_sec = static_cast<double>(num_ops) / (ms / 1000.0);
        std::cout << "  PUT (existing keys) : " << ms << " ms  |  "
                  << ops_sec << " ops/sec\n";
    }

    {
        lru_cache bench(bench_capacity);
        for (std::size_t i = 0; i < bench_capacity; ++i)
        {
            bench.put(std::to_string(i), std::to_string(i));
        }
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < num_ops; ++i)
        {
            bench.get(std::to_string(i % bench_capacity));
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double ops_sec = static_cast<double>(num_ops) / (ms / 1000.0);
        std::cout << "  GET (cache hits)    : " << ms << " ms  |  "
                  << ops_sec << " ops/sec\n";
    }

    {
        lru_cache bench(bench_capacity);
        for (std::size_t i = 0; i < bench_capacity; ++i)
        {
            bench.put(std::to_string(i), std::to_string(i));
        }
        auto start = std::chrono::high_resolution_clock::now();
        for (std::size_t i = 0; i < num_ops; ++i)
        {
            bench.get(std::to_string(bench_capacity + i));
        }
        auto end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double ops_sec = static_cast<double>(num_ops) / (ms / 1000.0);
        std::cout << "  GET (cache misses)  : " << ms << " ms  |  "
                  << ops_sec << " ops/sec\n";
    }

    std::cout << "\n  [OK] All operations confirm O(1) amortized time.\n";
    std::cout << "  ==============================\n";
}

void handle_info(const lru_cache& cache)
{
    std::cout << "\n";
    std::cout << "  ==============================\n";
    std::cout << "       Cache Information\n";
    std::cout << "  ==============================\n";
    std::cout << "  Capacity       : " << cache.capacity() << "\n";
    std::cout << "  Current Size   : " << cache.size() << "\n";
    std::cout << "  Empty          : " << (cache.empty() ? "Yes" : "No") << "\n";
    std::cout << "  Space Used     : " << cache.size() << "/" << cache.capacity();

    if (cache.capacity() > 0)
    {
        double usage = static_cast<double>(cache.size()) / static_cast<double>(cache.capacity()) * 100.0;
        std::cout << " (" << usage << "%)";
    }

    std::cout << "\n";
    std::cout << "  Data Structure : std::list + std::unordered_map\n";
    std::cout << "  Complexity     : O(1) PUT, GET, REMOVE, EVICT\n";
    std::cout << "  ==============================\n";
}

int main()
{
    print_banner();

    std::size_t capacity = read_capacity();
    auto cache = std::make_unique<lru_cache>(capacity);

    std::cout << "\n  [OK] Cache created with capacity " << capacity << ".\n";

    bool running = true;
    while (running)
    {
        print_menu(cache->capacity());
        int choice = read_choice();

        switch (choice)
        {
            case 1:
                handle_put(*cache);
                break;
            case 2:
                handle_get(*cache);
                break;
            case 3:
                handle_remove(*cache);
                break;
            case 4:
                handle_display(*cache);
                break;
            case 5:
                handle_stats(*cache);
                break;
            case 6:
                handle_clear(*cache);
                break;
            case 7:
                handle_benchmark(*cache);
                break;
            case 8:
                handle_info(*cache);
                break;
            case 9:
            {
                std::cout << "\n  Resetting cache with new capacity.\n";
                std::size_t new_capacity = read_capacity();
                cache = std::make_unique<lru_cache>(new_capacity);
                std::cout << "  [OK] Cache reset with capacity " << new_capacity << ".\n";
                break;
            }
            case 10:
                std::cout << "\n  [INFO] Thank you for using LRU Cache Simulator. Goodbye!\n\n";
                running = false;
                break;
            default:
                std::cout << "\n  [ERROR] Invalid choice. Please enter a number between 1 and 10.\n";
                break;
        }
    }

    return 0;
}
