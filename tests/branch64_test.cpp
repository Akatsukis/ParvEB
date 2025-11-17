#include <gtest/gtest.h>

#include "veb_branch.hpp"
#include "veb_leaf64.hpp"

using LeafBranch = Branch64<Leaf64, 6>;

TEST(Branch64Test, StartsEmptyUntilInsert)
{
    LeafBranch branch;
    EXPECT_TRUE(branch.empty());

    branch.insert(0);
    EXPECT_FALSE(branch.empty());
}

TEST(Branch64Test, ContainsRecognizesValuesAcrossChildren)
{
    LeafBranch branch;
    branch.insert(0);
    branch.insert(63);
    branch.insert(64);
    branch.insert(130);

    EXPECT_TRUE(branch.contains(0));
    EXPECT_TRUE(branch.contains(63));
    EXPECT_TRUE(branch.contains(64));
    EXPECT_TRUE(branch.contains(130));
    EXPECT_FALSE(branch.contains(5u << 6));
}

TEST(Branch64Test, MinTracksGlobalMinimum)
{
    LeafBranch branch;
    branch.insert(64);
    branch.insert(5);
    branch.insert(130);

    auto min_val = branch.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(5u, *min_val);
}

TEST(Branch64Test, MaxTracksGlobalMaximum)
{
    LeafBranch branch;
    branch.insert(63);
    branch.insert(70);
    branch.insert(130);

    auto max_val = branch.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(130u, *max_val);
}

TEST(Branch64Test, EraseUpdatesMinWhenLowestLeafRemoved)
{
    LeafBranch branch;
    branch.insert(5);
    branch.insert(70);
    branch.insert(130);

    branch.erase(5);
    auto min_val = branch.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(70u, *min_val);

    branch.erase(70);
    min_val = branch.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(130u, *min_val);
}

TEST(Branch64Test, EraseRemovesChildrenAndCanBecomeEmpty)
{
    LeafBranch branch;
    branch.insert(5);
    branch.insert(70);

    branch.erase(5);
    EXPECT_FALSE(branch.empty());

    branch.erase(70);
    EXPECT_TRUE(branch.empty());
    EXPECT_FALSE(branch.min().has_value());
    EXPECT_FALSE(branch.contains(70));
}

TEST(Branch64Test, SuccessorWithinSameChild)
{
    LeafBranch branch;
    branch.insert(2);
    branch.insert(10);

    auto succ = branch.successor(2);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(10u, *succ);
}

TEST(Branch64Test, SuccessorAcrossChildBoundary)
{
    LeafBranch branch;
    branch.insert(63);
    branch.insert(70);

    auto succ = branch.successor(63);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(70u, *succ);
}

TEST(Branch64Test, SuccessorReturnsNullAfterLargestElement)
{
    LeafBranch branch;
    branch.insert(190);

    EXPECT_FALSE(branch.successor(190).has_value());
    EXPECT_FALSE(branch.successor(0x3ff).has_value());
}

TEST(Branch64Test, PredecessorWithinSameChild)
{
    LeafBranch branch;
    branch.insert(2);
    branch.insert(10);

    auto pred = branch.predecessor(10);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch64Test, PredecessorAcrossChildBoundary)
{
    LeafBranch branch;
    branch.insert(2);
    branch.insert(70);

    auto pred = branch.predecessor(70);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch64Test, PredecessorReturnsNullBeforeSmallestElement)
{
    LeafBranch branch;
    branch.insert(5);

    EXPECT_FALSE(branch.predecessor(5).has_value());
    EXPECT_FALSE(branch.predecessor(0).has_value());
}
