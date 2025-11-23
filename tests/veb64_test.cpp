#include <algorithm>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "veb_branch.hpp"

using Veb64 = VebTop64;

TEST(Veb64Test, InsertContainsAndErase)
{
    Veb64 tree;
    uint64_t const max_key = Veb64::MAX_KEY;

    uint64_t low = 17;
    uint64_t mid = (uint64_t(1) << 40) + 5;
    uint64_t high = (uint64_t(1) << 55) + 99;

    tree.insert(low);
    tree.insert(mid);
    tree.insert(high);
    tree.insert(max_key);

    EXPECT_TRUE(tree.contains(low));
    EXPECT_TRUE(tree.contains(mid));
    EXPECT_TRUE(tree.contains(high));
    EXPECT_TRUE(tree.contains(max_key));

    tree.erase(mid);
    EXPECT_FALSE(tree.contains(mid));
}

TEST(Veb64Test, SuccessorAndPredecessor)
{
    Veb64 tree;
    uint64_t a = (uint64_t(1) << 33) + 1;
    uint64_t b = (uint64_t(1) << 48);
    uint64_t c = (uint64_t(1) << 60) + 7;

    tree.insert(a);
    tree.insert(b);
    tree.insert(c);

    auto succ = tree.successor(a);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(b, *succ);

    succ = tree.successor(b);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(c, *succ);
    EXPECT_FALSE(tree.successor(c).has_value());

    auto pred = tree.predecessor(b);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(a, *pred);
    pred = tree.predecessor(c);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(b, *pred);
    EXPECT_FALSE(tree.predecessor(0).has_value());
}

TEST(Veb64Test, MinMaxAndOrdering)
{
    Veb64 tree;
    std::vector<uint64_t> values = {
        3,
        (uint64_t(1) << 36) + 10,
        (uint64_t(1) << 60),
        Veb64::MAX_KEY,
    };
    for (auto v : values) {
        tree.insert(v);
    }

    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(3u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(Veb64::MAX_KEY, *max_val);

    std::vector<uint64_t> out;
    tree.for_each([&](uint64_t k) { out.push_back(k); });
    auto expected = values;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(expected, out);
}
