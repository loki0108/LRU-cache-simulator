#pragma once

#include <cstddef>
#include <random>
#include <string>
#include <vector>

// Access-pattern generators. Each returns a "trace": the sequence of keys
// a program requests from the cache, one per access.

enum class Workload { Sequential, Random, Repeated };

inline std::string to_string(Workload w) {
    switch (w) {
        case Workload::Sequential: return "sequential";
        case Workload::Random:     return "random";
        case Workload::Repeated:   return "repeated";
    }
    return "unknown";
}

// Sequential: scans keys 0, 1, ..., num_keys-1 and then loops back to 0.
// Like iterating over a large array again and again.
inline std::vector<int> sequential_trace(int num_keys, std::size_t length) {
    std::vector<int> trace;
    trace.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        trace.push_back(static_cast<int>(i % static_cast<std::size_t>(num_keys)));
    }
    return trace;
}

// Random: every access picks a key uniformly at random. No locality at all.
inline std::vector<int> random_trace(int num_keys, std::size_t length, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> key(0, num_keys - 1);
    std::vector<int> trace;
    trace.reserve(length);
    for (std::size_t i = 0; i < length; ++i) trace.push_back(key(rng));
    return trace;
}

// Repeated access: a small "hot" set of keys is accessed most of the time
// (hot_prob), the rest of the time a "cold" key is picked. Models real
// programs, where a few items (popular pages, hot variables) dominate.
inline std::vector<int> repeated_trace(int num_keys, std::size_t length,
                                       int hot_keys, double hot_prob, unsigned seed) {
    std::mt19937 rng(seed);
    std::bernoulli_distribution pick_hot(hot_prob);
    std::uniform_int_distribution<int> hot(0, hot_keys - 1);           // keys [0, hot_keys)
    std::uniform_int_distribution<int> cold(hot_keys, num_keys - 1);   // keys [hot_keys, num_keys)
    std::vector<int> trace;
    trace.reserve(length);
    for (std::size_t i = 0; i < length; ++i) {
        trace.push_back(pick_hot(rng) ? hot(rng) : cold(rng));
    }
    return trace;
}