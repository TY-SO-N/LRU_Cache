#include "lru_cache.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

struct bench_result
{
    std::string name;
    std::size_t operations;
    double total_ms;
    double ops_per_sec;
    double ns_per_op;
};

bench_result benchmark_put_new_keys(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);

    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        cache.put(std::to_string(i), std::to_string(i * 10));
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        "PUT (new keys, triggers eviction)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops)
    };
}

bench_result benchmark_put_existing_keys(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);

    for (std::size_t i = 0; i < cache_capacity; ++i)
    {
        cache.put(std::to_string(i), std::to_string(i));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        std::size_t key_idx = i % cache_capacity;
        cache.put(std::to_string(key_idx), std::to_string(i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        "PUT (existing keys, update + promote)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops)
    };
}

bench_result benchmark_get_hit(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);

    for (std::size_t i = 0; i < cache_capacity; ++i)
    {
        cache.put(std::to_string(i), std::to_string(i * 10));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        std::size_t key_idx = i % cache_capacity;
        cache.get(std::to_string(key_idx));
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        "GET (cache hit, lookup + promote)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops)
    };
}

bench_result benchmark_get_miss(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);

    for (std::size_t i = 0; i < cache_capacity; ++i)
    {
        cache.put(std::to_string(i), std::to_string(i));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        cache.get(std::to_string(cache_capacity + i));
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        "GET (cache miss, lookup only)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops)
    };
}

bench_result benchmark_mixed_workload(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);

    auto start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        if (i % 3 == 0)
        {
            cache.put(std::to_string(i), std::to_string(i));
        }
        else
        {
            cache.get(std::to_string(i / 2));
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return {
        "MIXED (33% put, 67% get)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops)
    };
}

void print_separator()
{
    std::cout << "  +" << std::string(42, '-')
              << "+" << std::string(14, '-')
              << "+" << std::string(14, '-')
              << "+" << std::string(16, '-')
              << "+" << std::string(14, '-')
              << "+\n";
}

void print_header()
{
    print_separator();
    std::cout << "  | " << std::left << std::setw(40) << "Benchmark"
              << " | " << std::right << std::setw(12) << "Operations"
              << " | " << std::setw(12) << "Total (ms)"
              << " | " << std::setw(14) << "Ops/sec"
              << " | " << std::setw(12) << "ns/op"
              << " |\n";
    print_separator();
}

void print_result(const bench_result& r)
{
    std::cout << "  | " << std::left << std::setw(40) << r.name
              << " | " << std::right << std::setw(12) << r.operations
              << " | " << std::setw(12) << std::fixed << std::setprecision(2) << r.total_ms
              << " | " << std::setw(14) << std::fixed << std::setprecision(0) << r.ops_per_sec
              << " | " << std::setw(12) << std::fixed << std::setprecision(1) << r.ns_per_op
              << " |\n";
}

int main()
{
    const std::size_t cache_capacity = 1000;
    const std::size_t num_ops = 500000;

    std::cout << "\n";
    std::cout << "  ==========================================\n";
    std::cout << "     LRU Cache Benchmark Suite\n";
    std::cout << "  ==========================================\n";
    std::cout << "\n";
    std::cout << "  Cache capacity : " << cache_capacity << "\n";
    std::cout << "  Operations     : " << num_ops << " per benchmark\n";
    std::cout << "\n";

    std::vector<bench_result> results;
    results.push_back(benchmark_put_new_keys(cache_capacity, num_ops));
    results.push_back(benchmark_put_existing_keys(cache_capacity, num_ops));
    results.push_back(benchmark_get_hit(cache_capacity, num_ops));
    results.push_back(benchmark_get_miss(cache_capacity, num_ops));
    results.push_back(benchmark_mixed_workload(cache_capacity, num_ops));

    print_header();
    for (const auto& r : results)
    {
        print_result(r);
    }
    print_separator();

    std::cout << "\n  Notes:\n";
    std::cout << "    - All operations are amortized O(1)\n";
    std::cout << "    - 'PUT new keys' includes eviction overhead\n";
    std::cout << "    - 'ns/op' = nanoseconds per operation (lower is better)\n";
    std::cout << "    - 'Ops/sec' = operations per second (higher is better)\n";
    std::cout << "\n";

    return 0;
}
