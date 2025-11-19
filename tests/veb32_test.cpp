#include <gtest/gtest.h>

#include "veb32.hpp"

TEST(Veb32Test, EmptyUntilInsert)
{
    VebTree32 tree;
    EXPECT_TRUE(tree.empty());

    tree.insert(42);
    EXPECT_FALSE(tree.empty());
}

TEST(Veb32Test, InsertAndContainsEdgeValues)
{
    VebTree32 tree;
    uint32_t const max_key = VebTree32::MAX_KEY;
    tree.insert(0);
    tree.insert(1u << 12);
    tree.insert(1u << 30);
    tree.insert(max_key);

    EXPECT_TRUE(tree.contains(0));
    EXPECT_TRUE(tree.contains(1u << 12));
    EXPECT_TRUE(tree.contains(1u << 30));
    EXPECT_TRUE(tree.contains(max_key));

    tree.erase(1ull << 12);
    EXPECT_FALSE(tree.contains(1ull << 12));
}

TEST(Veb32Test, MinAndMaxReflectGlobalExtremes)
{
    VebTree32 tree;
    uint32_t const max_key = VebTree32::MAX_KEY;
    tree.insert(77);
    tree.insert(1u << 25);
    tree.insert(max_key);
    tree.insert(15);

    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(15u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(max_key, *max_val);
}

TEST(Veb32Test, SuccessorBridgesAcrossWideLevels)
{
    VebTree32 tree;
    tree.insert(5);
    tree.insert(1u << 25);
    uint32_t const high_value = VebTree32::MAX_KEY - 5;
    tree.insert(high_value);

    auto succ = tree.successor(5);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(1u << 25, *succ);

    succ = tree.successor(1ull << 25);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(high_value, *succ);

    EXPECT_FALSE(tree.successor(high_value).has_value());
}

TEST(Veb32Test, PredecessorAcceptsOnePastMax)
{
    VebTree32 tree;
    uint32_t const max_key = VebTree32::MAX_KEY;
    tree.insert(1u << 25);
    tree.insert(max_key);

    auto pred = tree.predecessor(max_key);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(1ull << 25, *pred);

    pred = tree.predecessor(static_cast<uint64_t>(max_key) + 1);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(max_key, *pred);
}
