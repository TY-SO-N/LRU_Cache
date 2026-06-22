#pragma once

#include "lru_cache.h"
#include "spsc_queue.h"
#include <atomic>

#if defined(_WIN32)
#include <windows.h>
#include <thread>
#else
#include <thread>
#include <pthread.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
  #if defined(__i386__) || defined(__x86_64__)
    #include <immintrin.h>
    #define CPU_PAUSE() _mm_pause()
  #else
    #define CPU_PAUSE()
  #endif
#elif defined(_MSC_VER) || defined(_WIN32)
  #include <windows.h>
  #define CPU_PAUSE() YieldProcessor()
#else
  #define CPU_PAUSE()
#endif

enum class req_type {
    PUT,
    GET
};

struct cache_request {
    uint64_t req_id;
    req_type type;
    uint64_t key;
    uint64_t value; // Only used for PUT
};

struct cache_response {
    uint64_t req_id;
    bool found;
    uint64_t value; 
};

// Wrapper that runs the LRU Cache on an isolated background thread
class async_cache {
private:
    lru_cache cache_;
    spsc_queue<cache_request> inbox_;
    spsc_queue<cache_response> outbox_;
    std::atomic<bool> running_{true};
    int core_id_;

#if defined(_WIN32)
    HANDLE worker_{nullptr};
    static DWORD WINAPI ThreadFunc(LPVOID lpParam) {
        auto* cache = static_cast<async_cache*>(lpParam);
        
        // Pin worker thread to the specified core to maximize L1/L2 cache locality
        if (cache->core_id_ >= 0) {
            DWORD_PTR mask = (static_cast<DWORD_PTR>(1) << cache->core_id_);
            SetThreadAffinityMask(GetCurrentThread(), mask);
        }
        
        cache->worker_loop();
        return 0;
    }
#else
    std::thread worker_;
    static void set_thread_affinity(std::thread& th, int core_id) {
        if (core_id < 0) return;
#if defined(__linux__)
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);
        pthread_setaffinity_np(th.native_handle(), sizeof(cpu_set_t), &cpuset);
#endif
    }
#endif

    void worker_loop() {
        while (running_.load(std::memory_order_relaxed)) {
            cache_request req;
            if (inbox_.pop(req)) {
                if (req.type == req_type::PUT) {
                    cache_.put(req.key, req.value);
                } else if (req.type == req_type::GET) {
                    auto res = cache_.get(req.key);
                    cache_response out_res{req.req_id, res.found, res.value};
                    // Spin until there is room in the outbox
                    while (!outbox_.push(out_res) && running_.load(std::memory_order_relaxed)) {
                        CPU_PAUSE();
                    }
                }
            } else {
                // Yield CPU when idle to prevent 100% core usage and melting
                CPU_PAUSE();
            }
        }
        
        // Drain inbox on shutdown (Flush pending PUTs, discard GETs)
        cache_request req;
        while (inbox_.pop(req)) {
            if (req.type == req_type::PUT) {
                cache_.put(req.key, req.value);
            }
            // Discard GET requests because the main thread is blocked in ~async_cache
            // waiting for this thread to join. Attempting to push to the outbox here 
            // will cause an infinite deadlock if the outbox becomes full.
        }
    }

public:
    async_cache(std::size_t cache_capacity, std::size_t queue_capacity, int core_id = -1)
        : cache_(cache_capacity), inbox_(queue_capacity), outbox_(queue_capacity) {
        
        if (core_id == -1) {
#if defined(_WIN32)
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            unsigned int hardware_cores = sysinfo.dwNumberOfProcessors;
#else
            unsigned int hardware_cores = std::thread::hardware_concurrency();
#endif
            core_id_ = (hardware_cores > 0) ? (hardware_cores - 1) : 0;
        } else {
            core_id_ = core_id;
        }

#if defined(_WIN32)
        worker_ = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
#else
        worker_ = std::thread(&async_cache::worker_loop, this);
        set_thread_affinity(worker_, core_id_);
#endif
    }

    ~async_cache() {
        running_.store(false, std::memory_order_relaxed);
#if defined(_WIN32)
        if (worker_) {
            WaitForSingleObject(worker_, INFINITE);
            CloseHandle(worker_);
        }
#else
        if (worker_.joinable()) {
            worker_.join();
        }
#endif
    }

    // Producer API (Worker Thread)
    void async_put(uint64_t key, uint64_t value) {
        cache_request req{0, req_type::PUT, key, value};
        // Busy wait (spin) until space opens up in the lock-free queue
        while (!inbox_.push(req)) {
            CPU_PAUSE();
        }
    }

    void async_get(uint64_t req_id, uint64_t key) {
        cache_request req{req_id, req_type::GET, key, 0};
        while (!inbox_.push(req)) {
            CPU_PAUSE();
        }
    }

    bool poll_response(cache_response& res) {
        return outbox_.pop(res);
    }
};
