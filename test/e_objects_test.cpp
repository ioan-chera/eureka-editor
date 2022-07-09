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

#include "e_objects.h"

#include "Document.h"
#include "Instance.h"
#include "LineDef.h"
#include "Vertex.h"
#include "w_rawdef.h"

#include "gtest/gtest.h"

class EObjectsFixture : public ::testing::Test
{
protected:
	~EObjectsFixture()
	{
		inst.level.basis.clearAll();
	}

	Instance inst;
};

// Helpful vertex comparator so we sort our data
static bool vertexCompare(const Vertex *v1, const Vertex *v2)
{
	return v1->raw_x == v2->raw_x ? v1->raw_y < v2->raw_y : v1->raw_x < v2->raw_x;
};

/* To test:
 - that we invalidate the selection if the number of selected objects is changeed
 - various merge scenarios
 */

//
//             ._.
// We test a ._| |_. dragging of the top line to go to the level of the mid lines
//           |_____|
// We need:
// - 6 vertices
// - 1 sector
// - 5 sides
// - 5 lines
//
TEST_F(EObjectsFixture, DragWallLineToCancelSurroundingLines)
{
	// Each line shall be 64 units long
	static const FFixedPoint vertexCoordinates[8][2] = {
		{ FFixedPoint(-64), FFixedPoint(0) },
		{ FFixedPoint(0), FFixedPoint(0) },
		{ FFixedPoint(0), FFixedPoint(64) },
		{ FFixedPoint(64), FFixedPoint(64) },
		{ FFixedPoint(64), FFixedPoint(0) },
		{ FFixedPoint(128), FFixedPoint(0) },
		{ FFixedPoint(128), FFixedPoint(-64) },
		{ FFixedPoint(-64), FFixedPoint(-64) },
	};

	Document &doc = inst.level;

	for(size_t i = 0; i < 8; ++i)
	{
		auto vertex = new Vertex;
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(vertex);
	}

	Sector *sector = new Sector;
	doc.sectors.push_back(sector);

	for(int i = 0; i < 8; ++i)
	{
		auto side = new SideDef;
		side->sector = 0;
		doc.sidedefs.push_back(side);

		auto line = new LineDef;
		line->start = i;
		line->end = (i + 1) % 8;
		line->right = i;
		doc.linedefs.push_back(line);
	}

	//                    _
	// Now do the move ._| |_. --> ._._._.
	//                 |_____|     |_____|
	selection_c selection(ObjType::linedefs);
	selection.set(2);

	v3double_t delta = {0, -64, 0};
	doc.objects.move(selection, delta);

	// Now check we have a clear situation
	ASSERT_EQ(doc.numVertices(), 6);
	ASSERT_EQ(doc.numSectors(), 1);	// still one
	ASSERT_EQ(doc.numSidedefs(), 6);
	ASSERT_EQ(doc.numLinedefs(), 6);

	// Now we must check the coordinates. We do NOT care about order
	std::vector<Vertex *> vertices = doc.vertices;
	std::sort(vertices.begin(), vertices.end(), vertexCompare);
	ASSERT_EQ(vertices[0]->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vertices[1]->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vertices[2]->xy(), v2double_t(0, 0));
	ASSERT_EQ(vertices[3]->xy(), v2double_t(64, 0));
	ASSERT_EQ(vertices[4]->xy(), v2double_t(128, -64));
	ASSERT_EQ(vertices[5]->xy(), v2double_t(128, 0));

	std::vector<LineDef *> lines = doc.linedefs;
	std::sort(lines.begin(), lines.end(), [&doc](const LineDef *L1, const LineDef *L2){
		return vertexCompare(doc.vertices[L1->start], doc.vertices[L2->start]);
	});
	ASSERT_EQ(lines[0]->Start(doc)->xy(), v2double_t(-64, -64));
	ASSERT_EQ(lines[0]->End(doc)->xy(), v2double_t(-64, 0));
	ASSERT_EQ(lines[1]->Start(doc)->xy(), v2double_t(-64, 0));
	ASSERT_EQ(lines[1]->End(doc)->xy(), v2double_t(0, 0));
	ASSERT_EQ(lines[2]->Start(doc)->xy(), v2double_t(0, 0));
	ASSERT_EQ(lines[2]->End(doc)->xy(), v2double_t(64, 0));
	ASSERT_EQ(lines[3]->Start(doc)->xy(), v2double_t(64, 0));
	ASSERT_EQ(lines[3]->End(doc)->xy(), v2double_t(128, 0));
	ASSERT_EQ(lines[4]->Start(doc)->xy(), v2double_t(128, -64));
	ASSERT_EQ(lines[4]->End(doc)->xy(), v2double_t(-64, -64));
	ASSERT_EQ(lines[5]->Start(doc)->xy(), v2double_t(128, 0));
	ASSERT_EQ(lines[5]->End(doc)->xy(), v2double_t(128, -64));

	std::sort(lines.begin(), lines.end(), [](const LineDef *L1, const LineDef *L2) {
		return L1->right < L2->right;
	});
	for(int i = 0; i < 6; ++i)
		ASSERT_EQ(lines[i]->right, i);
}

//                        ._. ._.
// Performs a dragging of |_|_|_| into ._._._. eliminating sectors. The target lines have opposite
// orientations.          |_____|      |_____|
//
TEST_F(EObjectsFixture, DragLineToEliminateSector)
{
	static const FFixedPoint vertexCoordinates[10][2] = {
		{ FFixedPoint(-64), FFixedPoint(0) },
		{ FFixedPoint(-64), FFixedPoint(64) },
		{ FFixedPoint(0), FFixedPoint(64) },
		{ FFixedPoint(0), FFixedPoint(0) },
		{ FFixedPoint(64), FFixedPoint(0) },
		{ FFixedPoint(64), FFixedPoint(64) },
		{ FFixedPoint(128), FFixedPoint(64) },
		{ FFixedPoint(128), FFixedPoint(0) },
		{ FFixedPoint(128), FFixedPoint(-64) },
		{ FFixedPoint(-64), FFixedPoint(-64) },
	};

	Document &doc = inst.level;

	for(size_t i = 0; i < 10; ++i)
	{
		auto vertex = new Vertex;
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(vertex);
	}

	auto topLeftSector = new Sector;
	topLeftSector->floor_tex = BA_InternaliseString("FTOPLEFT");
	auto topRightSector = new Sector;
	topRightSector->floor_tex = BA_InternaliseString("FTOPRITE");
	auto bottomSector = new Sector;
	bottomSector->floor_tex = BA_InternaliseString("FBOTTOM");
	doc.sectors.push_back(bottomSector);
	doc.sectors.push_back(topLeftSector);
	doc.sectors.push_back(topRightSector);

	SideDef *side;
	// bottom room
	for(int i = 0; i < 6; ++i)
	{
		side = new SideDef;
		side->sector = 0;
		if(i != 0 && i != 2)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("BOTTOM");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(side);
	}
	// top-left room
	for(int i = 0; i < 4; ++i)
	{
		side = new SideDef;
		side->sector = 1;
		if(i != 3)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("TOPLEFT");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(side);
	}
	// top-right room
	for(int i = 0; i < 4; ++i)
	{
		side = new SideDef;
		side->sector = 2;
		if(i != 3)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("TOPRIGHT");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(side);
	}

	// Too many lines to concern about, so just create them here
	LineDef *lines[12];
	for(int i = 0; i < 12; ++i)
	{
		lines[i] = new LineDef;
		doc.linedefs.push_back(lines[i]);
	}
	for(int i = 0; i < 10; ++i)
	{
		lines[i]->start = i;
		lines[i]->end = (i + 1) % 10;
		lines[i]->flags = MLF_Blocking;
	}
	// Left bridge pointing right
	lines[10]->start = 0;
	lines[10]->end = 3;
	lines[10]->flags = MLF_TwoSided;

	// Right bridge pointing left
	lines[11]->start = 7;
	lines[11]->end = 4;
	lines[11]->flags = MLF_TwoSided;

	// Bottom room
	lines[10]->right = 0;
	lines[3]->right = 1;
	lines[11]->left = 2;
	lines[7]->right = 3;
	lines[8]->right = 4;
	lines[9]->right = 5;

	// top left room
	lines[0]->right = 6;
	lines[1]->right = 7;
	lines[2]->right = 8;
	lines[10]->left = 9;

	// top right room
	lines[4]->right = 10;
	lines[5]->right = 11;
	lines[6]->right = 12;
	lines[11]->right = 13;

	// Need to check both kinds of merge now
	//                 ._. ._.         ._.
	// Now do the move |_|_|_| --> ._._|_|
	//                 |_____|     |_____|
	selection_c selection(ObjType::linedefs);
	selection.set(1);

	v3double_t delta = {0, -64, 0};
	doc.objects.move(selection, delta);

	ASSERT_EQ(doc.numVertices(), 8);
	ASSERT_EQ(doc.numSidedefs(), 10);
	ASSERT_EQ(doc.numLinedefs(), 9);
	ASSERT_EQ(doc.numSectors(), 2);

	std::vector<Vertex *> vertices = doc.vertices;
	std::sort(vertices.begin(), vertices.end(), vertexCompare);

	ASSERT_EQ(vertices[0]->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vertices[1]->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vertices[2]->xy(), v2double_t(0, 0));
	ASSERT_EQ(vertices[3]->xy(), v2double_t(64, 0));
	ASSERT_EQ(vertices[4]->xy(), v2double_t(64, 64));
	ASSERT_EQ(vertices[5]->xy(), v2double_t(128, -64));
	ASSERT_EQ(vertices[6]->xy(), v2double_t(128, 0));
	ASSERT_EQ(vertices[7]->xy(), v2double_t(128, 64));

	std::vector<LineDef *> vlines = doc.linedefs;
	std::sort(vlines.begin(), vlines.end(), [&doc](const LineDef *L1, const LineDef *L2){
		return doc.vertices[L1->start]->xy() == doc.vertices[L2->start]->xy() ?
			vertexCompare(doc.vertices[L1->end], doc.vertices[L2->end]) :
			vertexCompare(doc.vertices[L1->start], doc.vertices[L2->start]);
	});
	ASSERT_EQ(vlines[0]->Start(doc)->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vlines[0]->End(doc)->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vlines[0]->Right(doc)->MidTex(), "BOTTOM");
	ASSERT_EQ(vlines[0]->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_FALSE(vlines[0]->Left(doc));
	ASSERT_EQ(vlines[0]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[1]->Start(doc)->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vlines[1]->End(doc)->xy(), v2double_t(0, 0));
	ASSERT_EQ(vlines[1]->Right(doc)->MidTex(), "TOPLEFT");	// texture copied
	ASSERT_EQ(vlines[1]->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_FALSE(vlines[1]->Left(doc));
	ASSERT_EQ(vlines[1]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[2]->Start(doc)->xy(), v2double_t(0, 0));
	ASSERT_EQ(vlines[2]->End(doc)->xy(), v2double_t(64, 0));
	ASSERT_EQ(vlines[2]->Right(doc)->MidTex(), "BOTTOM");
	ASSERT_EQ(vlines[2]->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_FALSE(vlines[2]->Left(doc));
	ASSERT_EQ(vlines[2]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[3]->Start(doc)->xy(), v2double_t(64, 0));
	ASSERT_EQ(vlines[3]->End(doc)->xy(), v2double_t(64, 64));
	ASSERT_EQ(vlines[3]->Right(doc)->MidTex(), "TOPRIGHT");
	ASSERT_EQ(vlines[3]->Right(doc)->SecRef(doc)->FloorTex(), "FTOPRITE");
	ASSERT_FALSE(vlines[3]->Left(doc));
	ASSERT_EQ(vlines[3]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[4]->Start(doc)->xy(), v2double_t(64, 64));
	ASSERT_EQ(vlines[4]->End(doc)->xy(), v2double_t(128, 64));
	ASSERT_EQ(vlines[4]->Right(doc)->MidTex(), "TOPRIGHT");
	ASSERT_EQ(vlines[4]->Right(doc)->SecRef(doc)->FloorTex(), "FTOPRITE");
	ASSERT_FALSE(vlines[4]->Left(doc));
	ASSERT_EQ(vlines[4]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[5]->Start(doc)->xy(), v2double_t(128, -64));
	ASSERT_EQ(vlines[5]->End(doc)->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vlines[5]->Right(doc)->MidTex(), "BOTTOM");
	ASSERT_EQ(vlines[5]->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_FALSE(vlines[5]->Left(doc));
	ASSERT_EQ(vlines[5]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[6]->Start(doc)->xy(), v2double_t(128, 0));
	ASSERT_EQ(vlines[6]->End(doc)->xy(), v2double_t(64, 0));
	ASSERT_EQ(vlines[6]->Right(doc)->MidTex(), "-");
	ASSERT_EQ(vlines[6]->Right(doc)->SecRef(doc)->FloorTex(), "FTOPRITE");
	ASSERT_EQ(vlines[6]->Left(doc)->MidTex(), "-");
	ASSERT_EQ(vlines[6]->Left(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_EQ(vlines[6]->flags, MLF_TwoSided);

	ASSERT_EQ(vlines[7]->Start(doc)->xy(), v2double_t(128, 0));
	ASSERT_EQ(vlines[7]->End(doc)->xy(), v2double_t(128, -64));
	ASSERT_EQ(vlines[7]->Right(doc)->MidTex(), "BOTTOM");
	ASSERT_EQ(vlines[7]->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
	ASSERT_FALSE(vlines[7]->Left(doc));
	ASSERT_EQ(vlines[7]->flags, MLF_Blocking);

	ASSERT_EQ(vlines[8]->Start(doc)->xy(), v2double_t(128, 64));
	ASSERT_EQ(vlines[8]->End(doc)->xy(), v2double_t(128, 0));
	ASSERT_EQ(vlines[8]->Right(doc)->MidTex(), "TOPRIGHT");
	ASSERT_EQ(vlines[8]->Right(doc)->SecRef(doc)->FloorTex(), "FTOPRITE");
	ASSERT_FALSE(vlines[8]->Left(doc));
	ASSERT_EQ(vlines[8]->flags, MLF_Blocking);

	//                     ._.
	// Now do the move ._._|_| --> ._._._.
	//                 |_____|     |_____|

	selection.clear_all();
	// Find the line to move
	int lineIndex = 0;
	for(; lineIndex < doc.numLinedefs(); ++lineIndex)
	{
		if(doc.linedefs[lineIndex]->Start(doc)->xy() == v2double_t{64, 64})
			break;
	}
	ASSERT_LT(lineIndex, doc.numLinedefs());
	selection.set(lineIndex);
	// same delta
	doc.objects.move(selection, delta);

	ASSERT_EQ(doc.numVertices(), 6);
	ASSERT_EQ(doc.numSidedefs(), 6);
	ASSERT_EQ(doc.numLinedefs(), 6);
	ASSERT_EQ(doc.numSectors(), 1);

	// Now find the line to check
	for(lineIndex = 0; lineIndex < doc.numLinedefs(); ++lineIndex)
	{
		const LineDef *line = doc.linedefs[lineIndex];
		if(line->Start(doc)->xy() == v2double_t{64, 0} &&
		   line->End(doc)->xy() == v2double_t{128, 0})
		{
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOPRIGHT");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
			break;
		}
	}
	ASSERT_LT(lineIndex, doc.numLinedefs());

	// Now undo.
	doc.basis.undo();
	doc.basis.undo();

	// Now merge in reverse! The mid linedef towards the top sectors
	//                 ._. ._.     ._. ._.
	// Now do the move |_|_|_| --> |  \|_|
	//                 |_____|     |_____|
	selection.clear_all();
	selection.set(10);
	delta.y = 64;
	doc.objects.move(selection, delta);

	ASSERT_EQ(doc.numVertices(), 8);
	ASSERT_EQ(doc.numLinedefs(), 9);
	ASSERT_EQ(doc.numSidedefs(), 10);
	ASSERT_EQ(doc.numSectors(), 2);

	// Now find the line to check
	int checks = 0;
	for(const LineDef *line : doc.linedefs)
	{
		if(line->Start(doc)->xy() == v2double_t{-64, -64} &&
		   line->End(doc)->xy() == v2double_t{-64, 64})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "BOTTOM");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{-64, 64} &&
				line->End(doc)->xy() == v2double_t{0, 64})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOPLEFT");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{0, 64} &&
				line->End(doc)->xy() == v2double_t{64, 0})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "BOTTOM");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
	}
	ASSERT_EQ(checks, 3);

	//                 ._. ._.     ._._._.
	// Now do the move |  \|_| --> |     |
	//                 |_____|     |_____|
	// Find the line to move
	for(lineIndex = 0; lineIndex < doc.numLinedefs(); ++lineIndex)
	{
		if(doc.linedefs[lineIndex]->Start(doc)->xy() == v2double_t{128, 0} &&
		   doc.linedefs[lineIndex]->End(doc)->xy() == v2double_t{64, 0})
			break;
	}
	ASSERT_LT(lineIndex, doc.numLinedefs());

	selection.clear_all();
	selection.set(lineIndex);
	doc.objects.move(selection, delta);

	ASSERT_EQ(doc.numVertices(), 6);
	ASSERT_EQ(doc.numSidedefs(), 6);
	ASSERT_EQ(doc.numLinedefs(), 6);
	ASSERT_EQ(doc.numSectors(), 1);

	checks = 0;
	for(const LineDef *line : doc.linedefs)
	{
		if(line->Start(doc)->xy() == v2double_t{0, 64} &&
		   line->End(doc)->xy() == v2double_t{64, 64})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "BOTTOM");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{64, 64} &&
		   line->End(doc)->xy() == v2double_t{128, 64})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOPRIGHT");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{128, 64} &&
				line->End(doc)->xy() == v2double_t{128, -64})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "BOTTOM");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
	}
	ASSERT_EQ(checks, 3);
}

//
//      _     Drag the line here below, causing a split and eliminating a sector
//     /_\
//     |/
TEST_F(EObjectsFixture, DragLineToSplitLineAndEliminateSector)
{
	static const FFixedPoint vertexCoordinates[5][2] = {
		{ FFixedPoint(0), FFixedPoint(0) },
		{ FFixedPoint(32), FFixedPoint(64) },
		{ FFixedPoint(96), FFixedPoint(64) },
		{ FFixedPoint(128), FFixedPoint(0) },
		{ FFixedPoint(0), FFixedPoint(-64) },
	};

	Document &doc = inst.level;

	for(size_t i = 0; i < 5; ++i)
	{
		auto vertex = new Vertex;
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(vertex);
	}

	Sector *sector;
	sector = new Sector;
	sector->floor_tex = BA_InternaliseString("FBOTTOM");
	doc.sectors.push_back(sector);
	sector = new Sector;
	sector->floor_tex = BA_InternaliseString("FTOP");
	doc.sectors.push_back(sector);

	SideDef *side;
	side = new SideDef;
	side->mid_tex = BA_InternaliseString("BOTTOM");
	side->sector = 0;
	doc.sidedefs.push_back(side);
	side = new SideDef;
	side->mid_tex = BA_InternaliseString("-");
	side->sector = 0;
	doc.sidedefs.push_back(side);
	side = new SideDef;
	side->mid_tex = BA_InternaliseString("BOTTOM");
	side->sector = 0;
	doc.sidedefs.push_back(side);

	side = new SideDef;
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(side);

	side = new SideDef;
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(side);

	side = new SideDef;
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(side);

	side = new SideDef;
	side->mid_tex = BA_InternaliseString("-");
	side->sector = 1;
	doc.sidedefs.push_back(side);

	LineDef *line;
	line = new LineDef;
	line->start = 0;
	line->end = 1;
	line->right = 3;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(line);

	line = new LineDef;
	line->start = 1;
	line->end = 2;
	line->right = 4;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(line);

	line = new LineDef;
	line->start = 2;
	line->end = 3;
	line->right = 5;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(line);

	line = new LineDef;
	line->start = 3;
	line->end = 4;
	line->right = 2;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(line);

	line = new LineDef;
	line->start = 4;
	line->end = 0;
	line->right = 0;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(line);

	line = new LineDef;
	line->start = 0;
	line->end = 3;
	line->right = 1;
	line->left = 6;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(line);

	selection_c selection(ObjType::linedefs);
	selection.set(1);

	v3double_t delta = {0, -64, 0};
	doc.objects.move(selection, delta);

	//     ___
	//     |/
	ASSERT_EQ(doc.numVertices(), 5);
	ASSERT_EQ(doc.numSectors(), 1);
	ASSERT_EQ(doc.numSidedefs(), 5);
	ASSERT_EQ(doc.numLinedefs(), 5);

	int checks = 0;
	for(const LineDef *line : doc.linedefs)
	{
		if(line->Start(doc)->xy() == v2double_t{0, 0} &&
		   line->End(doc)->xy() == v2double_t{32, 0})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOP");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{32, 0} &&
				line->End(doc)->xy() == v2double_t{96, 0})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOP");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(line->Start(doc)->xy() == v2double_t{96, 0} &&
				line->End(doc)->xy() == v2double_t{128, 0})
		{
			++checks;
			ASSERT_EQ(line->Right(doc)->MidTex(), "TOP");
			ASSERT_EQ(line->Right(doc)->SecRef(doc)->FloorTex(), "FBOTTOM");
			ASSERT_FALSE(line->Left(doc));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
	}
}
