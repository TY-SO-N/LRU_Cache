#pragma once

#include <atomic>
#include <vector>
#include <cstddef>
#include <stdexcept>

// A Lock-Free, Single-Producer Single-Consumer (SPSC) Ring Buffer
template<typename T>
class spsc_queue {
private:
    std::vector<T> buffer_;
    const std::size_t capacity_;
    
    // Pad atomic counters to 64 bytes to prevent "False Sharing" (cache line bouncing)
    // between the producer core and consumer core.
    alignas(64) std::atomic<std::size_t> head_{0};
    alignas(64) std::atomic<std::size_t> tail_{0};

public:
    explicit spsc_queue(std::size_t capacity) 
        : buffer_(capacity + 1), capacity_(capacity + 1) {
        if (capacity == 0) {
            throw std::invalid_argument("Queue capacity must be > 0");
        }
    }

    // Called by Producer thread only
    bool push(const T& item) {
        const std::size_t current_tail = tail_.load(std::memory_order_relaxed);
        const std::size_t next_tail = (current_tail + 1) % capacity_;

        // Acquire barrier ensures we see the latest consumer head updates
        if (next_tail == head_.load(std::memory_order_acquire)) {
            return false; // Queue is full
        }

        buffer_[current_tail] = item;
        // Release barrier ensures item is written to memory before tail is updated
        tail_.store(next_tail, std::memory_order_release);
        return true;
    }

    // Called by Consumer thread only
    bool pop(T& item) {
        const std::size_t current_head = head_.load(std::memory_order_relaxed);

        // Acquire barrier ensures we see the latest producer tail updates
        if (current_head == tail_.load(std::memory_order_acquire)) {
            return false; // Queue is empty
        }

        item = buffer_[current_head];
        // Release barrier ensures item is read before head is updated
        head_.store((current_head + 1) % capacity_, std::memory_order_release);
        return true;
    }
};
