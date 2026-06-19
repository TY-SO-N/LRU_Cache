#include "async_cache.h"
#include <chrono>
#include <iostream>
#include <iomanip>

void run_async_benchmark() {
    const std::size_t cache_capacity = 10000;
    const std::size_t queue_capacity = 8000000; // Large enough to prevent deadlock
    const std::size_t num_ops = 5000000;        // 5 Million ops

    std::cout << "\n  ====================================================================================================\n";
    std::cout << "     Lock-Free Asynchronous Cache Benchmark\n";
    std::cout << "  ====================================================================================================\n\n";

    std::cout << "  Spawning dedicated isolated cache thread...\n";
    async_cache async_lru(cache_capacity, queue_capacity);

    std::cout << "  Warming up cache...\n";
    for (std::size_t i = 0; i < cache_capacity; ++i) {
        async_lru.async_put(i, i * 10);
    }
    
    std::cout << "  Blasting " << num_ops << " cross-thread GET requests...\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    // Fire and forget (Producer blasting requests)
    for (std::size_t i = 0; i < num_ops; ++i) {
        async_lru.async_get(i, i % cache_capacity);
    }

    // Now wait to receive all 5 million responses back through the Outbox
    std::size_t received = 0;
    cache_response res;
    while (received < num_ops) {
        if (async_lru.poll_response(res)) {
            received++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    double ops_sec = static_cast<double>(num_ops) / (ms / 1000.0);

    std::cout << "\n  +" << std::string(40, '-') << "+\n";
    std::cout << "  | " << std::left << std::setw(15) << "Total Time" 
              << "| " << std::fixed << std::setprecision(2) << ms << " ms\n";
    std::cout << "  | " << std::left << std::setw(15) << "Throughput" 
              << "| " << std::fixed << std::setprecision(0) << ops_sec << " ops/sec\n";
    std::cout << "  +" << std::string(40, '-') << "+\n\n";
}

int main() {
    run_async_benchmark();
    return 0;
}
