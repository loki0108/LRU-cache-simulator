// LRU Cache Simulator
// Runs sequential, random and repeated-access workloads through the cache
// at several capacities and reports hit/miss ratios.

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "fifo_cache.hpp"
#include "lru_cache.hpp"
#include "random_cache.hpp"
#include "workload.hpp"

struct Config {
    int num_keys = 1000;          // size of the key space
    std::size_t num_ops = 100000; // accesses per trace
    unsigned seed = 42;           // fixed seed -> reproducible results
    std::string out = "results/results.csv";
};

struct Result {
    std::string workload;
    std::string policy;
    std::size_t capacity;
    CacheStats stats;
};

// Replay a trace through a cache: try get(); on a miss, load the key with put().
// This is how a cache in front of slow storage behaves.
template <typename Cache>
Result simulate(const std::string& policy, const std::string& workload,
                const std::vector<int>& trace, std::size_t capacity) {
    Cache cache(capacity);
    for (int key : trace) {
        if (!cache.get(key)) {
            cache.put(key, key);
        }
    }
    return {workload, policy, capacity, cache.stats()};
}

Config parse_args(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i + 1 < argc; i += 2) {
        std::string flag = argv[i];
        std::string value = argv[i + 1];
        if (flag == "--keys")      cfg.num_keys = std::stoi(value);
        else if (flag == "--ops")  cfg.num_ops = std::stoul(value);
        else if (flag == "--seed") cfg.seed = static_cast<unsigned>(std::stoul(value));
        else if (flag == "--out")  cfg.out = value;
        else {
            std::cerr << "Unknown option: " << flag << '\n'
                      << "Usage: simulator [--keys N] [--ops N] [--seed N] [--out file.csv]\n";
            std::exit(EXIT_FAILURE);
        }
    }
    return cfg;
}

void print_table(const std::vector<Result>& results) {
    std::cout << std::left << std::setw(12) << "workload" << std::setw(8) << "policy"
              << std::right << std::setw(10) << "capacity" << std::setw(10) << "hits"
              << std::setw(10) << "misses" << std::setw(12) << "hit ratio" << '\n'
              << std::string(62, '-') << '\n';
    std::string last_workload;
    for (const Result& r : results) {
        if (!last_workload.empty() && r.workload != last_workload) std::cout << '\n';
        last_workload = r.workload;
        std::cout << std::left << std::setw(12) << r.workload << std::setw(8) << r.policy
                  << std::right << std::setw(10) << r.capacity << std::setw(10) << r.stats.hits
                  << std::setw(10) << r.stats.misses << std::setw(11) << std::fixed
                  << std::setprecision(2) << r.stats.hit_ratio() * 100 << "%\n";
    }
}

void write_csv(const std::vector<Result>& results, const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) std::filesystem::create_directories(p.parent_path());
    std::ofstream out(p);
    out << "workload,policy,capacity,hits,misses,evictions,hit_ratio,miss_ratio\n";
    for (const Result& r : results) {
        out << r.workload << ',' << r.policy << ',' << r.capacity << ',' << r.stats.hits << ','
            << r.stats.misses << ',' << r.stats.evictions << ',' << std::setprecision(6)
            << r.stats.hit_ratio() << ',' << r.stats.miss_ratio() << '\n';
    }
}

int main(int argc, char** argv) {
    const Config cfg = parse_args(argc, argv);
    const int hot_keys = cfg.num_keys / 10;  // hot set = 10% of keys...
    const double hot_prob = 0.8;             // ...receiving 80% of accesses

    // Capacities as a fraction of the key space: 1%, 5%, 10%, 25%, 50%, 75%, 100%.
    std::vector<std::size_t> capacities;
    for (double f : {0.01, 0.05, 0.10, 0.25, 0.50, 0.75, 1.00}) {
        capacities.push_back(static_cast<std::size_t>(f * cfg.num_keys));
    }

    std::cout << "keys=" << cfg.num_keys << "  ops=" << cfg.num_ops << "  seed=" << cfg.seed
              << "  (repeated: " << hot_keys << " hot keys get " << hot_prob * 100
              << "% of accesses)\n\n";

    std::vector<Result> results;
    for (Workload w : {Workload::Sequential, Workload::Random, Workload::Repeated}) {
        std::vector<int> trace;
        switch (w) {
            case Workload::Sequential: trace = sequential_trace(cfg.num_keys, cfg.num_ops); break;
            case Workload::Random:     trace = random_trace(cfg.num_keys, cfg.num_ops, cfg.seed); break;
            case Workload::Repeated:
                trace = repeated_trace(cfg.num_keys, cfg.num_ops, hot_keys, hot_prob, cfg.seed);
                break;
        }
                for (std::size_t cap : capacities) {
                    results.push_back(simulate<LRUCache<int, int>>("LRU", to_string(w), trace, cap));
                    results.push_back(simulate<FIFOCache<int, int>>("FIFO", to_string(w), trace, cap));
                    results.push_back(simulate<RandomCache<int, int>>("Random", to_string(w), trace, cap));
                }
    }

    print_table(results);
    write_csv(results, cfg.out);
    std::cout << "\nResults written to " << cfg.out << '\n';
    return 0;
}