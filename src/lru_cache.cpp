#include "lru_cache.h"

#include <iostream>
#include <stdexcept>

lru_cache::lru_cache(std::size_t capacity)
    : capacity_{capacity}
{
    if (capacity_ == 0)
    {
        throw std::invalid_argument("cache capacity must be greater than zero");
    }
}

void lru_cache::put(const key_type& key, const value_type& value)
{
    auto it = lookup_.find(key);

    if (it != lookup_.end())
    {
        it->second->second = value;
        promote(it);
        return;
    }

    if (order_.size() == capacity_)
    {
        evict_lru();
    }

    order_.emplace_front(key, value);
    lookup_[key] = order_.begin();
}

get_result lru_cache::get(const key_type& key)
{
    auto it = lookup_.find(key);

    if (it == lookup_.end())
    {
        ++misses_;
        return {false, {}};
    }

    ++hits_;
    promote(it);
    return {true, it->second->second};
}

bool lru_cache::remove(const key_type& key)
{
    auto it = lookup_.find(key);

    if (it == lookup_.end())
    {
        return false;
    }

    order_.erase(it->second);
    lookup_.erase(it);
    return true;
}

void lru_cache::clear()
{
    order_.clear();
    lookup_.clear();
    hits_ = 0;
    misses_ = 0;
    evictions_ = 0;
}

void lru_cache::display() const
{
    if (order_.empty())
    {
        std::cout << "(cache is empty)\n";
        return;
    }

    std::cout << "Cache (" << order_.size() << "/" << capacity_ << ") [MRU -> LRU]:\n";

    std::size_t index = 1;
    for (const auto& entry : order_)
    {
        std::cout << "  " << index << ". " << entry.first << " = " << entry.second << "\n";
        ++index;
    }
}

std::size_t lru_cache::size() const
{
    return order_.size();
}

std::size_t lru_cache::capacity() const
{
    return capacity_;
}

bool lru_cache::empty() const
{
    return order_.empty();
}

std::size_t lru_cache::hits() const
{
    return hits_;
}

std::size_t lru_cache::misses() const
{
    return misses_;
}

std::size_t lru_cache::evictions() const
{
    return evictions_;
}

std::size_t lru_cache::total_requests() const
{
    return hits_ + misses_;
}

double lru_cache::hit_ratio() const
{
    const auto total = total_requests();
    if (total == 0)
    {
        return 0.0;
    }
    return static_cast<double>(hits_) / static_cast<double>(total) * 100.0;
}

void lru_cache::promote(map_type::iterator it)
{
    order_.splice(order_.begin(), order_, it->second);
}

void lru_cache::evict_lru()
{
    const auto& lru_key = order_.back().first;
    lookup_.erase(lru_key);
    order_.pop_back();
    ++evictions_;
}
