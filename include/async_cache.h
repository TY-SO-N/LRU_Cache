#pragma once

#include "lru_cache.h"
#include "spsc_queue.h"
#include <atomic>

#if defined(_WIN32)
#include <windows.h>
#else
#include <thread>
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
#if defined(_WIN32)
    HANDLE worker_{nullptr};
    static DWORD WINAPI ThreadFunc(LPVOID lpParam) {
        auto* cache = static_cast<async_cache*>(lpParam);
        cache->worker_loop();
        return 0;
    }
#else
    std::thread worker_;
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
                    }
                }
            }
        }
        
        // Drain inbox on shutdown
        cache_request req;
        while (inbox_.pop(req)) {
            if (req.type == req_type::PUT) {
                cache_.put(req.key, req.value);
            } else if (req.type == req_type::GET) {
                auto res = cache_.get(req.key);
                cache_response out_res{req.req_id, res.found, res.value};
                while (!outbox_.push(out_res)) {}
            }
        }
    }

public:
    async_cache(std::size_t cache_capacity, std::size_t queue_capacity)
        : cache_(cache_capacity), inbox_(queue_capacity), outbox_(queue_capacity) {
#if defined(_WIN32)
        worker_ = CreateThread(NULL, 0, ThreadFunc, this, 0, NULL);
#else
        worker_ = std::thread(&async_cache::worker_loop, this);
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
        while (!inbox_.push(req)) {}
    }

    void async_get(uint64_t req_id, uint64_t key) {
        cache_request req{req_id, req_type::GET, key, 0};
        while (!inbox_.push(req)) {}
    }

    bool poll_response(cache_response& res) {
        return outbox_.pop(res);
    }
};
