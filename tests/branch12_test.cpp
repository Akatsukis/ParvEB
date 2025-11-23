#include <gtest/gtest.h>

#include "veb_branch.hpp"

using Branch = VebBranch12;

TEST(Branch12Test, StartsEmptyUntilInsert)
{
    Branch branch;
    EXPECT_TRUE(branch.empty());

    branch.insert(0);
    EXPECT_FALSE(branch.empty());
}

TEST(Branch12Test, ContainsRecognizesValuesAcrossChildren)
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
    EXPECT_FALSE(branch.contains(4095));
}

TEST(Branch12Test, MinTracksGlobalMinimum)
{
    Branch branch;
    branch.insert(256);
    branch.insert(5);
    branch.insert(1024);

    auto min_val = branch.min();
    ASSERT_TRUE(min_val.has_value());
    EXPECT_EQ(5u, *min_val);
}

TEST(Branch12Test, MaxTracksGlobalMaximum)
{
    Branch branch;
    branch.insert(255);
    branch.insert(512);
    branch.insert(1024);

    auto max_val = branch.max();
    ASSERT_TRUE(max_val.has_value());
    EXPECT_EQ(1024u, *max_val);
}

TEST(Branch12Test, EraseUpdatesMinWhenLowestLeafRemoved)
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

TEST(Branch12Test, EraseRemovesChildrenAndCanBecomeEmpty)
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

TEST(Branch12Test, SuccessorWithinSameChild)
{
    Branch branch;
    branch.insert(2);
    branch.insert(10);

    auto succ = branch.successor(2);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(10u, *succ);
}

TEST(Branch12Test, SuccessorAcrossChildBoundary)
{
    Branch branch;
    branch.insert(255);
    branch.insert(300);

    auto succ = branch.successor(255);
    ASSERT_TRUE(succ.has_value());
    EXPECT_EQ(300u, *succ);
}

TEST(Branch12Test, SuccessorReturnsNullAfterLargestElement)
{
    Branch branch;
    branch.insert(600);

    EXPECT_FALSE(branch.successor(600).has_value());
    EXPECT_FALSE(branch.successor(0xFFFF).has_value());
}

TEST(Branch12Test, PredecessorWithinSameChild)
{
    Branch branch;
    branch.insert(2);
    branch.insert(10);

    auto pred = branch.predecessor(10);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch12Test, PredecessorAcrossChildBoundary)
{
    Branch branch;
    branch.insert(2);
    branch.insert(70);

    auto pred = branch.predecessor(70);
    ASSERT_TRUE(pred.has_value());
    EXPECT_EQ(2u, *pred);
}

TEST(Branch12Test, PredecessorReturnsNullBeforeSmallestElement)
{
    Branch branch;
    branch.insert(5);

    EXPECT_FALSE(branch.predecessor(5).has_value());
    EXPECT_FALSE(branch.predecessor(0).has_value());
}
