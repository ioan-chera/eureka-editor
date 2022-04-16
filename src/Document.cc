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
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"

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
		ChecksumThing(crc, things[i]);

	for(i = 0; i < numLinedefs(); i++)
		ChecksumLineDef(crc, linedefs[i], *this);
}
