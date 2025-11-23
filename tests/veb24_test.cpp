#include <gtest/gtest.h>

#include "veb24.hpp"

TEST(Veb24Test, EmptyUntilInsert)
{
    VebTree24 tree;
    EXPECT_TRUE(tree.empty());

    tree.insert(42);
    EXPECT_FALSE(tree.empty());
}

TEST(Veb24Test, InsertAndContainsEdgeValues)
{
    VebTree24 tree;
    auto const max_key = VebTree24::MAX_KEY;
    tree.insert(0);
    tree.insert(1u << 12);
    tree.insert(1u << 18);
    tree.insert(max_key);

    EXPECT_TRUE(tree.contains(0));
    EXPECT_TRUE(tree.contains(1u << 12));
    EXPECT_TRUE(tree.contains(1u << 18));
    EXPECT_TRUE(tree.contains(max_key));

    tree.erase(1ull << 12);
    EXPECT_FALSE(tree.contains(1ull << 12));
}

TEST(Veb24Test, MinAndMaxReflectGlobalExtremes)
{
    VebTree24 tree;
    auto const max_key = VebTree24::MAX_KEY;
    tree.insert(77);
    tree.insert(1u << 23);
    tree.insert(max_key);
    tree.insert(15);

    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(15u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(max_key, *max_val);
}

TEST(Veb24Test, SuccessorBridgesAcrossWideLevels)
{
    VebTree24 tree;
    tree.insert(5);
    tree.insert(1u << 23);
    auto const high_value = VebTree24::MAX_KEY - 5;
    tree.insert(high_value);

    auto succ = tree.successor(5);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(1u << 23, *succ);

    succ = tree.successor(1ull << 23);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(high_value, *succ);

    EXPECT_FALSE(tree.successor(high_value).has_value());
}

TEST(Veb24Test, PredecessorAtMax)
{
    VebTree24 tree;
    auto const max_key = VebTree24::MAX_KEY;
    tree.insert(1u << 23);
    tree.insert(max_key);

    auto pred = tree.predecessor(max_key);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(1ull << 23, *pred);
}
