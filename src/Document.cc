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
#include "Instance.h"
#include "lib_adler.h"

//
// Adds a point to the bounding box, expanding it as needed
//
void BoundingBox::addPoint(double x, double y)
{
	if (x < x1)
		x1 = x;
	if (y < y1)
		y1 = y;

	if (x > x2)
		x2 = x;
	if (y > y2)
		y2 = y;
}

//
// Clears the bounding box. Typical when no data is available.
//
void BoundingBox::zeroOut()
{
	x1 = y1 = x2 = y2 = 0;
}

//
// Resets to the default value
//
void BoundingBox::reset()
{
	x1 = 32767;
	y1 = 32767;
	x2 = -32767;
	y2 = -32767;
}

//
// The read-only access
//
void BoundingBox::get(double &x1, double &y1, double &x2, double &y2) const
{
	x1 = this->x1;
	y1 = this->y1;
	x2 = this->x2;
	y2 = this->y2;
}

//
// Constructor
//
Document::Document(Instance &inst) : inst(inst),
		checks(*this), hover(*this), linemod(*this), vertmod(*this),
		secmod(*this), objects(*this)
{
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

static void ChecksumThing(crc32_c &crc, const Thing *T)
{
	crc += T->raw_x;
	crc += T->raw_y;
	crc += T->angle;
	crc += T->type;
	crc += T->options;
}

static void ChecksumVertex(crc32_c &crc, const Vertex *V)
{
	crc += V->raw_x;
	crc += V->raw_y;
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

	ChecksumSector(crc, S->SecRef(doc));
}

static void ChecksumLineDef(crc32_c &crc, const LineDef *L, const Document &doc)
{
	crc += L->flags;
	crc += L->type;
	crc += L->tag;

	ChecksumVertex(crc, L->Start(doc));
	ChecksumVertex(crc, L->End(doc));

	if(L->Right(doc))
		ChecksumSideDef(crc, L->Right(doc), doc);

	if(L->Left(doc))
		ChecksumSideDef(crc, L->Left(doc), doc);
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
		ChecksumThing(crc, things[i].get());

	for(i = 0; i < numLinedefs(); i++)
		ChecksumLineDef(crc, linedefs[i].get(), *this);
}

//
// Updates the map bounds from all vertices starting from one
//
void Document::updateMapBoundsStartingFromVertex(int start_vert)
{
	for(int i = start_vert; i < numVertices(); i++)
	{
		const auto &V = vertices[i];
		mapBound.addPoint(V->x(), V->y());
	}
}

//
// Initializes and calculates the map bounds from all vertices
//
void Document::calculateMapBounds()
{
	if (numVertices() == 0)
	{
		mapBound.zeroOut();
		return;
	}

	mapBound.reset();
	updateMapBoundsStartingFromVertex(0);
}
