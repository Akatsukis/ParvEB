#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "veb24.hpp"
#include "veb32.hpp"
#include "veb48.hpp"
#include "veb64.hpp"

namespace
{

    struct RunOptions
    {
        std::uint64_t num_inserts = 10'000'000ULL;
        int trials = 5;
        std::uint64_t seed = 0;
        unsigned bits = 48;
    };

    void print_usage()
    {
        std::cerr << "Usage: run_veb [--num_inserts=N] [--trials=T] [--seed=S] "
                     "[--bits=24|32|48|64]\n";
    }

    RunOptions parse_options(int argc, char **argv)
    {
        RunOptions opts;
        for (int i = 1; i < argc; ++i) {
            std::string_view arg(argv[i]);
            if (arg == "--help" || arg == "-h") {
                print_usage();
                std::exit(0);
            }
            if (arg.rfind("--num_inserts=", 0) == 0) {
                std::string value(
                    arg.substr(std::string_view("--num_inserts=").size()));
                opts.num_inserts = std::stoull(value);
            }
            else if (arg.rfind("--trials=", 0) == 0) {
                std::string value(
                    arg.substr(std::string_view("--trials=").size()));
                opts.trials = std::stoi(value);
            }
            else if (arg.rfind("--seed=", 0) == 0) {
                std::string value(
                    arg.substr(std::string_view("--seed=").size()));
                opts.seed = std::stoull(value);
            }
            else if (arg.rfind("--bits=", 0) == 0) {
                std::string value(
                    arg.substr(std::string_view("--bits=").size()));
                if (value == "24") {
                    opts.bits = 24;
                }
                else if (value == "32") {
                    opts.bits = 32;
                }
                else if (value == "48") {
                    opts.bits = 48;
                }
                else if (value == "64") {
                    opts.bits = 64;
                }
                else {
                    throw std::invalid_argument("bits must be 48 or 64");
                }
            }
            else {
                throw std::invalid_argument(
                    "Unknown argument: " + std::string(arg));
            }
        }

        if (opts.trials <= 0) {
            throw std::invalid_argument("trials must be positive");
        }
        if (opts.num_inserts == 0) {
            throw std::invalid_argument("num_inserts must be positive");
        }
        if (opts.num_inserts > std::numeric_limits<std::size_t>::max()) {
            throw std::invalid_argument(
                "num_inserts exceeds available memory on this platform");
        }
        if (opts.num_inserts > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument(
                "num_inserts must fit in 32-bit buffer");
        }
        if (opts.bits != 24 && opts.bits != 32 && opts.bits != 48 &&
            opts.bits != 64) {
            throw std::invalid_argument("bits must be 24, 32, 48 or 64");
        }

        return opts;
    }

    double seconds_between(auto start, auto end)
    {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
                   end - start)
            .count();
    }

    template <class Tree, class KeyT>
    void run_trials(
        Tree &&, int trials, std::vector<KeyT> const &keys, double gen_secs)
    {
        for (int trial = 1; trial <= trials; ++trial) {
            std::cout << "\nTrial " << trial << "/" << trials << "\n";
            Tree tree;
            auto insert_start = std::chrono::steady_clock::now();
            for (auto key : keys) {
                tree.insert(static_cast<typename Tree::Key>(key));
            }
            auto insert_end = std::chrono::steady_clock::now();

            auto min_key = tree.min();
            auto max_key = tree.max();

            double insert_secs = seconds_between(insert_start, insert_end);

            std::cout << "insert=" << insert_secs
                      << "s (generate once: " << gen_secs << "s)\n";

            if (!min_key || !max_key) {
                std::cerr << "Warning: tree is empty after insertions\n";
            }
        }
    }

} // namespace

int main(int argc, char **argv)
{
    RunOptions opts;
    try {
        opts = parse_options(argc, argv);
    }
    catch (std::exception const &ex) {
        std::cerr << ex.what() << "\n";
        print_usage();
        return 1;
    }

    std::cout << "vEB insert benchmark\n";
    std::cout << "num_inserts=" << opts.num_inserts << " trials=" << opts.trials
              << "\n";
    std::cout << "seed=" << opts.seed
              << " (uniform draw reused across trials)\n";
    std::cout << "bits=" << opts.bits << "\n";
    std::cout << std::fixed << std::setprecision(3);

    std::mt19937_64 rng(opts.seed);
    auto gen_start = std::chrono::steady_clock::now();

    switch (opts.bits) {
    case 24: {
        std::uniform_int_distribution<VebTree24::Key> dist(
            0, VebTree24::MAX_KEY);
        std::vector<VebTree24::Key> keys(
            static_cast<std::size_t>(opts.num_inserts));
        for (auto &k : keys) {
            k = dist(rng);
        }
        auto gen_end = std::chrono::steady_clock::now();
        double gen_secs = seconds_between(gen_start, gen_end);
        std::cout << "generate_uniform=" << gen_secs << "s\n";
        run_trials(VebTree24{}, opts.trials, keys, gen_secs);
        break;
    }
    case 32: {
        std::uniform_int_distribution<VebTree32::Key> dist(
            0, VebTree32::MAX_KEY);
        std::vector<VebTree32::Key> keys(
            static_cast<std::size_t>(opts.num_inserts));
        for (auto &k : keys) {
            k = dist(rng);
        }
        auto gen_end = std::chrono::steady_clock::now();
        double gen_secs = seconds_between(gen_start, gen_end);
        std::cout << "generate_uniform=" << gen_secs << "s\n";
        run_trials(VebTree32{}, opts.trials, keys, gen_secs);
        break;
    }
    case 48: {
        std::uniform_int_distribution<VebTree48::Key> dist(
            0, VebTree48::MAX_KEY);
        std::vector<VebTree48::Key> keys(
            static_cast<std::size_t>(opts.num_inserts));
        for (auto &k : keys) {
            k = dist(rng);
        }
        auto gen_end = std::chrono::steady_clock::now();
        double gen_secs = seconds_between(gen_start, gen_end);
        std::cout << "generate_uniform=" << gen_secs << "s\n";
        run_trials(VebTree48{}, opts.trials, keys, gen_secs);
        break;
    }
    case 64: {
        std::uniform_int_distribution<VebTree64::Key> dist(
            0, VebTree64::MAX_KEY);
        std::vector<VebTree64::Key> keys(
            static_cast<std::size_t>(opts.num_inserts));
        for (auto &k : keys) {
            k = dist(rng);
        }
        auto gen_end = std::chrono::steady_clock::now();
        double gen_secs = seconds_between(gen_start, gen_end);
        std::cout << "generate_uniform=" << gen_secs << "s\n";
        run_trials(VebTree64{}, opts.trials, keys, gen_secs);
        break;
    }
    default:
        std::cerr << "Unsupported bits value\n";
        return 1;
    }

    return 0;
}
