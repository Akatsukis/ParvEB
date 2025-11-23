#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
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
#include "veb24.hpp"
#include "veb32.hpp"
#include "veb48.hpp"
#include "veb64.hpp"

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

    enum class KeyMode
    {
        Bits24,
        Bits32,
        Bits48,
        Bits64
    };

    struct BenchmarkOptions
    {
        DistributionKind distribution = DistributionKind::Uniform;
        double skew = 1.0;
        std::size_t num_inserts = 10'000'000;
        KeyMode key_mode = KeyMode::Bits48;
    };

    template <class Key, unsigned BitCount>
    class DistributionSampler
    {
    public:
        DistributionSampler(DistributionKind kind, double skew)
            : kind_(kind)
            , skew_(skew)
        {
            initialize();
        }

        Key sample(std::mt19937_64 &rng)
        {
            Key value = 0;
            for (std::size_t bit = 0; bit < BitCount; ++bit) {
                if (!bit_distributions_[bit](rng)) {
                    value |= (Key(1) << bit);
                }
            }
            return value;
        }

    private:
        void initialize()
        {
            bit_distributions_.clear();
            bit_distributions_.reserve(BitCount);
            for (std::size_t bit = 0; bit < BitCount; ++bit) {
                double prob_zero = compute_zero_probability(bit);
                prob_zero = std::clamp(prob_zero, 0.0001, 0.9999);
                bit_distributions_.emplace_back(prob_zero);
            }
        }

        double compute_zero_probability(std::size_t bit_index) const
        {
            switch (kind_) {
            case DistributionKind::Uniform:
                return 0.5;
            case DistributionKind::Exponential: {
                double lambda = skew_ > 0.0 ? skew_ : 0.0001;
                return std::exp(-lambda * static_cast<double>(bit_index));
            }
            case DistributionKind::Zipfian: {
                double s = skew_ > 0.0 ? skew_ : 0.0001;
                return 1.0 / std::pow(static_cast<double>(bit_index) + 1.0, s);
            }
            }
            return 0.5;
        }

        DistributionKind kind_;
        double skew_;
        std::vector<std::bernoulli_distribution> bit_distributions_;
    };

    void print_usage()
    {
        std::cerr << "Usage: veb_benchmark "
                     "[--distribution=uniform|exponential|zipfian] "
                     "[--bits=24|32|48|64] "
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

    KeyMode parse_key_mode(std::string_view value)
    {
        if (value == "24") {
            return KeyMode::Bits24;
        }
        if (value == "32") {
            return KeyMode::Bits32;
        }
        if (value == "48") {
            return KeyMode::Bits48;
        }
        if (value == "64") {
            return KeyMode::Bits64;
        }
        throw std::runtime_error("unknown bit width: " + std::string(value));
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
            else if (arg.rfind("--bits=", 0) == 0) {
                std::string value(arg.substr(std::strlen("--bits=")));
                try {
                    opts.key_mode = parse_key_mode(value);
                }
                catch (std::exception const &ex) {
                    std::cerr << ex.what() << "\n";
                    print_usage();
                    std::exit(1);
                }
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

    template <class Key>
    std::optional<Key> predecessor_from_set(std::set<Key> const &data, Key key)
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

    template <class Set, class Key>
    std::optional<Key> successor_from_ordered(Set const &data, Key key)
    {
        auto it = data.upper_bound(key);
        if (it == data.end()) {
            return std::nullopt;
        }
        return *it;
    }

    template <class Set, class Key>
    std::optional<Key> predecessor_from_ordered(Set const &data, Key key)
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
        Stopwatch<> &sw, std::string_view label, QueryVec const &queries,
        Fn &&fn) -> std::vector<decltype(fn(queries[0]))>
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

    template <class Tree, unsigned BitCount>
    void run_benchmark_for_tree(BenchmarkOptions const &options)
    {
        using Key = typename Tree::Key;
        std::size_t num_inserts = options.num_inserts;

        LOG_INFO(
            "=== vEB benchmark: {} inserts ({}-bit) ===",
            num_inserts,
            BitCount);
        LOG_INFO(
            "Distribution={}, skew={}",
            to_string(options.distribution),
            options.skew);

        std::mt19937_64 rng(std::random_device{}());
        DistributionSampler<Key, BitCount> sampler(
            options.distribution, options.skew);

        std::vector<Key> values;
        values.reserve(num_inserts);
        std::vector<Key> successor_queries;
        successor_queries.reserve(num_inserts);
        std::vector<Key> predecessor_queries;
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
                std::is_sorted(data.begin(), data.end()) &&
                "data must be sorted");
        };

        // std::set baseline (also generates expected answers)
        LOG_INFO("--- std::set ---");
        Stopwatch<> std_sw("std::set");
        std::set<Key> std_set;
        for (Key value : values) {
            std_set.insert(value);
        }
        std_sw.next("insert");
        std::vector<Key> std_sorted(std_set.begin(), std_set.end());
        assert_sorted("std::set", std_sorted);

        std::vector<std::optional<Key>> expected_successors(
            successor_queries.size());
        for (std::size_t i = 0; i < successor_queries.size(); ++i) {
            expected_successors[i] = successor_from_ordered<std::set<Key>, Key>(
                std_set, successor_queries[i]);
        }
        std_sw.next("successor");

        std::vector<std::optional<Key>> expected_predecessors(
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
        Tree tree;
        for (Key value : values) {
            tree.insert(value);
        }
        veb_sw.next("insert");
        std::vector<Key> veb_sorted = tree.to_vector();
        assert_sorted("vEB", veb_sorted);
        assert(veb_sorted == std_sorted);
        auto veb_successors = collect_queries(
            veb_sw, "successor", successor_queries, [&](Key key) {
                return tree.successor(key);
            });
        auto veb_predecessors = collect_queries(
            veb_sw, "predecessor", predecessor_queries, [&](Key key) {
                return tree.predecessor(static_cast<uint64_t>(key));
            });
        assert(veb_successors == expected_successors);
        assert(veb_predecessors == expected_predecessors);
        veb_sw.total_time();

        // absl::btree_set
        LOG_INFO("--- absl::btree_set ---");
        Stopwatch<> absl_sw("absl::btree_set");
        absl::btree_set<Key> absl_set;
        for (Key value : values) {
            absl_set.insert(value);
        }
        absl_sw.next("insert");
        std::vector<Key> absl_sorted(absl_set.begin(), absl_set.end());
        assert_sorted("absl::btree_set", absl_sorted);
        assert(absl_sorted == std_sorted);
        auto absl_successors = collect_queries(
            absl_sw, "successor", successor_queries, [&](Key key) {
                return successor_from_ordered<absl::btree_set<Key>, Key>(
                    absl_set, key);
            });
        auto absl_predecessors = collect_queries(
            absl_sw, "predecessor", predecessor_queries, [&](Key key) {
                return predecessor_from_ordered<absl::btree_set<Key>, Key>(
                    absl_set, key);
            });
        assert(absl_successors == expected_successors);
        assert(absl_predecessors == expected_predecessors);
        absl_sw.total_time();

        // boost::container::set
        LOG_INFO("--- boost::container::set ---");
        Stopwatch<> boost_sw("boost::container::set");
        boost::container::set<Key> boost_set;
        for (Key value : values) {
            boost_set.insert(value);
        }
        boost_sw.next("insert");
        std::vector<Key> boost_sorted(boost_set.begin(), boost_set.end());
        assert_sorted("boost::container::set", boost_sorted);
        assert(boost_sorted == std_sorted);
        auto boost_successors = collect_queries(
            boost_sw, "successor", successor_queries, [&](Key key) {
                return successor_from_ordered<boost::container::set<Key>, Key>(
                    boost_set, key);
            });
        auto boost_predecessors = collect_queries(
            boost_sw, "predecessor", predecessor_queries, [&](Key key) {
                return predecessor_from_ordered<
                    boost::container::set<Key>,
                    Key>(boost_set, key);
            });
        assert(boost_successors == expected_successors);
        assert(boost_predecessors == expected_predecessors);
        boost_sw.total_time();

        LOG_INFO("Benchmark complete");
    }

} // namespace

int main(int argc, char **argv)
{
    auto options = parse_options(argc, argv);
    quill::start();
    quill::preallocate();

    switch (options.key_mode) {
    case KeyMode::Bits24:
        run_benchmark_for_tree<VebTree24, 24>(options);
        break;
    case KeyMode::Bits32:
        run_benchmark_for_tree<VebTree32, 32>(options);
        break;
    case KeyMode::Bits48:
        run_benchmark_for_tree<VebTree48, 48>(options);
        break;
    case KeyMode::Bits64:
        run_benchmark_for_tree<VebTree64, 64>(options);
        break;
    }
    return 0;
}
