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

#ifndef Document_hpp
#define Document_hpp

#include "e_basis.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_vertex.h"

//
// The document associated with a file. All stuff will go here
//
struct Document
{
	std::vector<Thing *> things;
	std::vector<Vertex *> vertices;
	std::vector<Sector *> sectors;
	std::vector<SideDef *> sidedefs;
	std::vector<LineDef *> linedefs;

	std::vector<byte> headerData;
	std::vector<byte> behaviorData;
	std::vector<byte> scriptsData;

	Basis basis;
	Hover hover;
	LinedefModule linemod;
	VertexModule vertmod;

	Document() : basis(*this), hover(*this), linemod(*this), vertmod(*this)
	{
	}

	// FIXME: right now these are copied over to DocumentModule
	int numThings() const
	{
		return static_cast<int>(things.size());
	}
	int numVertices() const
	{
		return static_cast<int>(vertices.size());
	}
	int numSectors() const
	{
		return static_cast<int>(sectors.size());
	}
	int numSidedefs() const
	{
		return static_cast<int>(sidedefs.size());
	}
	int numLinedefs() const
	{
		return static_cast<int>(linedefs.size());
	}
	int numObjects(ObjType type) const;

	void getLevelChecksum(crc32_c &crc) const;
};

extern Document gDocument;


#define NumThings     ((int)gDocument.things.size())
#define NumVertices   ((int)gDocument.vertices.size())
#define NumSectors    ((int)gDocument.sectors.size())
#define NumSideDefs   ((int)gDocument.sidedefs.size())
#define NumLineDefs   ((int)gDocument.linedefs.size())

#define is_thing(n)    ((n) >= 0 && (n) < NumThings  )
#define is_vertex(n)   ((n) >= 0 && (n) < NumVertices)
#define is_sector(n)   ((n) >= 0 && (n) < NumSectors )
#define is_sidedef(n)  ((n) >= 0 && (n) < NumSideDefs)
#define is_linedef(n)  ((n) >= 0 && (n) < NumLineDefs)

#endif /* Document_hpp */
