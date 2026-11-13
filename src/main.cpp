#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "lru_cache.hpp"

void print_state(const LRUCache<int, std::string>& cache) {
    std::cout << "    cache: [ ";
    for (int k : cache.keys()) std::cout << k << ' ';
    std::cout << "]  (MRU -> LRU)\n";
}

void do_get(LRUCache<int, std::string>& cache, int key) {
    std::optional<std::string> v = cache.get(key);
    std::cout << "get(" << key << ") -> " << (v ? *v : "MISS") << '\n';
    print_state(cache);
}

void do_put(LRUCache<int, std::string>& cache, int key, const std::string& value) {
    cache.put(key, value);
    std::cout << "put(" << key << ", " << value << ")\n";
    print_state(cache);
}

int main() {
    LRUCache<int, std::string> cache(2);

    do_put(cache, 1, "one");
    do_put(cache, 2, "two");
    do_get(cache, 1);          // hit: 1 becomes most recent
    do_put(cache, 3, "three"); // full: evicts 2 (least recent)
    do_get(cache, 2);          // miss: 2 was evicted
    do_put(cache, 4, "four");  // full: evicts 1
    do_get(cache, 1);          // miss
    do_get(cache, 3);          // hit
    do_get(cache, 4);          // hit

    const CacheStats& s = cache.stats();
    std::cout << "\nhits: " << s.hits << ", misses: " << s.misses
              << ", evictions: " << s.evictions
              << ", hit ratio: " << s.hit_ratio() << '\n';
    return 0;
}