#include "lru_cache.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <cctype>

// Global registry to map uint64_t hashes back to their original strings for UI display
std::unordered_map<uint64_t, std::string> g_string_registry;

// Trim function to clean accidental whitespaces from user input
void trim_string(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// Conversion Layer: Convert string inputs to uint64_t to maintain zero-alloc core
uint64_t to_uint64(const std::string& str)
{
    try {
        uint64_t val = std::stoull(str);
        g_string_registry[val] = str;
        return val;
    } catch (...) {
        uint64_t hash_val = std::hash<std::string>{}(str);
        g_string_registry[hash_val] = str;
        return hash_val;
    }
}

std::string from_uint64(uint64_t val)
{
    auto it = g_string_registry.find(val);
    if (it != g_string_registry.end()) {
        return it->second;
    }
    return std::to_string(val);
}


void print_menu(std::size_t current_capacity)
{
    std::cout << "\n";
    std::cout << "  Current Capacity : " << current_capacity << "\n";
    std::cout << "\n";
    std::cout << "  1.  Put Item\n";
    std::cout << "  2.  Get Item\n";
    std::cout << "  3.  Remove Item\n";
    std::cout << "  4.  Display Cache\n";
    std::cout << "  5.  Cache Statistics\n";
    std::cout << "  6.  Clear Cache\n";
    std::cout << "  7.  Show Cache Information\n";
    std::cout << "  8.  Reset Cache\n";
    std::cout << "  9.  Exit\n";
    std::cout << "\n";
    std::cout << "  Enter Choice : ";
}

std::string read_line(const std::string& prompt)
{
    std::cout << prompt;
    std::string input;
    std::getline(std::cin, input);
    trim_string(input);
    return input;
}

int read_choice()
{
    std::string line;
    if (!std::getline(std::cin, line))
    {
        return 9;
    }

    std::istringstream stream(line);
    int choice = -1;
    stream >> choice;
    return choice;
}

std::size_t read_capacity()
{
    while (true)
    {
        std::string line = read_line("  Enter cache capacity (1-10000) : ");

        std::istringstream stream(line);
        int value = 0;

        if (!(stream >> value) || value <= 0 || value > 10000)
        {
            std::cout << "  [ERROR] Please enter a number between 1 and 10000.\n";
            continue;
        }

        std::string leftover;
        if (stream >> leftover)
        {
            std::cout << "  [ERROR] Please enter a single number.\n";
            continue;
        }

        return static_cast<std::size_t>(value);
    }
}

void handle_put(lru_cache& cache)
{
    std::string key_str = read_line("  Enter key   : ");
    if (key_str.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    std::string val_str = read_line("  Enter value : ");
    if (val_str.empty())
    {
        std::cout << "  [ERROR] Value cannot be empty.\n";
        return;
    }

    uint64_t key = to_uint64(key_str);
    uint64_t value = to_uint64(val_str);

    cache.put(key, value);
    std::cout << "\n  [OK] Stored: " << key_str << " = " << val_str << "\n";

    if (cache.size() == cache.capacity())
    {
        std::cout << "  [INFO] Cache is full (" << cache.size() << "/" << cache.capacity() << ")\n";
    }
}

void handle_get(lru_cache& cache)
{
    if (cache.empty()) {
        std::cout << "\n  [INFO] Cache is currently empty. Nothing to get.\n";
        return;
    }

    std::string key_str = read_line("  Enter key : ");
    if (key_str.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    uint64_t key = to_uint64(key_str);
    auto result = cache.get(key);

    std::cout << "\n";
    if (result.found)
    {
        std::cout << "  [HIT] " << key_str << " = " << from_uint64(result.value) << "\n";
        std::cout << "  [INFO] '" << key_str << "' promoted to Most Recently Used\n";
    }
    else
    {
        std::cout << "  [MISS] Key '" << key_str << "' not found in cache.\n";
    }
}

void handle_remove(lru_cache& cache)
{
    if (cache.empty()) {
        std::cout << "\n  [INFO] Cache is currently empty. Nothing to remove.\n";
        return;
    }

    std::string key_str = read_line("  Enter key to remove : ");
    if (key_str.empty())
    {
        std::cout << "  [ERROR] Key cannot be empty.\n";
        return;
    }

    uint64_t key = to_uint64(key_str);
    std::cout << "\n";
    if (cache.remove(key))
    {
        std::cout << "  [OK] Removed '" << key_str << "' from cache.\n";
        std::cout << "  [INFO] Cache size: " << cache.size() << "/" << cache.capacity() << "\n";
    }
    else
    {
        std::cout << "  [ERROR] Key '" << key_str << "' not found in cache.\n";
    }
}

void handle_display(const lru_cache& cache)
{
    std::cout << "\n";
    if (cache.empty())
    {
        std::cout << "  (cache is empty)\n";
        return;
    }

    std::cout << "  ---- Cache Contents ----\n";
    
    // We pass a formatter to display the original strings
    cache.display([](uint64_t val) { return from_uint64(val); });
    
    std::cout << "  ------------------------\n";
}

void handle_stats(const lru_cache& cache)
{
    std::cout << "\n";
    std::cout << "  ==============================\n";
    std::cout << "       Cache Statistics\n";
    std::cout << "  ==============================\n";
    std::cout << "  Capacity         : " << cache.capacity() << "\n";
    std::cout << "  Current Size     : " << cache.size() << "\n";
    std::cout << "  Total Requests   : " << cache.total_requests() << "\n";
    std::cout << "  Cache Hits       : " << cache.hits() << "\n";
    std::cout << "  Cache Misses     : " << cache.misses() << "\n";
    std::cout << "  Evictions        : " << cache.evictions() << "\n";
    std::cout << "  Hit Ratio        : " << cache.hit_ratio() << "%\n";
    std::cout << "  ==============================\n";
}

void handle_clear(lru_cache& cache)
{
    if (cache.empty())
    {
        std::cout << "\n  [INFO] Cache is already empty.\n";
        return;
    }

    std::size_t old_size = cache.size();
    cache.clear();
    std::cout << "\n  [OK] Cleared " << old_size << " items. Cache and statistics reset.\n";
}

void handle_info(const lru_cache& cache)
{
    std::cout << "\n";
    std::cout << "  ==============================\n";
    std::cout << "       Cache Information\n";
    std::cout << "  ==============================\n";
    std::cout << "  Capacity       : " << cache.capacity() << "\n";
    std::cout << "  Current Size   : " << cache.size() << "\n";
    std::cout << "  Empty          : " << (cache.empty() ? "Yes" : "No") << "\n";
    std::cout << "  Space Used     : " << cache.size() << "/" << cache.capacity();

    if (cache.capacity() > 0)
    {
        double usage = static_cast<double>(cache.size()) / static_cast<double>(cache.capacity()) * 100.0;
        std::cout << " (" << usage << "%)";
    }

    std::cout << "\n";
    std::cout << "  Data Structure : Array-Based Pool + Open Addressing Map\n";
    std::cout << "  Allocations    : Zero Allocations at Runtime\n";
    std::cout << "  Complexity     : Strict O(1) PUT, GET, REMOVE\n";
    std::cout << "  ==============================\n";
}

int main()
{

    std::size_t capacity = read_capacity();
    auto cache = std::make_unique<lru_cache>(capacity);

    std::cout << "\n  [OK] Cache created with capacity " << capacity << ".\n";

    bool running = true;
    while (running)
    {
        print_menu(cache->capacity());
        int choice = read_choice();

        switch (choice)
        {
            case 1: handle_put(*cache); break;
            case 2: handle_get(*cache); break;
            case 3: handle_remove(*cache); break;
            case 4: handle_display(*cache); break;
            case 5: handle_stats(*cache); break;
            case 6: handle_clear(*cache); break;
            case 7: handle_info(*cache); break;
            case 8:
            {
                std::cout << "\n  Resetting cache with new capacity.\n";
                std::size_t new_capacity = read_capacity();
                cache = std::make_unique<lru_cache>(new_capacity);
                std::cout << "  [OK] Cache reset with capacity " << new_capacity << ".\n";
                break;
            }
            case 9:
                std::cout << "\n  [INFO] Exiting Cache Simulator. Goodbye!\n\n";
                running = false;
                break;
            default:
                std::cout << "\n  [ERROR] Invalid choice. Please enter a number between 1 and 9.\n";
                break;
        }
    }

    return 0;
}
