#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cache_stats.hpp"
#include "doubly_linked_list.hpp"

// LRU (Least Recently Used) cache with O(1) average get and put.
//
//   map_  : key -> pointer to its node in the list   (O(1) lookup)
//   list_ : nodes ordered from most to least recently used (O(1) reorder/evict)
//
// When the cache is full and a new key arrives, the node at the back of
// the list (the least recently used one) is evicted.
template <typename K, typename V>
class LRUCache {
    using List = DoublyLinkedList<K, V>;
    using Node = typename List::Node;

public:
    explicit LRUCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("LRUCache capacity must be at least 1");
        }
        map_.reserve(capacity_);  // avoid rehashing as the cache fills up
    }

    // Look up a key. On a hit, the entry becomes the most recently used.
    // Returns the value, or std::nullopt on a miss.
    std::optional<V> get(const K& key) {
        auto it = map_.find(key);            // O(1) average
        if (it == map_.end()) {
            ++stats_.misses;
            return std::nullopt;
        }
        ++stats_.hits;
        list_.move_to_front(it->second);     // O(1)
        return it->second->value;
    }

    // Insert or update a key. The entry becomes the most recently used.
    // If the cache is full and the key is new, the LRU entry is evicted first.
    void put(const K& key, V value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            // Key already present: update the value and mark it as just used.
            it->second->value = std::move(value);
            list_.move_to_front(it->second);
            return;
        }

        if (list_.size() == capacity_) {
            K evicted = list_.pop_back();    // O(1): remove LRU node from list
            map_.erase(evicted);             // O(1): remove it from the map too
            ++stats_.evictions;
        }

        Node* node = list_.push_front(key, std::move(value));
        map_.emplace(key, node);
    }

    // Membership check that does NOT change recency or stats.
    bool contains(const K& key) const { return map_.count(key) > 0; }

    std::size_t size() const { return list_.size(); }
    std::size_t capacity() const { return capacity_; }

    const CacheStats& stats() const { return stats_; }
    void reset_stats() { stats_ = CacheStats{}; }

    // Keys from most to least recently used (for debugging and tests).
    std::vector<K> keys() const { return list_.keys(); }

private:
    std::size_t capacity_;
    List list_;
    std::unordered_map<K, Node*> map_;
    CacheStats stats_;
};