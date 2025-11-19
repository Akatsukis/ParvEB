#include <gtest/gtest.h>

#include "veb_leaf8.hpp"

TEST(Leaf8Test, InsertUpdatesContains)
{
    VebLeaf8 leaf;
    EXPECT_TRUE(leaf.empty());
    leaf.insert(0);
    leaf.insert(5);
    leaf.insert(63);

    EXPECT_FALSE(leaf.empty());
    EXPECT_TRUE(leaf.contains(0));
    EXPECT_TRUE(leaf.contains(5));
    EXPECT_TRUE(leaf.contains(63));
    EXPECT_FALSE(leaf.contains(7));
}

TEST(Leaf8Test, MinMaxTrackExtremes)
{
    VebLeaf8 leaf;
    leaf.insert(5);
    leaf.insert(1);
    leaf.insert(60);

    ASSERT_TRUE(leaf.min().has_value());
    EXPECT_EQ(1u, *leaf.min());
    ASSERT_TRUE(leaf.max().has_value());
    EXPECT_EQ(60u, *leaf.max());
}

TEST(Leaf8Test, EraseClearsStructure)
{
    VebLeaf8 leaf;
    leaf.insert(0);
    leaf.insert(5);
    leaf.insert(63);

    leaf.erase(0);
    EXPECT_FALSE(leaf.contains(0));
    ASSERT_TRUE(leaf.min().has_value());
    EXPECT_EQ(5u, *leaf.min());

    leaf.erase(5);
    leaf.erase(63);
    EXPECT_TRUE(leaf.empty());
    EXPECT_FALSE(leaf.min().has_value());
    EXPECT_FALSE(leaf.max().has_value());
}

TEST(Leaf8Test, SuccessorReturnsNextHigherValue)
{
    VebLeaf8 leaf;
    leaf.insert(2);
    leaf.insert(10);
    leaf.insert(42);

    ASSERT_TRUE(leaf.successor(2).has_value());
    EXPECT_EQ(10u, *leaf.successor(2));
    ASSERT_TRUE(leaf.successor(10).has_value());
    EXPECT_EQ(42u, *leaf.successor(10));
}

TEST(Leaf8Test, SuccessorReturnsNullWhenNoGreaterValue)
{
    VebLeaf8 leaf;
    leaf.insert(3);
    leaf.insert(9);

    EXPECT_FALSE(leaf.successor(9).has_value());
    EXPECT_FALSE(leaf.successor(63).has_value());
}

TEST(Leaf8Test, SuccessorReflectsLaterInsertions)
{
    VebLeaf8 leaf;
    leaf.insert(20);
    leaf.insert(50);

    EXPECT_FALSE(leaf.successor(50).has_value());

    leaf.insert(60);
    ASSERT_TRUE(leaf.successor(50).has_value());
    EXPECT_EQ(60u, *leaf.successor(50));
}

TEST(Leaf8Test, PredecessorOfZeroHasNoValue)
{
    VebLeaf8 leaf;
    leaf.insert(7);
    EXPECT_FALSE(leaf.predecessor(0).has_value());
}

TEST(Leaf8Test, PredecessorReturnsLargestSmallerValue)
{
    VebLeaf8 leaf;
    leaf.insert(0);
    leaf.insert(8);
    leaf.insert(17);

    ASSERT_TRUE(leaf.predecessor(1).has_value());
    EXPECT_EQ(0u, *leaf.predecessor(1));
    ASSERT_TRUE(leaf.predecessor(17).has_value());
    EXPECT_EQ(8u, *leaf.predecessor(17));
}

TEST(Leaf8Test, PredecessorTreats256AsInclusiveUpperBound)
{
    VebLeaf8 leaf;
    leaf.insert(0);
    leaf.insert(17);
    leaf.insert(255);

    ASSERT_TRUE(leaf.predecessor(256).has_value());
    EXPECT_EQ(255u, *leaf.predecessor(256));
}
