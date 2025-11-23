#include <gtest/gtest.h>

#include "veb_leaf8.hpp"

TEST(Leaf8Test, InsertUpdatesContains)
{
    VebLeaf8 leaf;
    EXPECT_TRUE(leaf.empty());

    leaf.insert(0);
    leaf.insert(5);
    leaf.insert(63);
    leaf.insert(64);
    leaf.insert(129);
    leaf.insert(255);

    EXPECT_FALSE(leaf.empty());
    EXPECT_TRUE(leaf.contains(0));
    EXPECT_TRUE(leaf.contains(5));
    EXPECT_TRUE(leaf.contains(63));
    EXPECT_TRUE(leaf.contains(64));
    EXPECT_TRUE(leaf.contains(129));
    EXPECT_TRUE(leaf.contains(255));
    EXPECT_FALSE(leaf.contains(200));
}

TEST(Leaf8Test, MinMaxTrackExtremes)
{
    VebLeaf8 leaf;
    leaf.insert(5);
    leaf.insert(1);
    leaf.insert(130);
    leaf.insert(250);

    ASSERT_TRUE(leaf.min().has_value());
    EXPECT_EQ(1u, *leaf.min());
    ASSERT_TRUE(leaf.max().has_value());
    EXPECT_EQ(250u, *leaf.max());
}

TEST(Leaf8Test, EraseClearsStructure)
{
    VebLeaf8 leaf;
    leaf.insert(0);
    leaf.insert(5);
    leaf.insert(255);

    leaf.erase(0);
    EXPECT_FALSE(leaf.contains(0));
    ASSERT_TRUE(leaf.min().has_value());
    EXPECT_EQ(5u, *leaf.min());

    leaf.erase(5);
    leaf.erase(255);
    EXPECT_TRUE(leaf.empty());
    EXPECT_FALSE(leaf.min().has_value());
    EXPECT_FALSE(leaf.max().has_value());
}

TEST(Leaf8Test, SuccessorAcrossWords)
{
    VebLeaf8 leaf;
    leaf.insert(2);
    leaf.insert(10);
    leaf.insert(70);
    leaf.insert(130);
    leaf.insert(200);

    ASSERT_TRUE(leaf.successor(2).has_value());
    EXPECT_EQ(10u, *leaf.successor(2));
    ASSERT_TRUE(leaf.successor(10).has_value());
    EXPECT_EQ(70u, *leaf.successor(10));
    ASSERT_TRUE(leaf.successor(70).has_value());
    EXPECT_EQ(130u, *leaf.successor(70));
    ASSERT_TRUE(leaf.successor(130).has_value());
    EXPECT_EQ(200u, *leaf.successor(130));
    EXPECT_FALSE(leaf.successor(200).has_value());
    EXPECT_FALSE(leaf.successor(255).has_value());
}

TEST(Leaf8Test, PredecessorAcrossWords)
{
    VebLeaf8 leaf;
    leaf.insert(2);
    leaf.insert(70);
    leaf.insert(130);
    leaf.insert(200);

    auto pred = leaf.predecessor(70);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);

    pred = leaf.predecessor(130);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(70u, *pred);

    pred = leaf.predecessor(200);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(130u, *pred);

    EXPECT_FALSE(leaf.predecessor(0).has_value());
}

TEST(Leaf8Test, PredecessorHandlesUpperBound)
{
    VebLeaf8 leaf;
    auto const max_key = VebLeaf8::MAX_KEY;
    leaf.insert(0);
    leaf.insert(17);
    leaf.insert(max_key);

    ASSERT_TRUE(leaf.predecessor(max_key).has_value());
    EXPECT_EQ(17u, *leaf.predecessor(max_key));
}
