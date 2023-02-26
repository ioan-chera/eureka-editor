//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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
#include "lib_adler.h"
#include "LineDef.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"

void Document::trimVertexArray(int count)
{
	if(count < (int)vertices.size())
		vertices.resize(count);
}

void Document::addVertex(const Vertex &vertex)
{
	VertexInfo info{};
	info.vertex = vertex;
	vertices.push_back(info);
}

void Document::insertVertex(const Vertex &vertex, int index)
{
	VertexInfo info{};
	info.vertex = vertex;
	vertices.insert(vertices.begin() + index, info);

	// fix references in linedefs

	if(index + 1 < numVertices())
		for(auto it = linedefs.rbegin(); it != linedefs.rend(); ++it)
		{
			auto &L = *it;
			if(L.start >= index)
				L.start++;
			if(L.end >= index)
				L.end++;
		}
}

Vertex Document::removeVertex(int index)
{
	auto result = vertices[index];
	vertices.erase(vertices.begin() + index);

	// fix the linedef references
	if(index < numVertices())
		for(auto it = linedefs.rbegin(); it != linedefs.rend(); ++it)
		{
			auto &L = *it;
			if(L.start > index)
				L.start--;
			if(L.end > index)
				L.end--;
		}

	return result.vertex;
}

void Document::deleteAllLinedefs()
{
	linedefs.clear();
	//for(auto &info : vertices)
	//	info.lines.clear();
}

LineDef Document::removeLinedef(int index)
{
	auto result = linedefs[index];
	linedefs.erase(linedefs.begin() + index);


	// Now also adjust vertices
	// Also adjust counts
	//for(auto &info : vertices)
	//	info.removeLineAndShiftBack(index);

	return result;
}


//
// Get number of objects based on enum
//
int Document::numObjects(ObjType type) const
{
	switch(type)
	{
	case ObjType::things:
		return numThings();
	case ObjType::linedefs:
		return numLinedefs();
	case ObjType::sidedefs:
		return numSidedefs();
	case ObjType::vertices:
		return numVertices();
	case ObjType::sectors:
		return numSectors();
	default:
		return 0;
	}
}

//------------------------------------------------------------------------
//   CHECKSUM LOGIC
//------------------------------------------------------------------------

static void ChecksumThing(crc32_c &crc, const Thing &T)
{
	crc += T.raw_x.raw();
	crc += T.raw_y.raw();
	crc += T.angle;
	crc += T.type;
	crc += T.options;
}

static void ChecksumVertex(crc32_c &crc, const Vertex &V)
{
	crc += V.raw_x.raw();
	crc += V.raw_y.raw();
}

static void ChecksumSector(crc32_c &crc, const Sector *sector)
{
	crc += sector->floorh;
	crc += sector->ceilh;
	crc += sector->light;
	crc += sector->type;
	crc += sector->tag;

	crc += sector->FloorTex();
	crc += sector->CeilTex();
}

static void ChecksumSideDef(crc32_c &crc, const SideDef *S, const Document &doc)
{
	crc += S->x_offset;
	crc += S->y_offset;

	crc += S->LowerTex();
	crc += S->MidTex();
	crc += S->UpperTex();

	ChecksumSector(crc, &doc.getSector(*S));
}

static void ChecksumLineDef(crc32_c &crc, const LineDef &L, const Document &doc)
{
	crc += L.flags;
	crc += L.type;
	crc += L.tag;

	ChecksumVertex(crc, doc.getStart(L));
	ChecksumVertex(crc, doc.getEnd(L));

	if(doc.getRight(L))
		ChecksumSideDef(crc, doc.getRight(L), doc);

	if(doc.getLeft(L))
		ChecksumSideDef(crc, doc.getLeft(L), doc);
}

//
// compute a checksum for the current level
//
void Document::getLevelChecksum(crc32_c &crc) const
{
	// the following method conveniently skips any unused vertices,
	// sidedefs and sectors.  It also adds each sector umpteen times
	// (for each line in the sector), but that should not affect the
	// validity of the final checksum.

	int i;

	for(i = 0; i < numThings(); i++)
		ChecksumThing(crc, getThing(i));

	for(i = 0; i < numLinedefs(); i++)
		ChecksumLineDef(crc, linedefs[i], *this);
}

const Sector &Document::getSector(const SideDef &side) const
{
	return sectors[side.sector];
}

int Document::getSectorID(const LineDef &line, Side side) const
{
	switch(side)
	{
	case Side::left:
		return getLeft(line) ? getLeft(line)->sector : -1;

	case Side::right:
		return getRight(line) ? getRight(line)->sector : -1;

	default:
		return -1;
	}
}

const Vertex &Document::getStart(const LineDef &line) const
{
	return vertices[line.start].vertex;
}

const Vertex &Document::getEnd(const LineDef &line) const
{
	return vertices[line.end].vertex;
}

const SideDef *Document::getRight(const LineDef &line) const
{
	return line.right >= 0 ? &sidedefs[line.right] : nullptr;
}

const SideDef *Document::getLeft(const LineDef &line) const
{
	return line.left >= 0 ? &sidedefs[line.left] : nullptr;
}

double Document::calcLength(const LineDef &line) const
{
	double dx = getStart(line).x() - getEnd(line).x();
	double dy = getStart(line).y() - getEnd(line).y();
	return hypot(dx, dy);
}

bool Document::touchesCoord(const LineDef &line, FFixedPoint tx, FFixedPoint ty) const
{
	return getStart(line).Matches(tx, ty) || getEnd(line).Matches(tx, ty);
}

bool Document::touchesSector(const LineDef &line, int secNum) const
{
	if(line.right >= 0 && sidedefs[line.right].sector == secNum)
		return true;
	if(line.left >= 0 && sidedefs[line.left].sector == secNum)
		return true;
	return false;
}

bool Document::isZeroLength(const LineDef &line) const
{
	return (getStart(line).raw_x == getEnd(line).raw_x) && (getStart(line).raw_y == getEnd(line).raw_y);
}

bool Document::isSelfRef(const LineDef &line) const
{
	return (line.left >= 0) && (line.right >= 0) &&
		sidedefs[line.left].sector == sidedefs[line.right].sector;
}

bool Document::isHorizontal(const LineDef &line) const
{
	return (getStart(line).raw_y == getEnd(line).raw_y);
}

bool Document::isVertical(const LineDef &line) const
{
	return (getStart(line).raw_x == getEnd(line).raw_x);
}

//void Document::VertexInfo::removeLineAndShiftBack(int index)
//{
//	for(auto it = lines.rbegin(); it != lines.rend(); ++it)
//		if(*it > index)
//			-- *it;
//		else if(*it == index)
//		{
//			lines.erase(std::next(it).base());
//			return;
//		}
//		else
//			return;
//}
