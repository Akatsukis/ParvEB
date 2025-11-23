#include <algorithm>
#include <limits>
#include <vector>

#include <gtest/gtest.h>

#include "veb48.hpp"

TEST(Veb48Test, InsertContainsAndErase)
{
    VebTree48 tree;
    uint64_t const max_key = VebTree48::MAX_KEY;

    uint64_t low = 17;
    uint64_t mid = (uint64_t(1) << 40) + 5;
    uint64_t high = (uint64_t(1) << 42) + 99;

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

TEST(Veb48Test, SuccessorSpansSparseClusters)
{
    VebTree48 tree;
    uint64_t a = (uint64_t(1) << 33) + 1;
    uint64_t b = (uint64_t(1) << 42);
    uint64_t c = (uint64_t(1) << 44) + 7;

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
}

TEST(Veb48Test, PredecessorHandlesLargeKeys)
{
    VebTree48 tree;
    uint64_t base = (uint64_t(1) << 47);
    tree.insert(base - 2);
    tree.insert(base + (uint64_t(1) << 32));

    auto pred = tree.predecessor(base);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(base - 2, *pred);

    pred = tree.predecessor((1ull << 48) - 1);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(base + (uint64_t(1) << 32), *pred);

    EXPECT_FALSE(tree.predecessor(0).has_value());
}

TEST(Veb48Test, MinMaxAndOrdering)
{
    VebTree48 tree;
    std::vector<uint64_t> values = {
        3,
        (uint64_t(1) << 36) + 10,
        (uint64_t(1) << 47),
        (1ull << 48) - 1,
    };
    for (auto v : values) {
        tree.insert(v);
    }
    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(3u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ((1ull << 48) - 1, *max_val);

    auto vec = tree.to_vector();
    auto expected = values;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(expected, vec);
}
