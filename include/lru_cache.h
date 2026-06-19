#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

struct get_result
{
    bool found;
    std::string value;
};

class lru_cache
{
public:
    using key_type = std::string;
    using value_type = std::string;
    using entry_type = std::pair<key_type, value_type>;
    using list_type = std::list<entry_type>;
    using map_type = std::unordered_map<key_type, list_type::iterator>;

    explicit lru_cache(std::size_t capacity);

    void put(const key_type& key, const value_type& value);

    get_result get(const key_type& key);

    bool remove(const key_type& key);

    void clear();

    void display() const;

    std::size_t size() const;

    std::size_t capacity() const;

    bool empty() const;

    std::size_t hits() const;
    std::size_t misses() const;
    std::size_t evictions() const;
    std::size_t total_requests() const;
    double hit_ratio() const;

private:
    std::size_t capacity_;
    list_type order_;
    map_type lookup_;

    std::size_t hits_ = 0;
    std::size_t misses_ = 0;
    std::size_t evictions_ = 0;

    void promote(map_type::iterator it);
    void evict_lru();
};
