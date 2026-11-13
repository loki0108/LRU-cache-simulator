#pragma once

#include <cstddef>

// Counters every cache keeps, so the simulator can compare policies later.
struct CacheStats {
    std::size_t hits = 0;
    std::size_t misses = 0;
    std::size_t evictions = 0;

    std::size_t accesses() const { return hits + misses; }

    double hit_ratio() const {
        return accesses() == 0 ? 0.0 : static_cast<double>(hits) / accesses();
    }

    double miss_ratio() const {
        return accesses() == 0 ? 0.0 : static_cast<double>(misses) / accesses();
    }
};