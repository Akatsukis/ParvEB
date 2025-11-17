#include <gtest/gtest.h>

#include "veb40.hpp"

TEST(Veb40Test, EmptyUntilInsert)
{
    VebTree40 tree;
    EXPECT_TRUE(tree.empty());

    tree.insert(42);
    EXPECT_FALSE(tree.empty());
}

TEST(Veb40Test, InsertAndContainsEdgeValues)
{
    VebTree40 tree;
    uint64_t const max_key = VebTree40::MAX_KEY;
    tree.insert(0);
    tree.insert(1ull << 12);
    tree.insert(1ull << 30);
    tree.insert(max_key);

    EXPECT_TRUE(tree.contains(0));
    EXPECT_TRUE(tree.contains(1ull << 12));
    EXPECT_TRUE(tree.contains(1ull << 30));
    EXPECT_TRUE(tree.contains(max_key));

    tree.erase(1ull << 12);
    EXPECT_FALSE(tree.contains(1ull << 12));
}

TEST(Veb40Test, MinAndMaxReflectGlobalExtremes)
{
    VebTree40 tree;
    uint64_t const max_key = VebTree40::MAX_KEY;
    tree.insert(77);
    tree.insert(1ull << 25);
    tree.insert(max_key);
    tree.insert(15);

    auto min_val = tree.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(15u, *min_val);

    auto max_val = tree.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(max_key, *max_val);
}

TEST(Veb40Test, SuccessorBridgesAcrossWideLevels)
{
    VebTree40 tree;
    tree.insert(5);
    tree.insert(1ull << 25);
    tree.insert((1ull << 32) + 7);

    auto succ = tree.successor(5);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(1ull << 25, *succ);

    succ = tree.successor(1ull << 25);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ((1ull << 32) + 7, *succ);

    EXPECT_FALSE(tree.successor((1ull << 32) + 7).has_value());
}

TEST(Veb40Test, PredecessorAcceptsOnePastMax)
{
    VebTree40 tree;
    uint64_t const max_key = VebTree40::MAX_KEY;
    tree.insert(1ull << 25);
    tree.insert(max_key);

    auto pred = tree.predecessor(max_key);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(1ull << 25, *pred);

    pred = tree.predecessor(max_key + 1);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(max_key, *pred);
}
