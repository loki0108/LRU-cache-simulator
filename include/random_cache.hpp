#pragma once

#include <cstddef>
#include <optional>
#include <random>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cache_stats.hpp"

// Random-eviction cache: when full, evicts a uniformly random entry.
// Baseline to compare against LRU. get and put are O(1) average.
template <typename K, typename V>
class RandomCache {
public:
    explicit RandomCache(std::size_t capacity, unsigned seed = 42)
        : capacity_(capacity), rng_(seed) {
        if (capacity_ == 0) {
            throw std::invalid_argument("RandomCache capacity must be at least 1");
        }
        map_.reserve(capacity_);
        keys_.reserve(capacity_);
    }

    std::optional<V> get(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            ++stats_.misses;
            return std::nullopt;
        }
        ++stats_.hits;
        return it->second.value;
    }

    void put(const K& key, V value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second.value = std::move(value);
            return;
        }
        if (keys_.size() == capacity_) evict_random();
        map_.emplace(key, Entry{std::move(value), keys_.size()});
        keys_.push_back(key);
    }

    bool contains(const K& key) const { return map_.count(key) > 0; }
    std::size_t size() const { return keys_.size(); }
    std::size_t capacity() const { return capacity_; }
    const CacheStats& stats() const { return stats_; }
    void reset_stats() { stats_ = CacheStats{}; }

private:
    struct Entry {
        V value;
        std::size_t index;  // position of this key inside keys_
    };

    // O(1) removal from the middle of a vector: overwrite the victim with the
    // last element, fix that element's stored index, then pop the back.
    void evict_random() {
        std::uniform_int_distribution<std::size_t> pick(0, keys_.size() - 1);
        std::size_t victim = pick(rng_);
        map_.erase(keys_[victim]);
        if (victim != keys_.size() - 1) {
            keys_[victim] = keys_.back();
            map_.at(keys_[victim]).index = victim;
        }
        keys_.pop_back();
        ++stats_.evictions;
    }

    std::size_t capacity_;
    std::unordered_map<K, Entry> map_;
    std::vector<K> keys_;  // all cached keys, so a random one can be picked in O(1)
    std::mt19937 rng_;
    CacheStats stats_;
};