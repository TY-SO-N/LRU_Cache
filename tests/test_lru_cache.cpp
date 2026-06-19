#include "lru_cache.h"

#include <iostream>
#include <stdexcept>
#include <string>

int tests_run = 0;
int tests_passed = 0;

void report(const std::string& name, bool passed)
{
    ++tests_run;
    if (passed)
    {
        ++tests_passed;
        std::cout << "  [PASS] " << name << "\n";
    }
    else
    {
        std::cout << "  [FAIL] " << name << "\n";
    }
}

void test_basic_put_and_get()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");

    auto ra = cache.get("a");
    auto rb = cache.get("b");
    auto rc = cache.get("c");

    report("put then get returns correct value",
           ra.found && ra.value == "1");
    report("get existing key 'b'",
           rb.found && rb.value == "2");
    report("get existing key 'c'",
           rc.found && rc.value == "3");
    report("size is 3 after 3 puts",
           cache.size() == 3);
}

void test_get_missing_key()
{
    lru_cache cache(3);
    cache.put("a", "1");

    report("get missing key returns not found",
           !cache.get("z").found);
}

void test_put_updates_existing_key()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("a", "100");

    auto r = cache.get("a");
    report("put overwrites value for existing key",
           r.found && r.value == "100");
    report("size stays 1 after overwrite",
           cache.size() == 1);
}

void test_eviction_at_capacity()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");
    cache.put("d", "4");

    report("LRU item 'a' evicted after capacity exceeded",
           !cache.get("a").found);
    report("newer item 'b' still present",
           cache.get("b").found);
    report("newer item 'c' still present",
           cache.get("c").found);

    auto rd = cache.get("d");
    report("newest item 'd' present",
           rd.found && rd.value == "4");
    report("size is 3 (capacity) after eviction",
           cache.size() == 3);
}

void test_get_promotes_to_mru()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");

    cache.get("a");

    cache.put("d", "4");

    report("'a' promoted by get - not evicted",
           cache.get("a").found);
    report("'b' is now LRU - evicted",
           !cache.get("b").found);
}

void test_put_promotes_existing_key()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");

    cache.put("a", "100");

    cache.put("d", "4");

    auto ra = cache.get("a");
    report("'a' promoted by put - not evicted",
           ra.found && ra.value == "100");
    report("'b' is now LRU - evicted after put(d)",
           !cache.get("b").found);
}

void test_remove()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");

    report("remove existing key returns true",
           cache.remove("a"));
    report("removed key returns not found",
           !cache.get("a").found);
    report("size decremented after remove",
           cache.size() == 1);
    report("remove missing key returns false",
           !cache.remove("z"));
}

void test_clear()
{
    lru_cache cache(3);
    cache.put("a", "1");
    cache.put("b", "2");
    cache.put("c", "3");

    cache.clear();

    report("cache is empty after clear",
           cache.empty());
    report("size is 0 after clear",
           cache.size() == 0);
    report("get returns not found after clear",
           !cache.get("a").found);
}

void test_capacity_one()
{
    lru_cache cache(1);
    cache.put("a", "1");
    cache.put("b", "2");

    report("capacity-1 cache evicts on second put",
           !cache.get("a").found);

    auto rb = cache.get("b");
    report("capacity-1 cache holds latest item",
           rb.found && rb.value == "2");
    report("capacity-1 size is 1",
           cache.size() == 1);
}

void test_stats_tracking()
{
    lru_cache cache(2);
    cache.put("a", "1");
    cache.put("b", "2");

    cache.get("a");
    cache.get("b");
    cache.get("z");

    cache.put("c", "3");

    report("hits count is 2",
           cache.hits() == 2);
    report("misses count is 1",
           cache.misses() == 1);
    report("evictions count is 1",
           cache.evictions() == 1);
    report("total requests is 3",
           cache.total_requests() == 3);
}

void test_zero_capacity_throws()
{
    bool threw = false;
    try
    {
        lru_cache cache(0);
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }

    report("capacity 0 throws invalid_argument", threw);
}

int main()
{
    std::cout << "=== LRU Cache Unit Tests ===\n\n";

    test_basic_put_and_get();
    test_get_missing_key();
    test_put_updates_existing_key();
    test_eviction_at_capacity();
    test_get_promotes_to_mru();
    test_put_promotes_existing_key();
    test_remove();
    test_clear();
    test_capacity_one();
    test_stats_tracking();
    test_zero_capacity_throws();

    std::cout << "\n=== Results: " << tests_passed << "/" << tests_run << " passed ===\n";

    return (tests_passed == tests_run) ? 0 : 1;
}
