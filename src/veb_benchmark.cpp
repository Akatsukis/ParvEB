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

#include <boost/container/set.hpp>

#include "absl/container/btree_set.h"
#include "quill/Quill.h"

#include "stopwatch.hpp"
#include "veb32.hpp"

namespace
{

    std::optional<uint32_t>
    predecessor_from_set(std::set<uint32_t> const &data, uint32_t key)
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
    std::optional<uint32_t>
    successor_from_ordered(Set const &data, uint32_t key)
    {
        auto it = data.upper_bound(key);
        if (it == data.end()) {
            return std::nullopt;
        }
        return *it;
    }

    template <class Set>
    std::optional<uint32_t>
    predecessor_from_ordered(Set const &data, uint32_t key)
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

    template <class QueryVec, class Fn>
    auto collect_queries(
        Stopwatch<> &sw, std::string_view label,
        QueryVec const &queries, Fn &&fn)
        -> std::vector<decltype(fn(queries[0]))>
    {
        using Result = decltype(fn(queries[0]));
        std::vector<Result> results;
        results.reserve(queries.size());
        for (auto const &key : queries) {
            results.push_back(fn(key));
        }
        sw.next(label);
        return results;
    }

} // namespace

int main()
{
    constexpr std::size_t kNumInserts = 10'000'000;

    quill::start();
    quill::preallocate();

    LOG_INFO("=== vEB benchmark: {} inserts ===", kNumInserts);

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> value_dist(
        0, VebTree32::MAX_KEY);
    std::uniform_int_distribution<uint32_t> succ_dist(
        0, VebTree32::MAX_KEY);
    std::uniform_int_distribution<uint32_t> pred_dist(
        0, VebTree32::MAX_KEY);

    std::vector<uint32_t> values;
    values.reserve(kNumInserts);
    std::vector<uint32_t> successor_queries;
    successor_queries.reserve(kNumInserts);
    std::vector<uint32_t> predecessor_queries;
    predecessor_queries.reserve(kNumInserts);

    Stopwatch<> data_sw("random data generation");
    for (std::size_t i = 0; i < kNumInserts; ++i) {
        values.emplace_back(value_dist(rng));
        successor_queries.emplace_back(succ_dist(rng));
        predecessor_queries.emplace_back(pred_dist(rng));
    }
    data_sw.stop();
    data_sw.total_time();

    auto assert_sorted = [](std::string_view, auto const &data) {
        assert(
            std::is_sorted(data.begin(), data.end()) && "data must be sorted");
    };

    // std::set baseline (also generates expected answers)
    LOG_INFO("--- std::set ---");
    Stopwatch<> std_sw("std::set");
    std::set<uint32_t> std_set;
    for (uint32_t value : values) {
        std_set.insert(value);
    }
    std_sw.next("insert");
    std::vector<uint32_t> std_sorted(std_set.begin(), std_set.end());
    assert_sorted("std::set", std_sorted);

    std::vector<std::optional<uint32_t>> expected_successors(
        successor_queries.size());
    for (std::size_t i = 0; i < successor_queries.size(); ++i) {
        expected_successors[i] =
            successor_from_ordered(std_set, successor_queries[i]);
    }
    std_sw.next("successor");

    std::vector<std::optional<uint32_t>> expected_predecessors(
        predecessor_queries.size());
    for (std::size_t i = 0; i < predecessor_queries.size(); ++i) {
        expected_predecessors[i] =
            predecessor_from_set(std_set, predecessor_queries[i]);
    }
    std_sw.next("predecessor");
    std_sw.total_time();

    // vEB tree
    LOG_INFO("--- vEB ---");
    Stopwatch<> veb_sw("vEB");
    VebTree32 tree;
    for (uint32_t value : values) {
        tree.insert(value);
    }
    veb_sw.next("insert");
    std::vector<uint32_t> veb_sorted = tree.to_vector();
    assert_sorted("vEB", veb_sorted);
    assert(veb_sorted == std_sorted);
    auto veb_successors = collect_queries(
        veb_sw, "successor", successor_queries, [&](uint32_t key) {
            return tree.successor(key);
        });
    auto veb_predecessors = collect_queries(
        veb_sw, "predecessor", predecessor_queries, [&](uint32_t key) {
            return tree.predecessor(key);
        });
    assert(veb_successors == expected_successors);
    assert(veb_predecessors == expected_predecessors);
    veb_sw.total_time();

    // absl::btree_set
    LOG_INFO("--- absl::btree_set ---");
    Stopwatch<> absl_sw("absl::btree_set");
    absl::btree_set<uint32_t> absl_set;
    for (uint32_t value : values) {
        absl_set.insert(value);
    }
    absl_sw.next("insert");
    std::vector<uint32_t> absl_sorted(absl_set.begin(), absl_set.end());
    assert_sorted("absl::btree_set", absl_sorted);
    assert(absl_sorted == std_sorted);
    auto absl_successors = collect_queries(
        absl_sw, "successor", successor_queries, [&](uint32_t key) {
            return successor_from_ordered(absl_set, key);
        });
    auto absl_predecessors = collect_queries(
        absl_sw, "predecessor", predecessor_queries, [&](uint32_t key) {
            return predecessor_from_ordered(absl_set, key);
        });
    assert(absl_successors == expected_successors);
    assert(absl_predecessors == expected_predecessors);
    absl_sw.total_time();

    // boost::container::set
    LOG_INFO("--- boost::container::set ---");
    Stopwatch<> boost_sw("boost::container::set");
    boost::container::set<uint32_t> boost_set;
    for (uint32_t value : values) {
        boost_set.insert(value);
    }
    boost_sw.next("insert");
    std::vector<uint32_t> boost_sorted(boost_set.begin(), boost_set.end());
    assert_sorted("boost::container::set", boost_sorted);
    assert(boost_sorted == std_sorted);
    auto boost_successors = collect_queries(
        boost_sw, "successor", successor_queries, [&](uint32_t key) {
            return successor_from_ordered(boost_set, key);
        });
    auto boost_predecessors = collect_queries(
        boost_sw, "predecessor", predecessor_queries, [&](uint32_t key) {
            return predecessor_from_ordered(boost_set, key);
        });
    assert(boost_successors == expected_successors);
    assert(boost_predecessors == expected_predecessors);
    boost_sw.total_time();

    LOG_INFO("Benchmark complete");
    return 0;
}
