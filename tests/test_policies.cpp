// Unit tests for the baseline eviction policies (FIFO and Random).

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "fifo_cache.hpp"
#include "random_cache.hpp"

static int g_failed_checks = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            std::cerr << "  FAILED  " << __FILE__ << ':' << __LINE__       \
                      << "  CHECK(" #cond ")\n";                           \
            ++g_failed_checks;                                             \
        }                                                                  \
    } while (0)

using Keys = std::vector<int>;

void test_fifo_evicts_oldest_inserted_even_if_recently_used() {
    FIFOCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.get(1);     // in LRU this would protect 1 -- in FIFO it does not
    c.put(3, 3);  // evicts 1 (inserted first)
    CHECK(!c.get(1));
    CHECK(c.get(2) == 2);
    CHECK(c.get(3) == 3);
}

void test_fifo_update_keeps_position() {
    FIFOCache<int, int> c(2);
    c.put(1, 1);
    c.put(2, 2);
    c.put(1, 10);  // update value, 1 is still the oldest
    CHECK((c.keys() == Keys{2, 1}));
    c.put(3, 3);   // evicts 1
    CHECK(!c.get(1));
    CHECK(c.stats().evictions == 1);
}

void test_fifo_capacity_zero_throws() {
    bool threw = false;
    try { FIFOCache<int, int> c(0); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
}

void test_random_never_exceeds_capacity_and_keeps_new_key() {
    RandomCache<int, int> c(3);
    for (int k = 0; k < 1000; ++k) {
        c.put(k, k);
        CHECK(c.size() <= 3);
        CHECK(c.contains(k));  // the key just inserted is always present
    }
    CHECK(c.stats().evictions == 997);
}

void test_random_hit_returns_value() {
    RandomCache<int, int> c(2);
    c.put(1, 100);
    CHECK(c.get(1) == 100);
    CHECK(!c.get(2));
    CHECK(c.stats().hits == 1);
    CHECK(c.stats().misses == 1);
}

int main() {
    struct Test { const char* name; void (*fn)(); };
    const Test tests[] = {
        {"FIFO evicts oldest inserted even if recently used", test_fifo_evicts_oldest_inserted_even_if_recently_used},
        {"FIFO update keeps position",                         test_fifo_update_keeps_position},
        {"FIFO capacity zero throws",                          test_fifo_capacity_zero_throws},
        {"Random never exceeds capacity, keeps new key",       test_random_never_exceeds_capacity_and_keeps_new_key},
        {"Random hit returns value",                           test_random_hit_returns_value},
    };

    int failed = 0, total = 0;
    for (const Test& t : tests) {
        int before = g_failed_checks;
        t.fn();
        bool ok = (g_failed_checks == before);
        failed += !ok;
        ++total;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << t.name << '\n';
    }
    std::cout << '\n' << (total - failed) << '/' << total << " tests passed\n";
    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}