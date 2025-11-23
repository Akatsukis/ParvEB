#include <algorithm>
#include <vector>

#include <gtest/gtest.h>

#include "veb_branch.hpp"

using Veb32 = VebTop32;

TEST(Veb32Test, InsertContainsAndErase)
{
    Veb32 tree;
    auto const max_key = Veb32::MAX_KEY;

    uint32_t low = 17;
    uint32_t mid = (uint32_t(1) << 20) + 5;
    uint32_t high = (uint32_t(1) << 28) + 99;

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

TEST(Veb32Test, SuccessorAndPredecessor)
{
    Veb32 tree;
    uint32_t a = (uint32_t(1) << 12) + 1;
    uint32_t b = (uint32_t(1) << 24);
    uint32_t c = (uint32_t(1) << 30) + 7;

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

TEST(Veb32Test, MinMaxAndOrdering)
{
    Veb32 tree;
    std::vector<uint32_t> values = {
        3,
        (uint32_t(1) << 18) + 10,
        (uint32_t(1) << 30),
        Veb32::MAX_KEY,
    };
    for (auto v : values) {
        tree.insert(v);
    }

    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(3u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(Veb32::MAX_KEY, *max_val);

    std::vector<uint32_t> out;
    tree.for_each([&](uint32_t k) { out.push_back(k); });
    auto expected = values;
    std::sort(expected.begin(), expected.end());
    EXPECT_EQ(expected, out);
}
