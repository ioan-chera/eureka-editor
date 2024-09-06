//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "Document.h"
#include "Instance.h"
#include "lib_adler.h"
#include "LineDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "gtest/gtest.h"

class DocumentFixture : public ::testing::Test
{
protected:
	DocumentFixture() : doc(inst)
	{
	}

	Instance inst;
	Document doc;
};

TEST_F(DocumentFixture, CheckObjects)
{
	// Start with 0
	ASSERT_FALSE(doc.numThings());
	ASSERT_FALSE(doc.numVertices());
	ASSERT_FALSE(doc.numSectors());
	ASSERT_FALSE(doc.numSidedefs());
	ASSERT_FALSE(doc.numLinedefs());

	// Add some objects
	doc.things.push_back(std::make_shared<Thing>());
	doc.things.push_back(std::make_shared<Thing>());
	doc.things.push_back(std::make_shared<Thing>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	// no sectors
	doc.sidedefs.push_back(std::make_shared<SideDef>());
	doc.sidedefs.push_back(std::make_shared<SideDef>());
	doc.linedefs.push_back(std::make_shared<LineDef>());

	ASSERT_EQ(doc.numThings(), 3);
	ASSERT_EQ(doc.numVertices(), 4);
	ASSERT_EQ(doc.numSectors(), 0);
	ASSERT_EQ(doc.numSidedefs(), 2);
	ASSERT_EQ(doc.numLinedefs(), 1);
	ASSERT_EQ(doc.numObjects(ObjType::things), 3);
	ASSERT_EQ(doc.numObjects(ObjType::vertices), 4);
	ASSERT_EQ(doc.numObjects(ObjType::sectors), 0);
	ASSERT_EQ(doc.numObjects(ObjType::sidedefs), 2);
	ASSERT_EQ(doc.numObjects(ObjType::linedefs), 1);

	ASSERT_FALSE(doc.isThing(-1));
	ASSERT_TRUE(doc.isThing(0));
	ASSERT_TRUE(doc.isThing(1));
	ASSERT_TRUE(doc.isThing(2));
	ASSERT_FALSE(doc.isThing(3));

	ASSERT_FALSE(doc.isVertex(-1));
	ASSERT_TRUE(doc.isVertex(0));
	ASSERT_TRUE(doc.isVertex(1));
	ASSERT_TRUE(doc.isVertex(2));
	ASSERT_TRUE(doc.isVertex(3));
	ASSERT_FALSE(doc.isVertex(4));

	ASSERT_FALSE(doc.isSector(-1));
	ASSERT_FALSE(doc.isSector(0));

	ASSERT_FALSE(doc.isSidedef(-1));
	ASSERT_TRUE(doc.isSidedef(0));
	ASSERT_TRUE(doc.isSidedef(1));
	ASSERT_FALSE(doc.isSidedef(2));

	ASSERT_FALSE(doc.isLinedef(-1));
	ASSERT_TRUE(doc.isLinedef(0));
	ASSERT_FALSE(doc.isLinedef(1));
}

TEST_F(DocumentFixture, CRC)
{
	// Add some objects
	doc.things.push_back(std::make_shared<Thing>());
	doc.things.push_back(std::make_shared<Thing>());
	doc.things.push_back(std::make_shared<Thing>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	doc.vertices.push_back(std::make_shared<Vertex>());
	// no sectors
	doc.sidedefs.push_back(std::make_shared<SideDef>());
	doc.sidedefs.push_back(std::make_shared<SideDef>());
	doc.linedefs.push_back(std::make_shared<LineDef>());

	crc32_c crc;
	doc.getLevelChecksum(crc);

	// Now remove one thing
	doc.things.pop_back();

	crc32_c crc2;
	doc.getLevelChecksum(crc2);

	ASSERT_NE(crc.getPath(), crc2.getPath());

	// Now add back one thing
	doc.things.push_back(std::make_shared<Thing>());

	crc32_c crc3;
	doc.getLevelChecksum(crc3);
	ASSERT_EQ(crc.getPath(), crc3.getPath());
}
