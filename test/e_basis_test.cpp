//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "gtest/gtest.h"
#include "Document.h"
#include "Instance.h"

class BasisLumpChangeFixture : public ::testing::TestWithParam<LumpType>
{
protected:
    Instance inst;
    Document doc{inst};

    std::vector<byte> &lump(LumpType type)
    {
        switch(type)
        {
        case LumpType::header:
            return doc.headerData;
        case LumpType::behavior:
            return doc.behaviorData;
        case LumpType::scripts:
            return doc.scriptsData;
        }
        // Should never reach here
        return doc.headerData;
    }
};

TEST_P(BasisLumpChangeFixture, ChangeUndoRedo)
{
    LumpType type = GetParam();
    auto &data = lump(type);

    std::vector<byte> original{1, 2, 3};
    data = original;

    std::vector<byte> replacement{4, 5};
    {
        EditOperation op(doc.basis);
        op.changeLump(type, std::vector<byte>(replacement));
    }

    EXPECT_EQ(data, replacement);

    ASSERT_TRUE(doc.basis.undo());
    EXPECT_EQ(data, original);

    ASSERT_TRUE(doc.basis.redo());
    EXPECT_EQ(data, replacement);

    ASSERT_TRUE(doc.basis.undo());
    EXPECT_EQ(data, original);

	// Now change something to see that redo becomes unavailable
	{
		EditOperation op(doc.basis);
		op.changeLump(type, std::vector<byte>{6, 7});
	}
	ASSERT_FALSE(doc.basis.redo());
	ASSERT_TRUE(doc.basis.undo());
	EXPECT_EQ(data, original);

	ASSERT_FALSE(doc.basis.undo());
}

INSTANTIATE_TEST_SUITE_P(
    BasisLumpChanges,
    BasisLumpChangeFixture,
    ::testing::Values(LumpType::header, LumpType::behavior, LumpType::scripts));

class BasisChangeStatusFixture : public ::testing::Test
{
protected:
    Instance inst;
    Document doc{inst};

    void changeHeader(const std::vector<byte> &data)
    {
        EditOperation op(doc.basis);
        op.changeLump(LumpType::header, std::vector<byte>(data));
    }
};

// Saving should update the saved stack and clear MadeChanges
TEST_F(BasisChangeStatusFixture, SavingResetsMadeChanges)
{
    doc.headerData = {1};
    changeHeader({2});
    EXPECT_TRUE(doc.hasChanges());

    doc.markSaved();
    EXPECT_FALSE(doc.hasChanges());
}

// Undoing and redoing toggles MadeChanges depending on the saved stack
TEST_F(BasisChangeStatusFixture, UndoRedoTogglesMadeChanges)
{
    doc.headerData = {1};

    changeHeader({2});
    doc.markSaved();

    changeHeader({3});
    EXPECT_TRUE(doc.hasChanges());

    ASSERT_TRUE(doc.basis.undo());
    EXPECT_FALSE(doc.hasChanges());

    ASSERT_TRUE(doc.basis.undo());
    EXPECT_TRUE(doc.hasChanges());

    ASSERT_TRUE(doc.basis.redo());
    EXPECT_FALSE(doc.hasChanges());

    ASSERT_TRUE(doc.basis.redo());
    EXPECT_TRUE(doc.hasChanges());
}

// Repeating a redo operation manually can clear the MadeChanges flag
TEST_F(BasisChangeStatusFixture, ManualRedoActionClearsMadeChanges)
{
    doc.headerData = {1};

    changeHeader({2});
    changeHeader({3});
    doc.markSaved();

    ASSERT_TRUE(doc.basis.undo());
    EXPECT_TRUE(doc.hasChanges());

    changeHeader({3});
    EXPECT_FALSE(doc.hasChanges());

    changeHeader({4});
    EXPECT_TRUE(doc.hasChanges());
}

