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
}

INSTANTIATE_TEST_SUITE_P(
    BasisLumpChanges,
    BasisLumpChangeFixture,
    ::testing::Values(LumpType::header, LumpType::behavior, LumpType::scripts));

