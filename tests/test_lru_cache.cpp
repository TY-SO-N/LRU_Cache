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
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    auto ra = cache.get(1);
    auto rb = cache.get(2);
    auto rc = cache.get(3);

    report("put then get returns correct value", ra.found && ra.value == 10);
    report("get existing key '2'", rb.found && rb.value == 20);
    report("get existing key '3'", rc.found && rc.value == 30);
    report("size is 3 after 3 puts", cache.size() == 3);
}

void test_get_missing_key()
{
    lru_cache cache(3);
    cache.put(1, 10);
    report("get missing key returns not found", !cache.get(99).found);
}

void test_put_updates_existing_key()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(1, 100);

    auto r = cache.get(1);
    report("put overwrites value for existing key", r.found && r.value == 100);
    report("size stays 1 after overwrite", cache.size() == 1);
}

void test_eviction_at_capacity()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);
    cache.put(4, 40);

    report("LRU item '1' evicted after capacity exceeded", !cache.get(1).found);
    report("newer item '2' still present", cache.get(2).found);
    report("newer item '3' still present", cache.get(3).found);

    auto rd = cache.get(4);
    report("newest item '4' present", rd.found && rd.value == 40);
    report("size is 3 (capacity) after eviction", cache.size() == 3);
}

void test_get_promotes_to_mru()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    cache.get(1);
    cache.put(4, 40);

    report("'1' promoted by get - not evicted", cache.get(1).found);
    report("'2' is now LRU - evicted", !cache.get(2).found);
}

void test_put_promotes_existing_key()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    cache.put(1, 100);
    cache.put(4, 40);

    auto ra = cache.get(1);
    report("'1' promoted by put - not evicted", ra.found && ra.value == 100);
    report("'2' is now LRU - evicted after put(4)", !cache.get(2).found);
}

void test_remove()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);

    report("remove existing key returns true", cache.remove(1));
    report("removed key returns not found", !cache.get(1).found);
    report("size decremented after remove", cache.size() == 1);
    report("remove missing key returns false", !cache.remove(99));
}

void test_clear()
{
    lru_cache cache(3);
    cache.put(1, 10);
    cache.put(2, 20);
    cache.put(3, 30);

    cache.clear();

    report("cache is empty after clear", cache.empty());
    report("size is 0 after clear", cache.size() == 0);
    report("get returns not found after clear", !cache.get(1).found);
}

void test_capacity_one()
{
    lru_cache cache(1);
    cache.put(1, 10);
    cache.put(2, 20);

    report("capacity-1 cache evicts on second put", !cache.get(1).found);

    auto rb = cache.get(2);
    report("capacity-1 cache holds latest item", rb.found && rb.value == 20);
    report("capacity-1 size is 1", cache.size() == 1);
}

void test_stats_tracking()
{
    lru_cache cache(2);
    cache.put(1, 10);
    cache.put(2, 20);

    cache.get(1);
    cache.get(2);
    cache.get(99);

    cache.put(3, 30);

    report("hits count is 2", cache.hits() == 2);
    report("misses count is 1", cache.misses() == 1);
    report("evictions count is 1", cache.evictions() == 1);
    report("total requests is 3", cache.total_requests() == 3);
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
    std::cout << "=== HFT LRU Cache Unit Tests ===\n\n";

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
