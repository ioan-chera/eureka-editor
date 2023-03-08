//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2023      Ioan Chera
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#include "Instance.h"
#include "w_rawdef.h"
#include <queue>

struct SideIteration
{
	byte parts;
	Side side;
	byte lower;
	byte rail;
	byte upper;
};

//
// Given a source line, a next line, and their facing sides, it returns the visible faces which
// touch each other for selecting neighbors by texture.
//

struct WallContinuity
{
	WallContinuity &moveLeft(Side side);
	byte otherParts(byte parts, const SideIteration &iteration) const;

	byte top, middle, bottom;
};

WallContinuity &WallContinuity::moveLeft(Side side)
{
	if(side == Side::left)
	{
		top *= PART_LF_LOWER / PART_RT_LOWER;	// gross hack to move flags
		middle *= PART_LF_LOWER / PART_RT_LOWER;
		bottom *= PART_LF_LOWER / PART_RT_LOWER;
	}
	return *this;
}

byte WallContinuity::otherParts(byte parts, const SideIteration &iteration) const
{
	return (parts & iteration.lower ? bottom : 0) | (parts & iteration.rail ? middle : 0) |
		   (parts & iteration.upper ? top : 0);
}

static WallContinuity getWallTextureContinuity(const Instance &inst, const LineDef &source,
											   Side sourceSide, const LineDef &next, Side nextSide)
{
	const Document &doc = inst.level;
	WallContinuity result = {};

	const SideDef *sourceFront = doc.getSide(source, sourceSide);
	const Sector *sourceFrontSector = sourceFront ? &doc.getSector(*sourceFront) : nullptr;
	const SideDef *sourceBack = doc.getSide(source, -sourceSide);
	const Sector *sourceBackSector = sourceBack ? &doc.getSector(*sourceBack) : nullptr;
	const SideDef *nextFront = doc.getSide(next, nextSide);
	const Sector *nextFrontSector = nextFront ? &doc.getSector(*nextFront) : nullptr;
	const SideDef *nextBack = doc.getSide(next, -nextSide);
	const Sector *nextBackSector = nextBack ? &doc.getSector(*nextBack) : nullptr;

	// Shut lines are over. Same if they have no common heights.
	if(!sourceFrontSector || !nextFrontSector ||
		sourceFrontSector->ceilh <= sourceFrontSector->floorh ||
		nextFrontSector->ceilh <= nextFrontSector->floorh ||
		sourceFrontSector->floorh >= nextFrontSector->ceilh ||
		nextFrontSector->floorh >= sourceFrontSector->ceilh)
	{
		return result;
	}

	// WARNING: one-sided lines actually use the lower part, not the rail!

	// If source is two-sided...
	if(sourceBackSector)
	{
		// We have top, so check it
		if(sourceFrontSector->ceilh > sourceBackSector->ceilh)
		{
			// 1-sided next
			if(!nextBackSector)
			{
				if(nextFrontSector->ceilh > sourceBackSector->ceilh &&
				   nextFront->MidTex() == sourceFront->UpperTex())
				{
					result.top |= PART_RT_LOWER;
				}
			}
			else // 2-sided next
			{
				// Continuous top facet
				if(nextFrontSector->ceilh > nextBackSector->ceilh &&
					nextFrontSector->ceilh > sourceBackSector->ceilh &&
					sourceFrontSector->ceilh > nextBackSector->ceilh &&
				   nextFront->UpperTex() == sourceFront->UpperTex())
				{
					result.top |= PART_RT_UPPER;
				}
				// Continuous top to bottom facet. Cannot match with railings this way (extremities
				// already tested)
				if(nextBackSector->floorh > nextFrontSector->floorh &&
					nextBackSector->floorh > sourceBackSector->ceilh &&
				   nextFront->LowerTex() == sourceFront->UpperTex())
				{
					result.top |= PART_RT_LOWER;
				}
			}
		}
		// We have bottom, so check it
		if(sourceBackSector->floorh > sourceFrontSector->floorh)
		{
			// 1-sided next
			if(!nextBackSector)
			{
				if(sourceBackSector->floorh > nextFrontSector->floorh &&
				   nextFront->MidTex() == sourceFront->LowerTex())
				{
					result.bottom |= PART_RT_LOWER;
				}
			}
			else // 2-sided next
			{
				// Continuous bottom facet
				if(nextBackSector->floorh > nextFrontSector->floorh &&
					nextBackSector->floorh > sourceFrontSector->floorh &&
					sourceBackSector->floorh > nextFrontSector->floorh &&
				   nextFront->LowerTex() == sourceFront->LowerTex())
				{
					result.bottom |= PART_RT_LOWER;
				}
				// Continuous bottom to top facet. Same thing about railings not being compatible.
				if(nextFrontSector->ceilh > nextBackSector->ceilh &&
					sourceBackSector->floorh > nextBackSector->ceilh &&
				   nextFront->UpperTex() == sourceFront->LowerTex())
				{
					result.bottom |= PART_RT_UPPER;
				}
			}
		}
		// We have window, so check middle (railing) continuity
		if(sourceBackSector->ceilh > sourceBackSector->floorh && nextBackSector &&
			nextBackSector->ceilh > nextBackSector->floorh &&
			sourceBackSector->ceilh > nextBackSector->floorh &&
			nextBackSector->ceilh > sourceBackSector->floorh &&
		   nextFront->MidTex() == sourceFront->MidTex())
		{
			result.middle |= PART_RT_RAIL;
		}
	}
	else // 1-sided source
	{
		// simple wall-wall continuity
		if(!nextBackSector)
		{
			if(nextFront->MidTex() == sourceFront->MidTex())
				result.bottom |= PART_RT_LOWER;
		}
		else // wall - 2-sided continuity
		{
			// wall - top continuity
			if(nextFrontSector->ceilh > nextBackSector->ceilh &&
				sourceFrontSector->ceilh > nextBackSector->ceilh &&
			   nextFront->UpperTex() == sourceFront->MidTex())
			{
				result.bottom |= PART_RT_UPPER;
			}
			if(nextBackSector->floorh > nextFrontSector->floorh &&
				nextBackSector->floorh > sourceFrontSector->floorh &&
			   nextFront->LowerTex() == sourceFront->MidTex())
			{
				result.bottom |= PART_RT_LOWER;
			}
		}
	}

	return result.moveLeft(nextSide);
}

static WallContinuity matchingWallHeights(const Instance &inst, const LineDef &source,
	Side sourceSide, const LineDef &next, Side nextSide)
{
	WallContinuity result = {};

	const Document &doc = inst.level;
	const SideDef *sourceSidedef = doc.getSide(source, sourceSide);
	const SideDef *nextSidedef = doc.getSide(next, nextSide);
	if(!sourceSidedef || !nextSidedef)
		return result;
	const Sector &sourceSector = doc.getSector(*sourceSidedef);
	const Sector &nextSector = doc.getSector(*nextSidedef);

	// Any shut sector is removed
	if(sourceSector.floorh >= sourceSector.ceilh || nextSector.floorh >= nextSector.ceilh)
		return result;

	const SideDef *sourceBack = doc.getSide(source, -sourceSide);
	const SideDef *nextBack = doc.getSide(next, -nextSide);
	const Sector *sourceBackSector = sourceBack ? &doc.getSector(*sourceBack) : nullptr;
	const Sector *nextBackSector = nextBack ? &doc.getSector(*nextBack) : nullptr;

	// Check lower

	int bottom, top;
	auto checkNextLine = [&bottom, &top, nextBack, &nextSector, nextBackSector]() -> byte
	{
		if(nextBack)
		{
			if(std::max(nextSector.floorh, nextBackSector->ceilh) == bottom && nextSector.ceilh == top)
				return PART_RT_UPPER;
			if(std::min(nextSector.ceilh, nextBackSector->floorh) == top && nextSector.floorh == bottom)
				return PART_RT_LOWER;
		}
		else
		{
			if(nextSector.ceilh == top && nextSector.floorh == bottom)
				return PART_RT_LOWER;
		}
		return 0;
	};
	if(sourceBack)
	{
		if(sourceBackSector->floorh > sourceSector.floorh)
		{
			bottom = sourceSector.floorh;
			top = std::min(sourceSector.ceilh, sourceBackSector->floorh);
			result.bottom = checkNextLine();
		}
		if(sourceBackSector->ceilh < sourceSector.ceilh)
		{
			bottom = std::max(sourceSector.floorh, sourceBackSector->ceilh);
			top = sourceSector.ceilh;
			result.top = checkNextLine();
		}
		if(sourceBackSector->ceilh > sourceSector.floorh &&
			sourceSector.ceilh > sourceBackSector->floorh &&
			sourceBackSector->ceilh > sourceBackSector->floorh && nextBack)
		{
			int lowestceil1 = std::min(sourceSector.ceilh, sourceBackSector->ceilh);
			int highestfloor1 = std::max(sourceSector.floorh, sourceBackSector->floorh);
			int midheight1 = lowestceil1 - highestfloor1;

			int lowestceil2 = std::min(nextSector.ceilh, nextBackSector->ceilh);
			int highestfloor2 = std::max(nextSector.floorh, nextBackSector->floorh);
			int midheight2 = lowestceil2 - highestfloor2;

			int texheight1 = inst.wad.images.W_GetTextureHeight(inst.conf, sourceSidedef->MidTex());
			int texheight2 = inst.wad.images.W_GetTextureHeight(inst.conf, nextSidedef->MidTex());

			auto translatedOffset = [](const LineDef &line, const SideDef &side, int midheight,
				int texheight)
			{
				if(line.flags & MLF_LowerUnpegged)
					return texheight - midheight + side.y_offset;
				return side.y_offset;
			};

			if(midheight1 == midheight2 && midheight1 <= std::min(texheight1, texheight2)
				&& lowestceil1 == lowestceil2 && highestfloor1 == highestfloor2)
			{
				result.middle = PART_RT_RAIL;
			}
			else if(texheight1 == texheight2 && texheight1 <= std::min(midheight1, midheight2)
				&& translatedOffset(source, *sourceSidedef, midheight1, texheight1) ==
				translatedOffset(next, *nextSidedef, midheight2, texheight2))
			{
				result.middle = PART_RT_RAIL;
			}
		}
	}
	else
	{
		// single sided
		bottom = sourceSector.floorh;
		top = sourceSector.ceilh;
		result.bottom = checkNextLine();
	}
	return result.moveLeft(nextSide);
}

static std::vector<std::vector<int>> makeVertexLineMap(const Document &doc)
{
	std::vector<std::vector<int>> result;
	result.resize(doc.numVertices());
	for(int i = 0; i < doc.numLinedefs(); ++i)
	{
		const auto &line = doc.linedefs[i];
		for(int vertNum : {line->start, line->end})
			if(doc.isVertex(vertNum))
				result[vertNum].push_back(i);
	}
	return result;
}


static void selectNeighborLines(Instance &inst, int objnum, byte parts, WallContinuity (*func)(
	const Instance &inst, const LineDef &source, Side sourceSide, const LineDef &next, 
	Side nextSide))
{
	const Document &doc = inst.level;
	if(!doc.isLinedef(objnum) || !(parts & (PART_RT_ALL | PART_LF_ALL)))
		return;

	auto vertLineMap = makeVertexLineMap(doc);

	const auto &source = doc.linedefs[objnum];
	struct Entry
	{
		const LineDef *line;
		byte parts;
	};
	std::queue<Entry> queue;
	queue.push({source.get(), parts});

	// Also select the current line
	inst.edit.Selected->set_ext(objnum, inst.edit.Selected->get_ext(objnum) | parts);

	while(!queue.empty())
	{
		Entry entry = queue.front();
		queue.pop();

		for(int vertNum : {entry.line->start, entry.line->end})
		{
			for(int neigh : vertLineMap[vertNum])
			{
				const auto &otherLine = doc.linedefs[neigh];
				if(otherLine.get() == entry.line)
					continue;
				bool flipped = otherLine->start == entry.line->start ||
							   otherLine->end == entry.line->end;

				for(auto iteration : {SideIteration{PART_RT_ALL, Side::right,
													PART_RT_LOWER, PART_RT_RAIL, PART_RT_UPPER},
									  SideIteration{PART_LF_ALL, Side::left,
													PART_LF_LOWER, PART_LF_RAIL, PART_LF_UPPER}})
				{
					if(entry.parts & iteration.parts)
					{
						WallContinuity continuity = func(inst, *entry.line, iteration.side, 
							*otherLine, flipped ? -iteration.side : iteration.side);
						byte otherParts = continuity.otherParts(entry.parts, iteration);
						byte otherCurrentlySelected = inst.edit.Selected->get_ext(neigh);
						if((otherCurrentlySelected & otherParts) < otherParts)
						{
							inst.edit.Selected->set_ext(neigh, otherCurrentlySelected | otherParts);
							queue.push({otherLine.get(), otherParts});
						}
					}
				}
			}
		}
	}
}

void Instance::SelectNeighborSectors(int objnum, SelectNeighborCriterion option, byte parts)
{
	const auto &sector1 = level.sectors[objnum];

	for (const auto &line : level.linedefs)
	{
		if (line->OneSided())
			continue;

		if (level.getRight(*line)->sector == objnum || level.getLeft(*line)->sector == objnum)
		{
			const Sector *sector2;
			int sectornum;

			bool match = false;

			if (level.getRight(*line)->sector == objnum)
			{
				sector2 = &level.getSector(*level.getLeft(*line));
				sectornum = level.getLeft(*line)->sector;
			}
			else
			{
				sector2 = &level.getSector(*level.getRight(*line));
				sectornum = level.getRight(*line)->sector;
			}

			if (edit.Selected->get(sectornum))
				continue;

			if (option == SelectNeighborCriterion::texture)
			{
				if (parts & PART_FLOOR)
					match = (sector1->FloorTex() == sector2->FloorTex());
				else
					match = (sector1->CeilTex() == sector2->CeilTex());
			}
			else
			{
				if (parts & PART_FLOOR)
					match = (sector1->floorh == sector2->floorh);
				else
					match = (sector1->ceilh == sector2->ceilh);
			}

			if (match)
			{
				edit.Selected->set_ext(sectornum, parts);
				SelectNeighborSectors(sectornum, option, parts);
			}
		}
	}

}

void Instance::CMD_SelectNeighbors()
{
	SString option = EXEC_Param[0];

	if (edit.mode != ObjType::linedefs && edit.mode != ObjType::sectors)
		return;

	if (option != "height" && option != "texture")
		return;

	SelectNeighborCriterion criterion = option == "height" ? SelectNeighborCriterion::height :
															 SelectNeighborCriterion::texture;

	if (edit.highlight.num < 0)
		return;

	int num = edit.highlight.num;
	byte parts = static_cast<byte>(edit.highlight.parts);

	if (edit.Selected->get(num) && edit.Selected->get_ext(num) & parts)
	{
		CMD_UnselectAll();
	}
	else
	{
		edit.Selected->set_ext(num, edit.Selected->get_ext(num) | parts);
		if (edit.mode == ObjType::linedefs)
		{
			if(criterion == SelectNeighborCriterion::texture)
				selectNeighborLines(*this, num, parts, getWallTextureContinuity);
			else
				selectNeighborLines(*this, num, parts, matchingWallHeights);
		}
		else
		{
			SelectNeighborSectors(num, criterion, parts);
		}

	}
	RedrawMap();
}
