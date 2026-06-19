#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <functional>
#include <string>

struct get_result
{
    bool found;
    uint64_t value;
};

class lru_cache
{
public:
    using key_type = uint64_t;
    using value_type = uint64_t;

    explicit lru_cache(std::size_t capacity);

    void put(key_type key, value_type value);

    get_result get(key_type key);

    bool remove(key_type key);

    void clear();

    void display(std::function<std::string(uint64_t)> formatter = nullptr) const;

    std::size_t size() const;

    std::size_t capacity() const;

    bool empty() const;

    std::size_t hits() const;
    std::size_t misses() const;
    std::size_t evictions() const;
    std::size_t total_requests() const;
    double hit_ratio() const;

private:
    // 32-byte aligned node fits two perfectly inside a 64-byte cache line
    struct alignas(32) Node {
        key_type key;
        value_type value;
        int32_t prev;
        int32_t next;
    };

    std::size_t capacity_;
    std::size_t size_;

    // Array-based doubly linked list acting as a Memory Pool
    std::vector<Node> nodes_;
    int32_t head_; // MRU index
    int32_t tail_; // LRU index
    int32_t free_head_; // Free list index

    // Open addressing flat hash map (maps hash -> node index)
    std::vector<int32_t> hash_table_;
    std::size_t hash_mask_;

    // Performance Counters
    std::size_t hits_ = 0;
    std::size_t misses_ = 0;
    std::size_t evictions_ = 0;

    // Internal HFT primitives
    int32_t find_node(key_type key) const;
    void promote(int32_t node_idx);
    void evict_lru();
    int32_t allocate_node();
    void free_node(int32_t node_idx);
    void link_node(int32_t node_idx);
    void unlink_node(int32_t node_idx);
    std::size_t hash(key_type key) const;
};
