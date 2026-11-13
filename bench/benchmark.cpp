// Timing benchmark: measures the average time of one get/put access at
// very different cache capacities. If get and put are O(1), the time per
// operation should stay roughly flat as capacity grows 1000x.

#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>

#include "lru_cache.hpp"

int main() {
    const std::size_t num_ops = 2'000'000;
    volatile long long sink = 0;  // stops the compiler optimising the work away

    std::cout << std::setw(12) << "capacity" << std::setw(16) << "ns per access" << '\n'
              << std::string(28, '-') << '\n';

    for (std::size_t capacity : {1'000u, 10'000u, 100'000u, 1'000'000u}) {
        LRUCache<int, int> cache(capacity);

        // Keys drawn from 2x the capacity -> roughly half hits, half misses+evictions,
        // so both get and put (with eviction) are exercised.
        std::mt19937 rng(42);
        std::uniform_int_distribution<int> key(0, static_cast<int>(2 * capacity) - 1);
        std::vector<int> keys(num_ops);
        for (int& k : keys) k = key(rng);

        for (std::size_t i = 0; i < capacity; ++i) cache.put(static_cast<int>(i), 0);  // warm up

        auto start = std::chrono::steady_clock::now();
        for (int k : keys) {
            if (auto v = cache.get(k)) sink = sink + *v;
            else cache.put(k, k);
        }
        auto end = std::chrono::steady_clock::now();

        double ns = std::chrono::duration<double, std::nano>(end - start).count() / num_ops;
        std::cout << std::setw(12) << capacity << std::setw(16) << std::fixed
                  << std::setprecision(1) << ns << '\n';
    }
    return 0;
}