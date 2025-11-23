#include <gtest/gtest.h>

#include "veb_leaf6.hpp"

TEST(Leaf6Test, InsertUpdatesContains)
{
    VebLeaf6 leaf;
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

TEST(Leaf6Test, BatchInsertErase)
{
    VebLeaf6 leaf;
    std::array<VebLeaf6::Key, 5> keys = {0, 5, 17, 42, 63};
    leaf.batch_insert(keys);
    for (auto k : keys) {
        EXPECT_TRUE(leaf.contains(k));
    }

    std::array<VebLeaf6::Key, 3> erase_keys = {5, 42, 63};
    leaf.batch_erase(erase_keys);
    EXPECT_TRUE(leaf.contains(0));
    EXPECT_TRUE(leaf.contains(17));
    EXPECT_FALSE(leaf.contains(5));
    EXPECT_FALSE(leaf.contains(42));
    EXPECT_FALSE(leaf.contains(63));
}

TEST(Leaf6Test, MinMaxTrackExtremes)
{
    VebLeaf6 leaf;
    leaf.insert(5);
    leaf.insert(1);
    leaf.insert(60);

    ASSERT_TRUE(leaf.min().has_value());
    EXPECT_EQ(1u, *leaf.min());
    ASSERT_TRUE(leaf.max().has_value());
    EXPECT_EQ(60u, *leaf.max());
}

TEST(Leaf6Test, EraseClearsStructure)
{
    VebLeaf6 leaf;
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

TEST(Leaf6Test, SuccessorReturnsNextHigherValue)
{
    VebLeaf6 leaf;
    leaf.insert(2);
    leaf.insert(10);
    leaf.insert(42);

    ASSERT_TRUE(leaf.successor(2).has_value());
    EXPECT_EQ(10u, *leaf.successor(2));
    ASSERT_TRUE(leaf.successor(10).has_value());
    EXPECT_EQ(42u, *leaf.successor(10));
}

TEST(Leaf6Test, SuccessorReturnsNullWhenNoGreaterValue)
{
    VebLeaf6 leaf;
    leaf.insert(3);
    leaf.insert(9);

    EXPECT_FALSE(leaf.successor(9).has_value());
    EXPECT_FALSE(leaf.successor(63).has_value());
}

TEST(Leaf6Test, SuccessorReflectsLaterInsertions)
{
    VebLeaf6 leaf;
    leaf.insert(20);
    leaf.insert(50);

    EXPECT_FALSE(leaf.successor(50).has_value());

    leaf.insert(60);
    ASSERT_TRUE(leaf.successor(50).has_value());
    EXPECT_EQ(60u, *leaf.successor(50));
}

TEST(Leaf6Test, PredecessorOfZeroHasNoValue)
{
    VebLeaf6 leaf;
    leaf.insert(7);
    EXPECT_FALSE(leaf.predecessor(0).has_value());
}

TEST(Leaf6Test, PredecessorReturnsLargestSmallerValue)
{
    VebLeaf6 leaf;
    leaf.insert(0);
    leaf.insert(8);
    leaf.insert(17);

    ASSERT_TRUE(leaf.predecessor(1).has_value());
    EXPECT_EQ(0u, *leaf.predecessor(1));
    ASSERT_TRUE(leaf.predecessor(17).has_value());
    EXPECT_EQ(8u, *leaf.predecessor(17));
}

TEST(Leaf6Test, PredecessorHandlesUpperBound)
{
    VebLeaf6 leaf;
    auto const max_key = VebLeaf6::MAX_KEY;
    leaf.insert(0);
    leaf.insert(17);
    leaf.insert(max_key);

    ASSERT_TRUE(leaf.predecessor(max_key).has_value());
    EXPECT_EQ(17u, *leaf.predecessor(max_key));
}
