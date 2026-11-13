#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cache_stats.hpp"

// FIFO (First In, First Out) cache: evicts the entry that was INSERTED
// earliest, no matter how often it has been used since. Baseline to compare
// against LRU. get and put are O(1) average.
template <typename K, typename V>
class FIFOCache {
public:
    explicit FIFOCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("FIFOCache capacity must be at least 1");
        }
        map_.reserve(capacity_);
    }

    // Unlike LRU, a hit does NOT change the eviction order.
    std::optional<V> get(const K& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            ++stats_.misses;
            return std::nullopt;
        }
        ++stats_.hits;
        return it->second;
    }

    void put(const K& key, V value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second = std::move(value);  // update in place, order unchanged
            return;
        }
        if (map_.size() == capacity_) {
            map_.erase(order_.front());     // oldest inserted key
            order_.pop_front();
            ++stats_.evictions;
        }
        order_.push_back(key);
        map_.emplace(key, std::move(value));
    }

    bool contains(const K& key) const { return map_.count(key) > 0; }
    std::size_t size() const { return map_.size(); }
    std::size_t capacity() const { return capacity_; }
    const CacheStats& stats() const { return stats_; }
    void reset_stats() { stats_ = CacheStats{}; }

    // Keys from newest to oldest inserted (same direction as LRUCache::keys()).
    std::vector<K> keys() const { return std::vector<K>(order_.rbegin(), order_.rend()); }

private:
    std::size_t capacity_;
    std::unordered_map<K, V> map_;
    std::deque<K> order_;  // front = oldest inserted, back = newest
    CacheStats stats_;
};