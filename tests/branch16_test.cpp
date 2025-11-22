#include <gtest/gtest.h>

#include "veb_branch.hpp"

using Branch = VebBranch16;

TEST(Branch16Test, StartsEmptyUntilInsert)
{
    Branch branch;
    EXPECT_TRUE(branch.empty());

    branch.insert(0);
    EXPECT_FALSE(branch.empty());
}

TEST(Branch16Test, ContainsRecognizesValuesAcrossChildren)
{
    Branch branch;
    branch.insert(0);
    branch.insert(255);
    branch.insert(256);
    branch.insert(1024);

    EXPECT_TRUE(branch.contains(0));
    EXPECT_TRUE(branch.contains(255));
    EXPECT_TRUE(branch.contains(256));
    EXPECT_TRUE(branch.contains(1024));
    EXPECT_FALSE(branch.contains(4096));
}

TEST(Branch16Test, MinTracksGlobalMinimum)
{
    Branch branch;
    branch.insert(256);
    branch.insert(5);
    branch.insert(1024);

    auto min_val = branch.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(5u, *min_val);
}

TEST(Branch16Test, MaxTracksGlobalMaximum)
{
    Branch branch;
    branch.insert(255);
    branch.insert(512);
    branch.insert(1024);

    auto max_val = branch.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(1024u, *max_val);
}

TEST(Branch16Test, EraseUpdatesMinWhenLowestLeafRemoved)
{
    Branch branch;
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

TEST(Branch16Test, EraseRemovesChildrenAndCanBecomeEmpty)
{
    Branch branch;
    branch.insert(5);
    branch.insert(70);

    branch.erase(5);
    EXPECT_FALSE(branch.empty());

    branch.erase(70);
    EXPECT_TRUE(branch.empty());
    EXPECT_FALSE(branch.min().has_value());
    EXPECT_FALSE(branch.contains(70));
}

TEST(Branch16Test, SuccessorWithinSameChild)
{
    Branch branch;
    branch.insert(2);
    branch.insert(10);

    auto succ = branch.successor(2);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(10u, *succ);
}

TEST(Branch16Test, SuccessorAcrossChildBoundary)
{
    Branch branch;
    branch.insert(255);
    branch.insert(300);

    auto succ = branch.successor(255);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(300u, *succ);
}

TEST(Branch16Test, SuccessorReturnsNullAfterLargestElement)
{
    Branch branch;
    branch.insert(60000);

    EXPECT_FALSE(branch.successor(60000).has_value());
    EXPECT_FALSE(branch.successor(0xFFFF).has_value());
}

TEST(Branch16Test, PredecessorWithinSameChild)
{
    Branch branch;
    branch.insert(2);
    branch.insert(10);

    auto pred = branch.predecessor(10);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch16Test, PredecessorAcrossChildBoundary)
{
    Branch branch;
    branch.insert(2);
    branch.insert(70);

    auto pred = branch.predecessor(70);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch16Test, PredecessorReturnsNullBeforeSmallestElement)
{
    Branch branch;
    branch.insert(5);

    EXPECT_FALSE(branch.predecessor(5).has_value());
    EXPECT_FALSE(branch.predecessor(0).has_value());
}
