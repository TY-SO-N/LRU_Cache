#include "lru_cache.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <random>

struct bench_result
{
    std::string name;
    std::size_t operations;
    double total_ms;
    double ops_per_sec;
    double avg_ns;
    double p50_ns;
    double p95_ns;
    double p99_ns;
};

// Calculate percentiles
void calculate_percentiles(std::vector<double>& latencies, bench_result& r) {
    std::sort(latencies.begin(), latencies.end());
    r.p50_ns = latencies[latencies.size() * 0.50];
    r.p95_ns = latencies[latencies.size() * 0.95];
    r.p99_ns = latencies[latencies.size() * 0.99];
}

bench_result benchmark_put_new_keys(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);
    
    auto global_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        cache.put(i, i * 10);
        auto end = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "PUT (new keys, eviction)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops),
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

bench_result benchmark_put_existing_keys(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);

    for (std::size_t i = 0; i < cache_capacity; ++i) {
        cache.put(i, i);
    }

    auto global_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        std::size_t key_idx = i % cache_capacity;
        auto start = std::chrono::high_resolution_clock::now();
        cache.put(key_idx, i);
        auto end = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "PUT (update, promote)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops),
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

bench_result benchmark_get_hit(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);

    for (std::size_t i = 0; i < cache_capacity; ++i) {
        cache.put(i, i * 10);
    }

    auto global_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        std::size_t key_idx = i % cache_capacity;
        auto start = std::chrono::high_resolution_clock::now();
        cache.get(key_idx);
        auto end = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "GET (hit, promote)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops),
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

bench_result benchmark_get_miss(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);

    for (std::size_t i = 0; i < cache_capacity; ++i) {
        cache.put(i, i);
    }

    auto global_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        cache.get(cache_capacity + i);
        auto end = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "GET (miss)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops),
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

bench_result benchmark_mixed_workload(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);

    auto global_start = std::chrono::high_resolution_clock::now();

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        if (i % 3 == 0) {
            cache.put(i, i);
        } else {
            cache.get(i / 2);
        }
        auto end = std::chrono::high_resolution_clock::now();
        latencies[i] = std::chrono::duration<double, std::nano>(end - start).count();
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "MIXED (33% put, 67% get)",
        num_ops,
        ms,
        static_cast<double>(num_ops) / (ms / 1000.0),
        (ms * 1e6) / static_cast<double>(num_ops),
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

bench_result benchmark_extreme_stress_test(std::size_t cache_capacity, std::size_t num_ops)
{
    lru_cache cache(cache_capacity);
    std::vector<double> latencies(num_ops);

    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<uint64_t> dist(0, cache_capacity * 10);
    std::uniform_int_distribution<int> op_dist(0, 100);

    for (std::size_t i = 0; i < cache_capacity; ++i) {
        cache.put(i, i);
    }

    auto global_start = std::chrono::high_resolution_clock::now();
    double total_op_time_ns = 0;

    for (std::size_t i = 0; i < num_ops; ++i)
    {
        uint64_t key = dist(rng);
        int op = op_dist(rng);

        auto start = std::chrono::high_resolution_clock::now();
        if (op < 50) {
            cache.put(key, key);
        } else {
            cache.get(key);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        double op_ns = std::chrono::duration<double, std::nano>(end - start).count();
        latencies[i] = op_ns;
        total_op_time_ns += op_ns;
    }

    auto global_end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(global_end - global_start).count();

    bench_result r{
        "EXTREME STRESS TEST",
        num_ops,
        ms, // Total time includes PRNG overhead
        static_cast<double>(num_ops) / (ms / 1000.0), // Ops/sec includes overhead
        total_op_time_ns / static_cast<double>(num_ops), // Pure average latency without overhead
        0, 0, 0
    };
    calculate_percentiles(latencies, r);
    return r;
}

void print_separator()
{
    std::cout << "  +" << std::string(26, '-')
              << "+" << std::string(12, '-')
              << "+" << std::string(12, '-')
              << "+" << std::string(12, '-')
              << "+" << std::string(10, '-')
              << "+" << std::string(10, '-')
              << "+" << std::string(10, '-')
              << "+" << std::string(10, '-')
              << "+\n";
}

void print_header()
{
    print_separator();
    std::cout << "  | " << std::left << std::setw(24) << "Benchmark"
              << " | " << std::right << std::setw(10) << "Operations"
              << " | " << std::setw(10) << "Total (ms)"
              << " | " << std::setw(10) << "Ops/sec"
              << " | " << std::setw(8) << "Avg(ns)"
              << " | " << std::setw(8) << "P50(ns)"
              << " | " << std::setw(8) << "P95(ns)"
              << " | " << std::setw(8) << "P99(ns)"
              << " |\n";
    print_separator();
}

void print_result(const bench_result& r)
{
    std::cout << "  | " << std::left << std::setw(24) << r.name
              << " | " << std::right << std::setw(10) << r.operations
              << " | " << std::setw(10) << std::fixed << std::setprecision(1) << r.total_ms
              << " | " << std::setw(10) << std::fixed << std::setprecision(0) << r.ops_per_sec
              << " | " << std::setw(8) << std::fixed << std::setprecision(0) << r.avg_ns
              << " | " << std::setw(8) << std::fixed << std::setprecision(0) << r.p50_ns
              << " | " << std::setw(8) << std::fixed << std::setprecision(0) << r.p95_ns
              << " | " << std::setw(8) << std::fixed << std::setprecision(0) << r.p99_ns
              << " |\n";
}

int main()
{
    const std::size_t cache_capacity = 1000;
    const std::size_t num_ops = 500000;

    std::cout << "\n";
    std::cout << "  ====================================================================================================\n";
    std::cout << "     LRU Cache Benchmark Suite\n";
    std::cout << "  ====================================================================================================\n";
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

    std::cout << "\n  Executing Extreme Stress Test... (This may take a moment)\n";
    const std::size_t extreme_capacity = 1000000; // 1 Million capacity
    const std::size_t extreme_ops = 50000000;     // 50 Million ops
    print_separator();
    print_result(benchmark_extreme_stress_test(extreme_capacity, extreme_ops));
    print_separator();

    return 0;
}
