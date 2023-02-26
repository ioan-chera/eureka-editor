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
#include "LineDef.h"
#include "Vertex.h"
#include <memory>
#include <set>
#include <vector>

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

	class VertexIterator
	{
	public:
		const Vertex *operator ->() const
		{
			return &doc.getVertex(index);
		}
		VertexIterator &operator++()
		{
			++index;
			return *this;
		}
		bool operator != (const VertexIterator &it) const
		{
			return index != it.index;
		}
	private:
		friend struct Document;

		VertexIterator(const Document &doc, int index) : doc(doc), index(index)
		{
		}

		const Document &doc;
		int index;
	};

	VertexIterator vertBegin() const
	{
		return VertexIterator{*this, 0};
	}
	VertexIterator vertEnd() const
	{
		return VertexIterator{*this, (int)vertices.size()};
	}

	void reserveVertexArray(int count)
	{
		vertices.reserve(count);
	}

	void trimVertexArray(int count);
	void addVertex(const Vertex &vertex);
	void insertVertex(const Vertex &vertex, int index);

	void deleteAllVertices()
	{
		vertices.clear();
	}

	Vertex removeVertex(int index);

	Vertex &getMutableVertex(int index)
	{
		return vertices[index].vertex;
	}
	Vertex &getLastMutableVertex()
	{
		return vertices.back().vertex;
	}
	const Vertex &getVertex(int index) const
	{
		return vertices[index].vertex;
	}

	const LineDef &getLinedef(int index) const
	{
		return linedefs[index];
	}
	LineDef &getMutableLinedef(int index)
	{
		return linedefs[index];
	}
	const std::vector<LineDef> &getLinedefs() const
	{
		return linedefs;
	}
	std::vector<LineDef> &getMutableLinedefs()
	{
		return linedefs;
	}

	void deleteAllLinedefs();
	LineDef removeLinedef(int index);

	void insertLinedef(const LineDef &linedef, int index)
	{
		linedefs.insert(linedefs.begin() + index, linedef);
	}
	void addLinedef(const LineDef &linedef)
	{
		linedefs.push_back(linedef);
	}
	LineDef &getLastMutableLinedef()
	{
		return linedefs.back();
	}

	const Thing &getThing(int index) const
	{
		return things[index];
	}

	Thing &getMutableThing(int index)
	{
		return things[index];
	}

	Thing removeThing(int index)
	{
		auto result = getThing(index);
		things.erase(things.begin() + index);
		return result;
	}

	void insertThing(const Thing &thing, int index)
	{
		things.insert(things.begin() + index, thing);
	}

	void deleteAllThings()
	{
		things.clear();
	}

	const std::vector<Thing> &getThings() const
	{
		return things;
	}

	void addThing(const Thing &thing)
	{
		things.push_back(thing);
	}

	Thing &getLastMutableThing()
	{
		return things.back();
	}

	std::vector<Sector> sectors;
	std::vector<SideDef> sidedefs;

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
	bool isThing(int n) const
	{
		return n >= 0 && n < numThings();
	}
	bool isVertex(int n) const
	{
		return n >= 0 && n < numVertices();
	}
	bool isSector(int n) const
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
	double calcLength(const LineDef &line) const;
	bool touchesCoord(const LineDef &line, FFixedPoint tx, FFixedPoint ty) const;
	bool touchesSector(const LineDef &line, int secNum) const;
	bool isZeroLength(const LineDef &line) const;
	bool isSelfRef(const LineDef &line) const;
	bool isHorizontal(const LineDef &line) const;
	bool isVertical(const LineDef &line) const;
private:
	friend class DocumentModule;

	struct VertexInfo
	{
		Vertex vertex;

		std::set<int> linesOut;
		std::set<int> linesIn;
	};

	std::vector<VertexInfo> vertices;
	std::vector<LineDef> linedefs;
	std::vector<Thing> things;
};

#endif /* Document_hpp */
