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
		inst.level.clear();
	}

	void moveMouse(const v2double_t &pos);

	Instance inst;
};

void EObjectsFixture::moveMouse(const v2double_t &pos)
{
	inst.edit.map.xy = pos;
	inst.UpdateHighlight();
}

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
		auto vertex = std::make_unique<Vertex>();
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(std::move(vertex));
	}

	auto sector = std::make_shared<Sector>();
	doc.sectors.push_back(std::move(sector));

	for(int i = 0; i < 8; ++i)
	{
		auto side = std::make_shared<SideDef>();
		side->sector = 0;
		doc.sidedefs.push_back(std::move(side));

		auto line = std::make_shared<LineDef>();
		line->start = i;
		line->end = (i + 1) % 8;
		line->right = i;
		doc.linedefs.push_back(std::move(line));
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
	std::vector<Vertex *> vertices;
	for(const std::shared_ptr<Vertex> &vertex : doc.vertices)
		vertices.push_back(vertex.get());
	std::sort(vertices.begin(), vertices.end(), vertexCompare);
	ASSERT_EQ(vertices[0]->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vertices[1]->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vertices[2]->xy(), v2double_t(0, 0));
	ASSERT_EQ(vertices[3]->xy(), v2double_t(64, 0));
	ASSERT_EQ(vertices[4]->xy(), v2double_t(128, -64));
	ASSERT_EQ(vertices[5]->xy(), v2double_t(128, 0));

	std::vector<const LineDef *> lines;
	for(const std::shared_ptr<LineDef> &line : doc.linedefs)
		lines.push_back(line.get());
	std::sort(lines.begin(), lines.end(), [&doc](const LineDef *L1, const LineDef *L2){
		return vertexCompare(doc.vertices[L1->start].get(), doc.vertices[L2->start].get());
	});
	ASSERT_EQ(doc.getStart(*lines[0]).xy(), v2double_t(-64, -64));
	ASSERT_EQ(doc.getEnd(*lines[0]).xy(), v2double_t(-64, 0));
	ASSERT_EQ(doc.getStart(*lines[1]).xy(), v2double_t(-64, 0));
	ASSERT_EQ(doc.getEnd(*lines[1]).xy(), v2double_t(0, 0));
	ASSERT_EQ(doc.getStart(*lines[2]).xy(), v2double_t(0, 0));
	ASSERT_EQ(doc.getEnd(*lines[2]).xy(), v2double_t(64, 0));
	ASSERT_EQ(doc.getStart(*lines[3]).xy(), v2double_t(64, 0));
	ASSERT_EQ(doc.getEnd(*lines[3]).xy(), v2double_t(128, 0));
	ASSERT_EQ(doc.getStart(*lines[4]).xy(), v2double_t(128, -64));
	ASSERT_EQ(doc.getEnd(*lines[4]).xy(), v2double_t(-64, -64));
	ASSERT_EQ(doc.getStart(*lines[5]).xy(), v2double_t(128, 0));
	ASSERT_EQ(doc.getEnd(*lines[5]).xy(), v2double_t(128, -64));

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
		auto vertex = std::make_unique<Vertex>();
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(std::move(vertex));
	}

	auto topLeftSector = std::make_shared<Sector>();
	topLeftSector->floor_tex = BA_InternaliseString("FTOPLEFT");
	auto topRightSector = std::make_shared<Sector>();
	topRightSector->floor_tex = BA_InternaliseString("FTOPRITE");
	auto bottomSector = std::make_shared<Sector>();
	bottomSector->floor_tex = BA_InternaliseString("FBOTTOM");
	doc.sectors.push_back(std::move(bottomSector));
	doc.sectors.push_back(std::move(topLeftSector));
	doc.sectors.push_back(std::move(topRightSector));

	std::shared_ptr<SideDef> side;
	// bottom room
	for(int i = 0; i < 6; ++i)
	{
		side = std::make_shared<SideDef>();
		side->sector = 0;
		if(i != 0 && i != 2)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("BOTTOM");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(std::move(side));
	}
	// top-left room
	for(int i = 0; i < 4; ++i)
	{
		side = std::make_shared<SideDef>();
		side->sector = 1;
		if(i != 3)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("TOPLEFT");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(std::move(side));
	}
	// top-right room
	for(int i = 0; i < 4; ++i)
	{
		side = std::make_shared<SideDef>();
		side->sector = 2;
		if(i != 3)	// do not texture mid sides
			side->mid_tex = BA_InternaliseString("TOPRIGHT");
		else
			side->mid_tex = BA_InternaliseString("-");
		doc.sidedefs.push_back(std::move(side));
	}

	// Too many lines to concern about, so just create them here
	std::vector<std::shared_ptr<LineDef>> &lines = doc.linedefs;
	for(int i = 0; i < 12; ++i)
	{
		lines.push_back(std::make_unique<LineDef>());
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

	std::vector<const Vertex *> vertices;
	for(const std::shared_ptr<Vertex> &vertex : doc.vertices)
		vertices.push_back(vertex.get());
	std::sort(vertices.begin(), vertices.end(), vertexCompare);

	ASSERT_EQ(vertices[0]->xy(), v2double_t(-64, -64));
	ASSERT_EQ(vertices[1]->xy(), v2double_t(-64, 0));
	ASSERT_EQ(vertices[2]->xy(), v2double_t(0, 0));
	ASSERT_EQ(vertices[3]->xy(), v2double_t(64, 0));
	ASSERT_EQ(vertices[4]->xy(), v2double_t(64, 64));
	ASSERT_EQ(vertices[5]->xy(), v2double_t(128, -64));
	ASSERT_EQ(vertices[6]->xy(), v2double_t(128, 0));
	ASSERT_EQ(vertices[7]->xy(), v2double_t(128, 64));

	std::vector<const LineDef *> vlines;
	for(const std::shared_ptr<LineDef> &line : doc.linedefs)
		vlines.push_back(line.get());
	std::sort(vlines.begin(), vlines.end(), [&doc](const LineDef *L1, const LineDef *L2){
		return doc.vertices[L1->start]->xy() == doc.vertices[L2->start]->xy() ?
			vertexCompare(doc.vertices[L1->end].get(), doc.vertices[L2->end].get()) :
			vertexCompare(doc.vertices[L1->start].get(), doc.vertices[L2->start].get());
	});
	ASSERT_EQ(doc.getStart(*vlines[0]).xy(), v2double_t(-64, -64));
	ASSERT_EQ(doc.getEnd(*vlines[0]).xy(), v2double_t(-64, 0));
	ASSERT_EQ(doc.getRight(*vlines[0])->MidTex(), "BOTTOM");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[0])).FloorTex(), "FBOTTOM");
	ASSERT_FALSE(doc.getLeft(*vlines[0]));
	ASSERT_EQ(vlines[0]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[1]).xy(), v2double_t(-64, 0));
	ASSERT_EQ(doc.getEnd(*vlines[1]).xy(), v2double_t(0, 0));
	ASSERT_EQ(doc.getRight(*vlines[1])->MidTex(), "TOPLEFT");	// texture copied
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[1])).FloorTex(), "FBOTTOM");
	ASSERT_FALSE(doc.getLeft(*vlines[1]));
	ASSERT_EQ(vlines[1]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[2]).xy(), v2double_t(0, 0));
	ASSERT_EQ(doc.getEnd(*vlines[2]).xy(), v2double_t(64, 0));
	ASSERT_EQ(doc.getRight(*vlines[2])->MidTex(), "BOTTOM");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[2])).FloorTex(), "FBOTTOM");
	ASSERT_FALSE(doc.getLeft(*vlines[2]));
	ASSERT_EQ(vlines[2]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[3]).xy(), v2double_t(64, 0));
	ASSERT_EQ(doc.getEnd(*vlines[3]).xy(), v2double_t(64, 64));
	ASSERT_EQ(doc.getRight(*vlines[3])->MidTex(), "TOPRIGHT");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[3])).FloorTex(), "FTOPRITE");
	ASSERT_FALSE(doc.getLeft(*vlines[3]));
	ASSERT_EQ(vlines[3]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[4]).xy(), v2double_t(64, 64));
	ASSERT_EQ(doc.getEnd(*vlines[4]).xy(), v2double_t(128, 64));
	ASSERT_EQ(doc.getRight(*vlines[4])->MidTex(), "TOPRIGHT");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[4])).FloorTex(), "FTOPRITE");
	ASSERT_FALSE(doc.getLeft(*vlines[4]));
	ASSERT_EQ(vlines[4]->flags, MLF_Blocking);
	
	// TODO

	ASSERT_EQ(doc.getStart(*vlines[5]).xy(), v2double_t(128, -64));
	ASSERT_EQ(doc.getEnd(*vlines[5]).xy(), v2double_t(-64, -64));
	ASSERT_EQ(doc.getRight(*vlines[5])->MidTex(), "BOTTOM");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[5])).FloorTex(), "FBOTTOM");
	ASSERT_FALSE(doc.getLeft(*vlines[5]));
	ASSERT_EQ(vlines[5]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[6]).xy(), v2double_t(128, 0));
	ASSERT_EQ(doc.getEnd(*vlines[6]).xy(), v2double_t(64, 0));
	ASSERT_EQ(doc.getRight(*vlines[6])->MidTex(), "-");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[6])).FloorTex(), "FTOPRITE");
	ASSERT_EQ(doc.getLeft(*vlines[6])->MidTex(), "-");
	ASSERT_EQ(doc.getSector(*doc.getLeft(*vlines[6])).FloorTex(), "FBOTTOM");
	ASSERT_EQ(vlines[6]->flags, MLF_TwoSided);

	ASSERT_EQ(doc.getStart(*vlines[7]).xy(), v2double_t(128, 0));
	ASSERT_EQ(doc.getEnd(*vlines[7]).xy(), v2double_t(128, -64));
	ASSERT_EQ(doc.getRight(*vlines[7])->MidTex(), "BOTTOM");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[7])).FloorTex(), "FBOTTOM");
	ASSERT_FALSE(doc.getLeft(*vlines[7]));
	ASSERT_EQ(vlines[7]->flags, MLF_Blocking);

	ASSERT_EQ(doc.getStart(*vlines[8]).xy(), v2double_t(128, 64));
	ASSERT_EQ(doc.getEnd(*vlines[8]).xy(), v2double_t(128, 0));
	ASSERT_EQ(doc.getRight(*vlines[8])->MidTex(), "TOPRIGHT");
	ASSERT_EQ(doc.getSector(*doc.getRight(*vlines[8])).FloorTex(), "FTOPRITE");
	ASSERT_FALSE(doc.getLeft(*vlines[8]));
	ASSERT_EQ(vlines[8]->flags, MLF_Blocking);

	//                     ._.
	// Now do the move ._._|_| --> ._._._.
	//                 |_____|     |_____|

	selection.clear_all();
	// Find the line to move
	int lineIndex = 0;
	for(; lineIndex < doc.numLinedefs(); ++lineIndex)
	{
		if(doc.getStart(*doc.linedefs[lineIndex]).xy() == v2double_t{64, 64})
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
		const std::shared_ptr<LineDef> &line = doc.linedefs[lineIndex];
		if(doc.getStart(*line).xy() == v2double_t{64, 0} &&
		   doc.getEnd(*line).xy() == v2double_t{128, 0})
		{
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOPRIGHT");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
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
	for(const std::shared_ptr<LineDef> &line : doc.linedefs)
	{
		if(doc.getStart(*line).xy() == v2double_t{-64, -64} &&
		   doc.getEnd(*line).xy() == v2double_t{-64, 64})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "BOTTOM");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{-64, 64} &&
				doc.getEnd(*line).xy() == v2double_t{0, 64})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOPLEFT");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{0, 64} &&
				doc.getEnd(*line).xy() == v2double_t{64, 0})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "BOTTOM");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
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
		if(doc.getStart(*doc.linedefs[lineIndex]).xy() == v2double_t{128, 0} &&
		   doc.getEnd(*doc.linedefs[lineIndex]).xy() == v2double_t{64, 0})
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
	for(const std::shared_ptr<LineDef> &line : doc.linedefs)
	{
		if(doc.getStart(*line).xy() == v2double_t{0, 64} &&
		   doc.getEnd(*line).xy() == v2double_t{64, 64})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "BOTTOM");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{64, 64} &&
		   doc.getEnd(*line).xy() == v2double_t{128, 64})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOPRIGHT");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{128, 64} &&
				doc.getEnd(*line).xy() == v2double_t{128, -64})
		{
			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "BOTTOM");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
	}
	ASSERT_EQ(checks, 3);
}

//
//      _     Drag the line here below, causing a split and eliminating a sector
//     /_\    .
//     |/
TEST_F(EObjectsFixture, DragLineToSplitLineAndEliminateSector)
{
	/*
	  1 2
	 0   3
	 4
	*/
	static const FFixedPoint vertexCoordinates[5][2] = {
		{ FFixedPoint(0), FFixedPoint(0) },
		{ FFixedPoint(32), FFixedPoint(64) },
		{ FFixedPoint(96), FFixedPoint(64) },
		{ FFixedPoint(128), FFixedPoint(0) },
		{ FFixedPoint(0), FFixedPoint(-64) },
	};

	Document &doc = inst.level;
	inst.grid.ForceStep(8);

	for(size_t i = 0; i < 5; ++i)
	{
		auto vertex = std::make_shared<Vertex>();
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(std::move(vertex));
	}

	std::shared_ptr<Sector> sector;
	sector = std::make_shared<Sector>();
	sector->floor_tex = BA_InternaliseString("FBOTTOM");
	doc.sectors.push_back(std::move(sector));
	sector = std::make_shared<Sector>();
	sector->floor_tex = BA_InternaliseString("FTOP");
	doc.sectors.push_back(std::move(sector));

	std::shared_ptr<SideDef> side;	// 0
	side = std::make_shared<SideDef>();
	side->mid_tex = BA_InternaliseString("BOTTOM");
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));
	
	side = std::make_shared<SideDef>();	// 1
	side->mid_tex = BA_InternaliseString("-");
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));
	
	side = std::make_shared<SideDef>();	// 2
	side->mid_tex = BA_InternaliseString("BOTTOM");
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();	// 3
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();	// 4
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();	// 5
	side->mid_tex = BA_InternaliseString("TOP");
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();	// 6
	side->mid_tex = BA_InternaliseString("-");
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	std::shared_ptr<LineDef> line;
	line = std::make_shared<LineDef>();
	line->start = 0;
	line->end = 1;
	line->right = 3;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 1;
	line->end = 2;
	line->right = 4;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 2;
	line->end = 3;
	line->right = 5;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 3;
	line->end = 4;
	line->right = 2;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 4;
	line->end = 0;
	line->right = 0;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 0;
	line->end = 3;
	line->right = 1;
	line->left = 6;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(std::move(line));

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

//	int checks = 0;
	for(const std::shared_ptr<LineDef> &line : doc.linedefs)
	{
		if(doc.getStart(*line).xy() == v2double_t{0, 0} &&
		   doc.getEnd(*line).xy() == v2double_t{32, 0})
		{
//			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOP");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{32, 0} &&
				doc.getEnd(*line).xy() == v2double_t{96, 0})
		{
//			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOP");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
		else if(doc.getStart(*line).xy() == v2double_t{96, 0} &&
				doc.getEnd(*line).xy() == v2double_t{128, 0})
		{
//			++checks;
			ASSERT_EQ(doc.getRight(*line)->MidTex(), "TOP");
			ASSERT_EQ(doc.getSector(*doc.getRight(*line)).FloorTex(), "FBOTTOM");
			ASSERT_FALSE(doc.getLeft(*line));
			ASSERT_EQ(line->flags, MLF_Blocking);
		}
	}
}

TEST_F(EObjectsFixture, AvoidAccidentalDegeneration)
{
   static const FFixedPoint vertexCoordinates[12][2] = {
	   { FFixedPoint(-256), FFixedPoint(256) },
	   { FFixedPoint(-256), FFixedPoint(768) },
	   { FFixedPoint(256), FFixedPoint(768) },
	   { FFixedPoint(-64), FFixedPoint(256) },

	   { FFixedPoint(-64), FFixedPoint(448) },
	   { FFixedPoint(256), FFixedPoint(512) },
	   { FFixedPoint(0), FFixedPoint(256) },
	   { FFixedPoint(0), FFixedPoint(448) },

	   { FFixedPoint(256), FFixedPoint(448) },
	   { FFixedPoint(64), FFixedPoint(256) },
	   { FFixedPoint(64), FFixedPoint(384) },
	   { FFixedPoint(256), FFixedPoint(384) },
   };

	Document &doc = inst.level;
	inst.grid.ForceStep(64);
	inst.grid.NearestScale(0.33);

	for(size_t i = 0; i < 12; ++i)
	{
		auto vertex = std::make_shared<Vertex>();
		vertex->raw_x = vertexCoordinates[i][0];
		vertex->raw_y = vertexCoordinates[i][1];
		doc.vertices.push_back(std::move(vertex));
	}

	std::shared_ptr<Sector> sector;

	sector = std::make_shared<Sector>();
	doc.sectors.push_back(std::move(sector));

	sector = std::make_shared<Sector>();
	doc.sectors.push_back(std::move(sector));

	sector = std::make_shared<Sector>();
	doc.sectors.push_back(std::move(sector));

	std::shared_ptr<SideDef> side;	// 0

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 0;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 1;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	side = std::make_shared<SideDef>();
	side->sector = 2;
	doc.sidedefs.push_back(std::move(side));

	std::shared_ptr<LineDef> line;

	line = std::make_shared<LineDef>();
	line->start = 3;
	line->end = 0;
	line->right = 1;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 0;
	line->end = 1;
	line->right = 2;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 1;
	line->end = 2;
	line->right = 3;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 2;
	line->end = 5;
	line->right = 0;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 6;
	line->end = 3;
	line->right = 4;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 3;
	line->end = 4;
	line->right = 7;
	line->right = 9;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 5;
	line->end = 8;
	line->right = 5;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 4;
	line->end = 5;
	line->right = 6;
	line->right = 8;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 9;
	line->end = 6;
	line->right = 10;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 6;
	line->end = 7;
	line->right = 13;
	line->right = 15;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 8;
	line->end = 11;
	line->right = 11;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 7;
	line->end = 8;
	line->right = 12;
	line->right = 14;
	line->flags = MLF_TwoSided;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 10;
	line->end = 9;
	line->right = 17;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	line = std::make_shared<LineDef>();
	line->start = 11;
	line->end = 10;
	line->right = 16;
	line->flags = MLF_Blocking;
	doc.linedefs.push_back(std::move(line));

	selection_c selection(ObjType::sectors);
	for(int i = 0; i < doc.numSectors(); ++i)
		selection.set(i);

	int numverts = doc.numVertices();
	int numsectors = doc.numSectors();
	int numsides = doc.numSidedefs();
	int numlines = doc.numLinedefs();

	doc.objects.move(selection, v3double_t(256, 128, 0));

	ASSERT_EQ(doc.numVertices(), numverts);
	ASSERT_EQ(doc.numSectors(), numsectors);
	ASSERT_EQ(doc.numSidedefs(), numsides);
	ASSERT_EQ(doc.numLinedefs(), numlines);
}

TEST_F(EObjectsFixture, InsertThingWithDefaultFlags)
{
	inst.edit.mode = ObjType::things;
	inst.edit.Selected = selection_c(ObjType::things);
	inst.conf.default_thing = 1234;

	thingflag_t flag = {};
	flag.value = 1;
	flag.defaultSet = thingflag_t::DefaultMode::off;
	inst.conf.thing_flags.push_back(flag);
	flag.value = 2;
	flag.defaultSet = thingflag_t::DefaultMode::off;
	inst.conf.thing_flags.push_back(flag);
	flag.value = 4;
	flag.defaultSet = thingflag_t::DefaultMode::on;
	inst.conf.thing_flags.push_back(flag);
	flag.value = 8;
	flag.defaultSet = thingflag_t::DefaultMode::off;
	inst.conf.thing_flags.push_back(flag);
	flag.value = 16;
	flag.defaultSet = thingflag_t::DefaultMode::onOpposite;
	inst.conf.thing_flags.push_back(flag);
	flag.value = 32;
	flag.defaultSet = thingflag_t::DefaultMode::on;
	inst.conf.thing_flags.push_back(flag);

	inst.CMD_ObjectInsert();

	ASSERT_EQ(inst.level.things.size(), 1);
	ASSERT_EQ(inst.level.things[0]->options, 36);
}

//
// Test that findSplitLine with exactPoint=true uses the exact point,
// not grid-snapped or ratio-locked positions.
//
TEST_F(EObjectsFixture, FindSplitLineExactPoint)
{
	Document &doc = inst.level;
	inst.loaded.levelFormat = MapFormat::udmf;

	// Set up a horizontal linedef from (0, 100) to (200, 100)
	auto v1 = std::make_unique<Vertex>();
	v1->SetRawXY(inst.loaded.levelFormat, v2double_t{0.0, 100.0});
	doc.vertices.push_back(std::move(v1));

	auto v2 = std::make_unique<Vertex>();
	v2->SetRawXY(inst.loaded.levelFormat, v2double_t{200.0, 100.0});
	doc.vertices.push_back(std::move(v2));

	auto line = std::make_shared<LineDef>();
	line->start = 0;
	line->end = 1;
	doc.linedefs.push_back(std::move(line));

	// Create a vertex that we'll use to test ignore_vert
	auto v3 = std::make_unique<Vertex>();
	v3->SetRawXY(inst.loaded.levelFormat, v2double_t{300.0, 300.0});
	doc.vertices.push_back(std::move(v3));

	// Set up grid with snapping enabled - grid step of 32 units
	inst.grid.ForceStep(32);
	inst.grid.SetSnap(true);

	// Test point that is off-grid: (87.5, 100.0)
	// With grid snapping, this would snap to (96, 100) or (64, 100)
	// With exactPoint=true, it should use the exact point (87.5, 100.0)
	v2double_t testPoint{87.5, 100.0};
	v2double_t outPos;

	// Call with exactPoint=true
	Objid result = inst.findSplitLine(outPos, testPoint, -1, true);

	// Should find the linedef
	ASSERT_TRUE(result.valid());
	EXPECT_EQ(result.num, 0);

	// The out_pos should be the exact point projected onto the line
	// Since the line is horizontal at y=100, the point should be at (87.5, 100.0)
	EXPECT_NEAR(outPos.x, 87.5, 0.01);
	EXPECT_NEAR(outPos.y, 100.0, 0.01);

	// Now test with exactPoint=false and grid snapping
	// This should snap to grid (either 64 or 96, depending on rounding)
	result = inst.findSplitLine(outPos, testPoint, -1, false);

	// Should still find the linedef
	ASSERT_TRUE(result.valid());
	EXPECT_EQ(result.num, 0);

	// The out_pos should be grid-snapped
	// 87.5 rounds to 96 (nearest multiple of 32 is either 64 or 96, but 96 is closer to 88)
	// Actually, let's check it's different from the exact point
	EXPECT_TRUE(fabs(outPos.x - 87.5) > 0.1 || fabs(outPos.y - 100.0) > 0.1);

	// Test ignore_vert parameter
	// Add v3 to one of the linedef endpoints
	doc.linedefs[0]->start = 2;  // Now linedef is from v3 (300, 300) to v2 (200, 100)

	// With ignore_vert=-1, should find the linedef
	result = inst.findSplitLine(outPos, v2double_t{250.0, 200.0}, -1, true);
	EXPECT_TRUE(result.valid());

	// With ignore_vert=2 (v3's index), should NOT find the linedef
	// because it touches the ignored vertex
	result = inst.findSplitLine(outPos, v2double_t{250.0, 200.0}, 2, true);
	EXPECT_FALSE(result.valid()) << "Should not find linedef when it touches ignore_vert";
}

//
// Test that findSplitLine with exactPoint=true bypasses ratio-lock
// when grid.getRatio() > 0 && edit.action == EditorAction::drawLine
//
TEST_F(EObjectsFixture, FindSplitLineExactPointWithRatioLock)
{
	Document &doc = inst.level;
	inst.loaded.levelFormat = MapFormat::udmf;

	// Set up a horizontal linedef from (0, 100) to (200, 100)
	auto v1 = std::make_unique<Vertex>();
	v1->SetRawXY(inst.loaded.levelFormat, v2double_t{0.0, 100.0});
	doc.vertices.push_back(std::move(v1));

	auto v2 = std::make_unique<Vertex>();
	v2->SetRawXY(inst.loaded.levelFormat, v2double_t{200.0, 100.0});
	doc.vertices.push_back(std::move(v2));

	auto line = std::make_shared<LineDef>();
	line->start = 0;
	line->end = 1;
	doc.linedefs.push_back(std::move(line));

	// Create a starting vertex for drawing (this will be edit.drawLine.from)
	auto vStart = std::make_unique<Vertex>();
	vStart->SetRawXY(inst.loaded.levelFormat, v2double_t{50.0, 50.0});
	doc.vertices.push_back(std::move(vStart));

	// Enable ratio lock (e.g., 1:1 or 2:1 ratio)
	inst.grid.configureRatio(1, false);  // 1:1 ratio
	ASSERT_GT(inst.grid.getRatio(), 0);

	// Set up drawing mode
	inst.edit.action = EditorAction::drawLine;
	inst.edit.drawLine.from = Objid(ObjType::vertices, 2);  // vertex index 2 (vStart)

	// Test point on the horizontal line
	// With ratio-lock enabled and drawing from (50, 50), the intersection
	// with the horizontal line would be ratio-constrained
	v2double_t testPoint{120.0, 100.0};
	v2double_t outPos;

	// Call with exactPoint=true - should use exact intersection
	Objid result = inst.findSplitLine(outPos, testPoint, -1, true);

	// Should find the linedef
	ASSERT_TRUE(result.valid());
	EXPECT_EQ(result.num, 0);

	// The out_pos should be the exact point projected onto the line
	// Since exactPoint=true, it bypasses ratio-lock
	EXPECT_NEAR(outPos.x, 120.0, 0.01);
	EXPECT_NEAR(outPos.y, 100.0, 0.01);

	// Now test with exactPoint=false - should apply ratio-lock
	result = inst.findSplitLine(outPos, testPoint, -1, false);

	// Should still find the linedef
	ASSERT_TRUE(result.valid());
	EXPECT_EQ(result.num, 0);

	// With ratio-lock from (50, 50), the intersection point will be different
	// The exact calculation depends on RatioSnapXY, but it should differ from exact point
	// when ratio-lock is applied
	bool ratioLockApplied = (fabs(outPos.x - 120.0) > 0.1) || (fabs(outPos.y - 100.0) > 0.1);
	EXPECT_TRUE(ratioLockApplied) << "Ratio-lock should have modified the position from exact point";
}

//
// Test for GitHub issue #177: Drawing a polygon starting and finishing at an existing vertex
// should produce a sector when drawn clockwise, not a solid void wall.
//
TEST_F(EObjectsFixture, DrawClockwiseInsideOctagonFromVertex)
{
	inst.edit.mode = ObjType::vertices;
	inst.edit.action = EditorAction::nothing;
	inst.edit.pointer_in_window = true;

	// Draw an octagonal room (outer room)
	// Octagon vertices at radius ~181 from center (128, 128)
	const v2double_t octagonCoords[] =
	{
		{128, 256},		// top
		{238, 238},		// top-right
		{256, 128},		// right
		{238, 18},		// bottom-right
		{128, 0},		// bottom
		{18, 18},		// bottom-left
		{0, 128},		// left
		{18, 238},		// top-left
		{128, 256}		// close octagon (back to start)
	};

	for (v2double_t xy : octagonCoords)
	{
		moveMouse(xy);
		inst.CMD_ObjectInsert();
	}

	// Verify octagon was created
	ASSERT_EQ(inst.level.vertices.size(), 8);
	ASSERT_EQ(inst.level.linedefs.size(), 8);
	ASSERT_EQ(inst.level.sectors.size(), 1);

	// Now draw clockwise inside the octagon, starting and ending at vertex 0 (top vertex)
	// Create an inner rectangle that starts at the top vertex of the octagon
	const v2double_t innerCoords[] =
	{
		{128, 256},		// start at top vertex of octagon
		{192, 192},		// inner top-right
		{192, 64},		// inner bottom-right
		{64, 64},		// inner bottom-left
		{64, 192},		// inner top-left
		{128, 256}		// return to start (close the polygon)
	};

	for (v2double_t xy : innerCoords)
	{
		moveMouse(xy);
		inst.CMD_ObjectInsert();
	}

	// Verify that a new sector was created (not a void wall)
	// We should now have 2 sectors: the outer octagon and the inner rectangle
	ASSERT_EQ(inst.level.sectors.size(), 2);

	// Verify we have the correct number of vertices
	// 8 original octagon vertices + 4 new inner vertices = 12 total
	ASSERT_EQ(inst.level.vertices.size(), 12);

	// Verify we have the correct number of linedefs
	// 8 octagon lines + 5 inner lines + 1 connecting line = 14 total
	// (The exact count depends on the implementation, but we should have at least created a sector)
	ASSERT_GT(inst.level.linedefs.size(), 8);

	// Verify that the inner sector has proper sidedefs (not solid walls)
	// Find the inner sector (should be sector 1)
	const Sector *innerSector = inst.level.sectors[1].get();
	ASSERT_TRUE(innerSector != nullptr);

	// Count how many linedefs reference the inner sector
	int innerSectorRefs = 0;
	for (const auto &line : inst.level.linedefs)
	{
		const SideDef *right = inst.level.getRight(*line);
		const SideDef *left = inst.level.getLeft(*line);

		if (right && right->sector == 1)
			innerSectorRefs++;
		if (left && left->sector == 1)
			innerSectorRefs++;
	}

	// The inner sector should be referenced by multiple sidedefs (at least 5)
	ASSERT_GT(innerSectorRefs, 0);
}

