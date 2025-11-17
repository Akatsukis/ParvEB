#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <tsl/ordered_set.h>

#include "absl/container/btree_set.h"
#include "quill/Quill.h"

#include "stopwatch.hpp"
#include "veb40.hpp"

namespace {

std::optional<uint64_t> predecessor_from_set(std::set<uint64_t> const &data, uint64_t key)
{
    if (data.empty()) {
        return std::nullopt;
    }
    auto it = data.lower_bound(key);
    if (it == data.begin()) {
        if (it == data.end() || *it >= key) {
            return std::nullopt;
        }
    }
    if (it == data.end() || *it >= key) {
        --it;
        return *it;
    }
    return *it;
}

template <class Set>
std::optional<uint64_t> successor_from_ordered(const Set &data, uint64_t key)
{
    auto it = data.upper_bound(key);
    if (it == data.end()) {
        return std::nullopt;
    }
    return *it;
}

template <class Set>
std::optional<uint64_t> predecessor_from_ordered(const Set &data, uint64_t key)
{
    if (data.empty()) {
        return std::nullopt;
    }
    auto it = data.lower_bound(key);
    if (it == data.begin()) {
        if (it == data.end() || *it >= key) {
            return std::nullopt;
        }
    }
    if (it == data.end() || *it >= key) {
        --it;
        return *it;
    }
    return *it;
}

template <class Fn>
void benchmark_queries(std::string label,
                       std::vector<uint64_t> const &queries,
                       std::vector<std::optional<uint64_t>> const &expected,
                       Fn &&fn)
{
    Stopwatch<> sw(std::move(label), false);
    sw.start();
    for (std::size_t i = 0; i < queries.size(); ++i) {
        auto result = fn(queries[i]);
        assert(result == expected[i]);
    }
    sw.stop();
    sw.total_time();
}

} // namespace

int main()
{
    constexpr std::size_t kNumInserts = 10'000'000;

    quill::start();
    quill::preallocate();

    LOG_INFO("Starting vEB benchmark with {} inserts", kNumInserts);

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> value_dist(0, VebTree40::MAX_KEY);
    std::uniform_int_distribution<uint64_t> succ_dist(0, VebTree40::MAX_KEY);
    std::uniform_int_distribution<uint64_t> pred_dist(
        0, VebTree40::PREDECESSOR_QUERY_MAX);

    std::vector<uint64_t> values;
    values.reserve(kNumInserts);
    std::vector<uint64_t> successor_queries;
    successor_queries.reserve(kNumInserts);
    std::vector<uint64_t> predecessor_queries;
    predecessor_queries.reserve(kNumInserts);

    Stopwatch<> data_sw("random data generation");
    for (std::size_t i = 0; i < kNumInserts; ++i) {
        values.emplace_back(value_dist(rng));
        successor_queries.emplace_back(succ_dist(rng));
        predecessor_queries.emplace_back(pred_dist(rng));
    }
    data_sw.stop();
    data_sw.total_time();

    Stopwatch<> construct_sw("vEB construction", false);
    construct_sw.start();
    VebTree40 tree;
    construct_sw.stop();
    construct_sw.total_time();

    Stopwatch<> insert_sw("vEB inserts", false);
    insert_sw.start();
    for (uint64_t value : values) {
        tree.insert(value);
    }
    insert_sw.stop();
    insert_sw.total_time();

    Stopwatch<> std_sw("std::set inserts");
    std::set<uint64_t> std_set;
    for (uint64_t value : values) {
        std_set.insert(value);
    }
    std_sw.stop();
    std_sw.total_time();

    Stopwatch<> absl_sw("absl::btree_set inserts");
    absl::btree_set<uint64_t> absl_set;
    for (uint64_t value : values) {
        absl_set.insert(value);
    }
    absl_sw.stop();
    absl_sw.total_time();

    Stopwatch<> tsl_sw("tsl::ordered_set inserts");
    tsl::ordered_set<uint64_t> tsl_set;
    tsl_set.reserve(values.size());
    for (uint64_t value : values) {
        tsl_set.insert(value);
    }
    tsl_sw.stop();
    tsl_sw.total_time();

    auto assert_sorted = [](std::string_view name, std::vector<uint64_t> const &data) {
        assert(std::is_sorted(data.begin(), data.end()) && "data must be sorted");
        LOG_INFO("{} produced {} unique keys", name, data.size());
    };

    std::vector<uint64_t> veb_sorted = tree.to_vector();
    std::vector<uint64_t> std_sorted(std_set.begin(), std_set.end());
    std::vector<uint64_t> absl_sorted(absl_set.begin(), absl_set.end());
    std::vector<uint64_t> tsl_sorted(tsl_set.begin(), tsl_set.end());
    std::sort(tsl_sorted.begin(), tsl_sorted.end());

    assert_sorted("vEB", veb_sorted);
    assert_sorted("std::set", std_sorted);
    assert_sorted("absl::btree_set", absl_sorted);
    assert_sorted("tsl::ordered_set", tsl_sorted);

    assert(veb_sorted == std_sorted);
    assert(veb_sorted == absl_sorted);
    assert(veb_sorted == tsl_sorted);

    auto std_successor_lookup = [&](uint64_t key) -> std::optional<uint64_t> {
        return successor_from_ordered(std_set, key);
    };
    auto std_predecessor_lookup = [&](uint64_t key) -> std::optional<uint64_t> {
        return predecessor_from_set(std_set, key);
    };

    std::vector<std::optional<uint64_t>> expected_successors;
    expected_successors.reserve(successor_queries.size());
    for (uint64_t key : successor_queries) {
        expected_successors.push_back(std_successor_lookup(key));
    }

    std::vector<std::optional<uint64_t>> expected_predecessors;
    expected_predecessors.reserve(predecessor_queries.size());
    for (uint64_t key : predecessor_queries) {
        expected_predecessors.push_back(std_predecessor_lookup(key));
    }

    auto benchmark_successor = [&](std::string_view name, auto &&fn) {
        benchmark_queries(std::string(name) + " successor",
                          successor_queries,
                          expected_successors,
                          std::forward<decltype(fn)>(fn));
    };

    auto benchmark_predecessor = [&](std::string_view name, auto &&fn) {
        benchmark_queries(std::string(name) + " predecessor",
                          predecessor_queries,
                          expected_predecessors,
                          std::forward<decltype(fn)>(fn));
    };

    benchmark_successor("vEB", [&](uint64_t key) { return tree.successor(key); });
    benchmark_successor("std::set", std_successor_lookup);
    benchmark_successor("absl::btree_set", [&](uint64_t key) {
        return successor_from_ordered(absl_set, key);
    });

    benchmark_predecessor("vEB", [&](uint64_t key) { return tree.predecessor(key); });
    benchmark_predecessor("std::set", std_predecessor_lookup);
    benchmark_predecessor("absl::btree_set", [&](uint64_t key) {
        return predecessor_from_ordered(absl_set, key);
    });

    LOG_INFO("Benchmark complete");
    return 0;
}
