#include "lru_cache.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

// Utility to find next power of two for the hash mask
static std::size_t next_power_of_two(std::size_t n) {
    std::size_t count = 1;
    while (count < n) count <<= 1;
    return count;
}

lru_cache::lru_cache(std::size_t capacity)
    : capacity_{capacity}, size_{0}, head_{-1}, tail_{-1}, free_head_{0}
{
    if (capacity_ == 0) {
        throw std::invalid_argument("cache capacity must be greater than zero");
    }

    // Preallocate all memory pool nodes up front. Zero runtime allocations.
    nodes_.resize(capacity_);
    for (int32_t i = 0; i < static_cast<int32_t>(capacity_); ++i) {
        nodes_[i].next = (i + 1 < static_cast<int32_t>(capacity_)) ? i + 1 : -1;
    }

    // Initialize open addressing hash table with power of 2 size.
    // We use a load factor of 0.5 (capacity * 2) to minimize probe lengths and prevent rehashes.
    std::size_t table_size = next_power_of_two(capacity_ * 2);
    hash_table_.assign(table_size, -1);
    hash_mask_ = table_size - 1;
}

std::size_t lru_cache::hash(key_type key) const {
    // Fast integer hash (splitmix64 style) to avoid clustering in the open addressing table
    uint64_t z = (key ^ (key >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return (z ^ (z >> 31)) & hash_mask_;
}

int32_t lru_cache::find_node(key_type key) const {
    std::size_t idx = hash(key);
    while (true) {
        int32_t node_idx = hash_table_[idx];
        if (node_idx == -1) {
            return -1; // Empty slot found, key does not exist
        }
        if (nodes_[node_idx].key == key) {
            return node_idx; // Exact match found
        }
        idx = (idx + 1) & hash_mask_; // Linear probing
    }
}

void lru_cache::put(key_type key, value_type value) {
    std::size_t idx = hash(key);
    std::size_t insert_idx = idx;
    
    // 1. Probing loop to find existing key or an empty slot
    while (true) {
        int32_t node_idx = hash_table_[insert_idx];
        if (node_idx == -1) {
            break; // Empty slot found
        }
        if (nodes_[node_idx].key == key) {
            // Key exists. Update value and promote to MRU.
            nodes_[node_idx].value = value;
            promote(node_idx);
            return;
        }
        insert_idx = (insert_idx + 1) & hash_mask_;
    }

    // 2. Key does not exist. Check capacity.
    if (size_ == capacity_) {
        evict_lru();
        
        // Eviction modifies the hash table (removes an entry).
        // This means our previously found `insert_idx` might no longer be optimal 
        // or might have shifted. Re-probe to find the new ideal empty slot.
        insert_idx = hash(key);
        while (hash_table_[insert_idx] != -1) {
            insert_idx = (insert_idx + 1) & hash_mask_;
        }
    }

    // 3. Allocate a pre-allocated node from the free list
    int32_t new_node_idx = allocate_node();
    nodes_[new_node_idx].key = key;
    nodes_[new_node_idx].value = value;
    
    // 4. Insert into hash table and link as MRU
    hash_table_[insert_idx] = new_node_idx;
    link_node(new_node_idx);
}

get_result lru_cache::get(key_type key) {
    int32_t node_idx = find_node(key);
    if (node_idx == -1) {
        ++misses_;
        return {false, 0};
    }
    
    ++hits_;
    promote(node_idx); // Move to MRU in O(1) array pointer swaps
    return {true, nodes_[node_idx].value};
}

bool lru_cache::remove(key_type key) {
    std::size_t idx = hash(key);
    while (true) {
        int32_t node_idx = hash_table_[idx];
        if (node_idx == -1) {
            return false; // Not found
        }
        if (nodes_[node_idx].key == key) {
            // Found the node. Unlink from list and return to free pool.
            unlink_node(node_idx);
            free_node(node_idx);
            
            // Open addressing removal with backward shifting to maintain dense table.
            // Avoids complex tombstones and preserves O(1) performance.
            hash_table_[idx] = -1;
            std::size_t next_idx = (idx + 1) & hash_mask_;
            while (hash_table_[next_idx] != -1) {
                int32_t move_node_idx = hash_table_[next_idx];
                std::size_t ideal_idx = hash(nodes_[move_node_idx].key);
                
                // Determine if we can safely slide the entry backwards to fill the hole
                bool can_move = false;
                if (ideal_idx <= next_idx) {
                    can_move = (idx >= ideal_idx && idx < next_idx);
                } else {
                    can_move = (idx >= ideal_idx || idx < next_idx);
                }
                
                if (can_move) {
                    hash_table_[idx] = move_node_idx;
                    hash_table_[next_idx] = -1;
                    idx = next_idx; // Move the hole forward
                }
                next_idx = (next_idx + 1) & hash_mask_;
            }
            return true;
        }
        idx = (idx + 1) & hash_mask_;
    }
}

void lru_cache::evict_lru() {
    int32_t evict_idx = tail_; // LRU element
    
    // 1. Remove from hash table
    std::size_t idx = hash(nodes_[evict_idx].key);
    while (true) {
        int32_t node_idx = hash_table_[idx];
        if (node_idx == evict_idx) {
            // Found it. Erase and shift exactly like remove().
            hash_table_[idx] = -1;
            std::size_t next_idx = (idx + 1) & hash_mask_;
            while (hash_table_[next_idx] != -1) {
                int32_t move_node_idx = hash_table_[next_idx];
                std::size_t ideal_idx = hash(nodes_[move_node_idx].key);
                bool can_move = false;
                if (ideal_idx <= next_idx) {
                    can_move = (idx >= ideal_idx && idx < next_idx);
                } else {
                    can_move = (idx >= ideal_idx || idx < next_idx);
                }
                if (can_move) {
                    hash_table_[idx] = move_node_idx;
                    hash_table_[next_idx] = -1;
                    idx = next_idx;
                }
                next_idx = (next_idx + 1) & hash_mask_;
            }
            break;
        }
        idx = (idx + 1) & hash_mask_;
    }
    
    // 2. Unlink from array list and free
    unlink_node(evict_idx);
    free_node(evict_idx);
    ++evictions_;
}

void lru_cache::promote(int32_t node_idx) {
    if (node_idx == head_) return; // Already MRU
    unlink_node(node_idx);
    link_node(node_idx);
}

int32_t lru_cache::allocate_node() {
    // O(1) allocation from free list
    int32_t idx = free_head_;
    free_head_ = nodes_[idx].next;
    ++size_;
    return idx;
}

void lru_cache::free_node(int32_t node_idx) {
    // O(1) deallocation to free list
    nodes_[node_idx].next = free_head_;
    free_head_ = node_idx;
    --size_;
}

void lru_cache::link_node(int32_t node_idx) {
    // Links node to the front (MRU) of the list
    nodes_[node_idx].next = head_;
    nodes_[node_idx].prev = -1;
    
    if (head_ != -1) {
        nodes_[head_].prev = node_idx;
    }
    head_ = node_idx;
    
    if (tail_ == -1) {
        tail_ = node_idx;
    }
}

void lru_cache::unlink_node(int32_t node_idx) {
    // Removes node from its current position in the list
    int32_t p = nodes_[node_idx].prev;
    int32_t n = nodes_[node_idx].next;
    
    if (p != -1) nodes_[p].next = n;
    else head_ = n; // It was head
    
    if (n != -1) nodes_[n].prev = p;
    else tail_ = p; // It was tail
}

void lru_cache::clear() {
    size_ = 0;
    head_ = -1;
    tail_ = -1;
    free_head_ = 0;
    
    // Re-link the free list
    for (int32_t i = 0; i < static_cast<int32_t>(capacity_); ++i) {
        nodes_[i].next = (i + 1 < static_cast<int32_t>(capacity_)) ? i + 1 : -1;
    }
    
    // Clear hash table
    std::fill(hash_table_.begin(), hash_table_.end(), -1);
    
    hits_ = 0;
    misses_ = 0;
    evictions_ = 0;
}

void lru_cache::display(std::function<std::string(uint64_t)> formatter) const {
    if (size_ == 0) {
        std::cout << "(cache is empty)\n";
        return;
    }
    std::cout << "Cache (" << size_ << "/" << capacity_ << ") [MRU -> LRU]:\n";
    int32_t curr = head_;
    std::size_t index = 1;
    while (curr != -1) {
        if (formatter) {
            std::cout << "  " << index << ". " << formatter(nodes_[curr].key) << " = " << formatter(nodes_[curr].value) << "\n";
        } else {
            std::cout << "  " << index << ". " << nodes_[curr].key << " = " << nodes_[curr].value << "\n";
        }
        curr = nodes_[curr].next;
        ++index;
    }
}

std::size_t lru_cache::size() const { return size_; }
std::size_t lru_cache::capacity() const { return capacity_; }
bool lru_cache::empty() const { return size_ == 0; }
std::size_t lru_cache::hits() const { return hits_; }
std::size_t lru_cache::misses() const { return misses_; }
std::size_t lru_cache::evictions() const { return evictions_; }
std::size_t lru_cache::total_requests() const { return hits_ + misses_; }

double lru_cache::hit_ratio() const {
    const auto total = total_requests();
    if (total == 0) return 0.0;
    return static_cast<double>(hits_) / static_cast<double>(total) * 100.0;
}
