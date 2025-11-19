#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <optional>
#include <random>
#include <set>
#include <stdexcept>
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

    enum class DistributionKind
    {
        Uniform,
        Exponential,
        Zipfian
    };

    std::string_view to_string(DistributionKind kind)
    {
        switch (kind) {
        case DistributionKind::Uniform:
            return "uniform";
        case DistributionKind::Exponential:
            return "exponential";
        case DistributionKind::Zipfian:
            return "zipfian";
        }
        return "unknown";
    }

    struct BenchmarkOptions
    {
        DistributionKind distribution = DistributionKind::Uniform;
        double skew = 1.0;
        std::size_t num_inserts = 10'000'000;
    };

    class DistributionSampler
    {
    public:
        DistributionSampler(
            DistributionKind kind, double skew, uint32_t max_key)
            : kind_(kind), skew_(skew), max_key_(max_key)
        {
            initialize();
        }

        uint32_t sample(std::mt19937_64 &rng)
        {
            std::size_t bucket = bucket_picker_(rng);
            uint32_t start = bucket_start_[bucket];
            uint32_t end = bucket_end_[bucket];
            if (start == end) {
                return start;
            }
            std::uniform_int_distribution<uint32_t> dist(start, end);
            return dist(rng);
        }

    private:
        static constexpr std::size_t kBucketCount = 4096;

        void initialize()
        {
            bucket_start_.resize(kBucketCount);
            bucket_end_.resize(kBucketCount);
            std::vector<double> weights(kBucketCount);
            uint64_t span =
                static_cast<uint64_t>(max_key_) + 1ULL;
            uint64_t width =
                std::max<uint64_t>(1, span / kBucketCount);

            for (std::size_t i = 0; i < kBucketCount; ++i) {
                uint64_t start = std::min<uint64_t>(i * width, max_key_);
                uint64_t end =
                    std::min<uint64_t>((i + 1) * width - 1, max_key_);
                bucket_start_[i] = static_cast<uint32_t>(start);
                bucket_end_[i] = static_cast<uint32_t>(std::max(start, end));
                weights[i] = compute_weight(i);
            }
            bucket_picker_ =
                std::discrete_distribution<std::size_t>(
                    weights.begin(), weights.end());
        }

        double compute_weight(std::size_t idx) const
        {
            switch (kind_) {
            case DistributionKind::Uniform:
                return 1.0;
            case DistributionKind::Exponential: {
                double lambda = skew_ > 0.0 ? skew_ : 0.0;
                if (lambda == 0.0) {
                    return 1.0;
                }
                double x = static_cast<double>(idx) /
                           std::max<std::size_t>(1, kBucketCount - 1);
                return std::exp(-lambda * x);
            }
            case DistributionKind::Zipfian: {
                double s = skew_ > 0.0 ? skew_ : 0.0001;
                return 1.0 /
                       std::pow(static_cast<double>(idx) + 1.0, s);
            }
            }
            return 1.0;
        }

        DistributionKind kind_;
        double skew_;
        uint32_t max_key_;
        std::vector<uint32_t> bucket_start_;
        std::vector<uint32_t> bucket_end_;
        std::discrete_distribution<std::size_t> bucket_picker_;
    };

    void print_usage()
    {
        std::cerr
            << "Usage: veb_benchmark "
               "[--distribution=uniform|exponential|zipfian] "
               "[--skew=value] [--num_inserts=N]\n";
    }

    DistributionKind parse_distribution(std::string_view value)
    {
        if (value == "uniform") {
            return DistributionKind::Uniform;
        }
        if (value == "exponential") {
            return DistributionKind::Exponential;
        }
        if (value == "zipfian") {
            return DistributionKind::Zipfian;
        }
        throw std::runtime_error("unknown distribution: " + std::string(value));
    }

    BenchmarkOptions parse_options(int argc, char **argv)
    {
        BenchmarkOptions opts;
        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage();
                std::exit(0);
            }
            else if (arg.rfind("--distribution=", 0) == 0) {
                std::string value(arg.substr(std::strlen("--distribution=")));
                try {
                    opts.distribution = parse_distribution(value);
                }
                catch (std::exception const &ex) {
                    std::cerr << ex.what() << "\n";
                    print_usage();
                    std::exit(1);
                }
            }
            else if (arg.rfind("--skew=", 0) == 0) {
                std::string value(arg.substr(std::strlen("--skew=")));
                opts.skew = std::stod(value);
            }
            else if (arg.rfind("--num_inserts=", 0) == 0) {
                std::string value(arg.substr(std::strlen("--num_inserts=")));
                opts.num_inserts = std::stoull(value);
            }
            else {
                std::cerr << "Unknown argument: " << arg << "\n";
                print_usage();
                std::exit(1);
            }
        }
        if (opts.num_inserts == 0) {
            opts.num_inserts = 1;
        }
        return opts;
    }

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

int main(int argc, char **argv)
{
    auto options = parse_options(argc, argv);
    std::size_t num_inserts = options.num_inserts;

    quill::start();
    quill::preallocate();

    LOG_INFO("=== vEB benchmark: {} inserts ===", num_inserts);
    LOG_INFO(
        "Distribution={}, skew={}",
        to_string(options.distribution), options.skew);

    std::mt19937_64 rng(std::random_device{}());
    DistributionSampler sampler(options.distribution, options.skew, VebTree32::MAX_KEY);

    std::vector<uint32_t> values;
    values.reserve(num_inserts);
    std::vector<uint32_t> successor_queries;
    successor_queries.reserve(num_inserts);
    std::vector<uint32_t> predecessor_queries;
    predecessor_queries.reserve(num_inserts);

    Stopwatch<> data_sw("random data generation");
    for (std::size_t i = 0; i < num_inserts; ++i) {
        values.emplace_back(sampler.sample(rng));
        successor_queries.emplace_back(sampler.sample(rng));
        predecessor_queries.emplace_back(sampler.sample(rng));
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
