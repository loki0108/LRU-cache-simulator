// Unit tests for LRUCache.
// A tiny self-made test harness: no external libraries needed.

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "lru_cache.hpp"

// ---------- minimal test harness ----------

static int g_failed_checks = 0;

// If the condition is false, print where and what failed, then keep going
// (so one run reports every failing check, not just the first one).
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "  FAILED  " << __FILE__ << ':' << __LINE__       \
                      << "  CHECK(" #cond ")\n";                           \
            ++g_failed_checks;                                             \
        }                                                                  \
    } while (0)

using Keys = std::vector<int>;

// ---------- tests ----------

void test_get_on_empty_cache_is_miss() {
    LRUCache<int, int> c(2);
    CHECK(!c.get(1));
    CHECK(c.size() == 0);
}

void test_put_then_get_returns_value() {
    LRUCache<int, int> c(2);
    c.put(1, 100);
    CHECK(c.get(1) == 100);
    CHECK(c.size() == 1);
}

void test_evicts_least_recently_used_when_full() {
    LRUCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.put(3, 3);  // 1 is least recently used -> evicted
    CHECK(!c.get(1));
    CHECK(c.get(2) == 2);
    CHECK(c.get(3) == 3);
    CHECK(c.size() == 2);
}

void test_get_refreshes_recency() {
    LRUCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.get(1);     // 1 is now most recent, so 2 is the LRU
    c.put(3, 3);  // evicts 2, not 1
    CHECK(c.get(1) == 1);
    CHECK(!c.get(2));
    CHECK(c.get(3) == 3);
}

void test_put_existing_key_updates_value_and_recency() {
    LRUCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.put(1, 10);  // update, no eviction; 1 becomes most recent
    CHECK(c.size() == 2);
    CHECK(c.stats().evictions == 0);
    CHECK((c.keys() == Keys{1, 2}));

    c.put(3, 3);   // evicts 2
    CHECK(c.get(1) == 10);
    CHECK(!c.get(2));
}

void test_keys_are_in_mru_to_lru_order() {
    LRUCache<int, int> c(3);
    c.put(1, 1);
    c.put(2, 2);
    c.put(3, 3);
    CHECK((c.keys() == Keys{3, 2, 1}));
    c.get(1);
    CHECK((c.keys() == Keys{1, 3, 2}));
}

void test_capacity_one() {
    LRUCache<int, int> c(1);
    c.put(1, 1);
    c.put(2, 2);  // evicts 1 immediately
    CHECK(!c.get(1));
    CHECK(c.get(2) == 2);
    CHECK(c.size() == 1);
}

void test_capacity_zero_throws() {
    bool threw = false;
    try {
        LRUCache<int, int> c(0);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void test_contains_does_not_change_recency_or_stats() {
    LRUCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    CHECK(c.contains(1));
    CHECK(!c.contains(99));
    CHECK((c.keys() == Keys{2, 1}));  // order unchanged
    CHECK(c.stats().accesses() == 0); // not counted as a hit or miss
}

void test_stats_count_hits_misses_evictions() {
    LRUCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.get(1);     // hit
    c.get(5);     // miss
    c.put(3, 3);  // eviction
    c.get(2);     // miss (evicted)

    const CacheStats& s = c.stats();
    CHECK(s.hits == 1);
    CHECK(s.misses == 2);
    CHECK(s.evictions == 1);
    CHECK(s.accesses() == 3);
    CHECK(s.hit_ratio() > 0.333 && s.hit_ratio() < 0.334);

    c.reset_stats();
    CHECK(c.stats().accesses() == 0);
}

void test_works_with_string_keys() {
    LRUCache<std::string, int> c(2);
    c.put("apple", 1);
    c.put("banana", 2);
    c.get("apple");
    c.put("cherry", 3);  // evicts "banana"
    CHECK(c.get("apple") == 1);
    CHECK(!c.get("banana"));
    CHECK(c.get("cherry") == 3);
}

// Randomised test: run thousands of random get/put calls on the real cache
// and on a deliberately simple (slow, O(n)) reference version, and check
// they always agree. The reference is so simple it is obviously correct.
void test_matches_simple_reference_model() {
    const std::size_t capacity = 5;
    LRUCache<int, int> cache(capacity);
    std::vector<std::pair<int, int>> model;  // front = most recently used

    std::mt19937 rng(42);  // fixed seed -> same sequence every run
    std::uniform_int_distribution<int> key_dist(0, 19);
    std::uniform_int_distribution<int> op_dist(0, 1);

    for (int step = 0; step < 20000; ++step) {
        int key = key_dist(rng);
        auto it = std::find_if(model.begin(), model.end(),
                               [key](const auto& kv) { return kv.first == key; });

        if (op_dist(rng) == 0) {  // get
            std::optional<int> got = cache.get(key);
            if (it == model.end()) {
                CHECK(!got);
            } else {
                CHECK(got == it->second);
                auto kv = *it;
                model.erase(it);
                model.insert(model.begin(), kv);
            }
        } else {                  // put
            int value = step;
            cache.put(key, value);
            if (it != model.end()) {
                model.erase(it);
            } else if (model.size() == capacity) {
                model.pop_back();
            }
            model.insert(model.begin(), {key, value});
        }

        Keys expected;
        for (const auto& kv : model) expected.push_back(kv.first);
        if (cache.keys() != expected) {
            CHECK(cache.keys() == expected);
            return;  // stop after the first divergence to avoid flooding output
        }
    }
}

// ---------- runner ----------

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests = {
        {"get on empty cache is a miss",            test_get_on_empty_cache_is_miss},
        {"put then get returns value",              test_put_then_get_returns_value},
        {"evicts least recently used when full",    test_evicts_least_recently_used_when_full},
        {"get refreshes recency",                   test_get_refreshes_recency},
        {"put on existing key updates and refreshes", test_put_existing_key_updates_value_and_recency},
        {"keys are in MRU -> LRU order",            test_keys_are_in_mru_to_lru_order},
        {"capacity one",                            test_capacity_one},
        {"capacity zero throws",                    test_capacity_zero_throws},
        {"contains does not change recency/stats",  test_contains_does_not_change_recency_or_stats},
        {"stats count hits, misses, evictions",     test_stats_count_hits_misses_evictions},
        {"works with string keys",                  test_works_with_string_keys},
        {"matches simple reference model (random)", test_matches_simple_reference_model},
    };

    int failed_tests = 0;
    for (const auto& [name, fn] : tests) {
        int before = g_failed_checks;
        fn();
        bool ok = (g_failed_checks == before);
        if (!ok) ++failed_tests;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << '\n';
    }

    std::cout << '\n' << (tests.size() - failed_tests) << '/' << tests.size()
              << " tests passed\n";
    return failed_tests == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}