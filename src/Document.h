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
#include "e_checks.h"
#include "e_hover.h"
#include "e_linedef.h"
#include "e_objects.h"
#include "e_sector.h"
#include "e_vertex.h"
#include "Errors.h"
#include "LineDef.h"
#include "Vertex.h"
#include <memory>

class crc32_c;
class Instance;

//
// The document associated with a file. All stuff will go here
//
struct Document
{
private:
	Instance &inst;	// make this private because we don't want to access it from Document
public:

	std::vector<std::unique_ptr<Thing>> things;
	std::vector<std::unique_ptr<Vertex>> vertices;
	std::vector<std::unique_ptr<Sector>> sectors;
	std::vector<std::unique_ptr<SideDef>> sidedefs;
	std::vector<std::unique_ptr<LineDef>> linedefs;

	std::vector<byte> headerData;
	std::vector<byte> behaviorData;
	std::vector<byte> scriptsData;

	Basis basis;
	ChecksModule checks;
	Hover hover;
	LinedefModule linemod;
	VertexModule vertmod;
	SectorModule secmod;
	ObjectsModule objects;

	explicit Document(Instance &inst) : inst(inst), basis(*this), checks(*this), hover(*this),
	linemod(*this), vertmod(*this), secmod(*this), objects(*this)
	{
	}

	//
	// Count map objects
	//
	int numThings() const
	{
		return static_cast<int>(things.size());
	}
	int numVertices() const noexcept
	{
		return static_cast<int>(vertices.size());
	}
	int numSectors() const noexcept
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
	bool isThing(int n) const
	{
		return n >= 0 && n < numThings();
	}
	bool isVertex(int n) const
	{
		return n >= 0 && n < numVertices();
	}
	bool isSector(int n) const noexcept
	{
		return n >= 0 && n < numSectors();
	}
	bool isSidedef(int n) const
	{
		return n >= 0 && n < numSidedefs();
	}
	bool isLinedef(int n) const
	{
		return n >= 0 && n < numLinedefs();
	}

	int numObjects(ObjType type) const;
	void getLevelChecksum(crc32_c &crc) const;

	const Sector &getSector(const SideDef &side) const;
	int getSectorID(const LineDef &line, Side side) const;
	const Sector *getSector(const LineDef &line, Side side) const;
	const Vertex &getStart(const LineDef &line) const;
	const Vertex &getEnd(const LineDef &line) const;
	const SideDef *getRight(const LineDef &line) const;
	SideDef *getRight(const LineDef &line)
	{
		return const_cast<SideDef *>(static_cast<const Document *>(this)->getRight(line));
	}
	const SideDef *getLeft(const LineDef &line) const;
	SideDef *getLeft(const LineDef &line)
	{
		return const_cast<SideDef *>(static_cast<const Document *>(this)->getLeft(line));
	}
	const SideDef *getSide(const LineDef &line, Side side) const
	{
		return side == Side::right ? getRight(line) : side == Side::left ? getLeft(line) : nullptr;
	}
	double calcLength(const LineDef &line) const;
	bool touchesCoord(const LineDef &line, FFixedPoint tx, FFixedPoint ty) const;
	bool touchesSector(const LineDef &line, int secNum) const;
	bool isZeroLength(const LineDef &line) const;
	bool isSelfRef(const LineDef &line) const;
	bool isHorizontal(const LineDef &line) const;
	bool isVertical(const LineDef &line) const;
	
	void LoadHeader(int loading_level, const Wad_file &load_wad);
	Failable<void> LoadThings(int loading_level, const Wad_file *load_wad);
	Failable<void> LoadThings_Hexen(int loading_level, const Wad_file *load_wad);
	Failable<void> LoadVertices(int loading_level, const Wad_file *load_wad);
	Failable<void> LoadSectors(int loading_level, const Wad_file *load_wad);
	
	void clear();
private:
	friend class DocumentModule;
};

#endif /* Document_hpp */
