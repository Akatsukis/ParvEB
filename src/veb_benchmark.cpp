#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <random>
#include <set>
#include <string_view>
#include <vector>

#include "absl/container/btree_set.h"
#include "ankerl/unordered_dense.h"
#include "quill/Quill.h"

#include "stopwatch.hpp"
#include "veb40.hpp"

int main()
{
    constexpr std::size_t kNumInserts = 10'000'000;

    quill::start();
    quill::preallocate();

    LOG_INFO("Starting vEB benchmark with {} inserts", kNumInserts);

    std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<uint64_t> dist(0, VebTree40::MAX_KEY);

    std::vector<uint64_t> values;
    values.reserve(kNumInserts);
    Stopwatch<> data_sw("value generation");
    for (std::size_t i = 0; i < kNumInserts; ++i) {
        values.emplace_back(dist(rng));
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

    Stopwatch<> ankerl_sw("ankerl::unordered_dense::set inserts");
    ankerl::unordered_dense::set<uint64_t> ankerl_set;
    ankerl_set.reserve(values.size());
    for (uint64_t value : values) {
        ankerl_set.insert(value);
    }
    ankerl_sw.stop();
    ankerl_sw.total_time();

    auto assert_sorted = [](std::string_view name, std::vector<uint64_t> const &data) {
        assert(std::is_sorted(data.begin(), data.end()) && "data must be sorted");
        LOG_INFO("{} produced {} unique keys", name, data.size());
    };

    std::vector<uint64_t> veb_sorted = tree.to_vector();

    std::vector<uint64_t> std_sorted(std_set.begin(), std_set.end());
    std::vector<uint64_t> absl_sorted(absl_set.begin(), absl_set.end());
    std::vector<uint64_t> ankerl_sorted(ankerl_set.begin(), ankerl_set.end());
    std::sort(ankerl_sorted.begin(), ankerl_sorted.end());

    assert_sorted("vEB", veb_sorted);
    assert_sorted("std::set", std_sorted);
    assert_sorted("absl::btree_set", absl_sorted);
    assert_sorted("ankerl::unordered_dense::set", ankerl_sorted);

    assert(veb_sorted == std_sorted);
    assert(veb_sorted == absl_sorted);
    assert(veb_sorted == ankerl_sorted);

    LOG_INFO("Benchmark complete");
    return 0;
}
