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

//
// Given a source line, a next line, and their facing sides, it returns the visible faces which
// touch each other for selecting neighbors by texture.
//

struct WallContinuity
{
	byte top;
	byte middle;
	byte bottom;
};

static WallContinuity getWallTextureContinuity(const Document &doc, const LineDef &source,
											   Side sourceSide, const LineDef &next, Side nextSide)
{
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

	// Finally, flip the front/back to match nextSide
	if(nextSide == Side::left)
	{
		result.top *= PART_LF_LOWER / PART_RT_LOWER;	// gross hack to move flags
		result.middle *= PART_LF_LOWER / PART_RT_LOWER;
		result.bottom *= PART_LF_LOWER / PART_RT_LOWER;
	}
	return result;
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

void Instance::SelectNeighborLines_texture(int objnum, byte parts)
{
	if(!level.isLinedef(objnum) || !(parts & (PART_RT_ALL | PART_LF_ALL)))
		return;

	auto vertLineMap = makeVertexLineMap(level);

	const auto &source = level.linedefs[objnum];
	struct Entry
	{
		const LineDef *line;
		byte parts;
	};
	std::queue<Entry> queue;
	queue.push({source.get(), parts});

	// Also select the current line
	edit.Selected->set_ext(objnum, edit.Selected->get_ext(objnum) | parts);

	while(!queue.empty())
	{
		Entry entry = queue.front();
		queue.pop();

		for(int vertNum : {entry.line->start, entry.line->end})
		{
			for(int neigh : vertLineMap[vertNum])
			{
				const auto &otherLine = level.linedefs[neigh];
				if(otherLine.get() == entry.line)
					continue;
				bool flipped = otherLine->start == entry.line->start ||
							   otherLine->end == entry.line->end;

				struct Iteration
				{
					byte parts;
					Side side;
					byte lower;
					byte rail;
					byte upper;
				};

				for(auto iteration : {Iteration{PART_RT_ALL, Side::right,
												PART_RT_LOWER, PART_RT_RAIL, PART_RT_UPPER},
									  Iteration{PART_LF_ALL, Side::left,
												PART_LF_LOWER, PART_LF_RAIL, PART_LF_UPPER}})
				{
					if(entry.parts & iteration.parts)
					{
						WallContinuity continuity = getWallTextureContinuity(level, *entry.line,
								iteration.side, *otherLine, flipped ? -iteration.side :
																	   iteration.side);
						byte otherParts = (entry.parts & iteration.lower ? continuity.bottom : 0) |
										  (entry.parts & iteration.rail  ? continuity.middle : 0) |
										  (entry.parts & iteration.upper ? continuity.top    : 0);
						byte otherCurrentlySelected = edit.Selected->get_ext(neigh);
						if((otherCurrentlySelected & otherParts) < otherParts)
						{
							edit.Selected->set_ext(neigh, otherCurrentlySelected | otherParts);
							queue.push({otherLine.get(), otherParts});
						}
					}
				}
			}
		}
	}
}

static bool checkMatchingSectorHeights(const Instance &inst, byte parts,
									   const LineDef &source, Side sourceSide, const LineDef &next,
									   Side nextSide)
{
	const Document &doc = inst.level;
	const SideDef *sourceSidedef = doc.getSide(source, sourceSide);
	const SideDef *nextSidedef = doc.getSide(next, nextSide);
	if(!sourceSidedef || !nextSidedef)
		return false;
	const Sector &sourceSector = doc.getSector(*sourceSidedef);
	const Sector &nextSector = doc.getSector(*nextSidedef);

	if(parts & (PART_RT_LOWER | PART_LF_LOWER) && sourceSector.floorh != nextSector.floorh)
		return false;
	if(parts & (PART_RT_UPPER | PART_LF_UPPER) && sourceSector.ceilh != nextSector.ceilh)
		return false;
	if(parts & (PART_RT_RAIL | PART_LF_RAIL))
	{
		const Sector *sourceBackSector = doc.getSector(source, -sourceSide);
		if(!sourceBackSector)
			return false;
		const Sector *nextBackSector = doc.getSector(next, -nextSide);
		if(!nextBackSector)
			return false;

		int lowestceil1 = std::min(sourceSector.ceilh, sourceBackSector->ceilh);
		int highestfloor1 = std::max(sourceSector.floorh, sourceBackSector->floorh);
		int midheight1 = lowestceil1 - highestfloor1;

		int lowestceil2 = std::min(nextSector.ceilh, nextBackSector->ceilh);
		int highestfloor2 = std::max(nextSector.floorh, nextBackSector->floorh);
		int midheight2 = lowestceil2 - highestfloor2;

		int texheight1 = inst.wad.images.W_GetTextureHeight(inst.conf, sourceSidedef->MidTex());
		int texheight2 = inst.wad.images.W_GetTextureHeight(inst.conf, nextSidedef->MidTex());

		if (midheight1 == midheight2 && midheight1 <= std::min(texheight1, texheight2)
			&& lowestceil1 == lowestceil2 && highestfloor1 == highestfloor2)
		{
			return true;
		}

		auto translatedOffset = [](const LineDef &line, const SideDef &side, int midheight,
								   int texheight)
		{
			if(line.flags & MLF_LowerUnpegged)
				return texheight - midheight + side.y_offset;
			return side.y_offset;
		};

		if (texheight1 == texheight2 && texheight1 <= std::min(midheight1, midheight2)
			&& translatedOffset(source, *sourceSidedef, midheight1, texheight1) ==
			   translatedOffset(next, *nextSidedef, midheight2, texheight2))
		{
			return true;
		}
		return false;
	}
	return true;
}

void Instance::SelectNeighborLines_height(int objnum, byte parts)
{
	if(!level.isLinedef(objnum) || !(parts & (PART_RT_ALL | PART_LF_ALL)))
		return;

	auto vertLineMap = makeVertexLineMap(level);

	const auto &source = level.linedefs[objnum];
	struct Entry
	{
		const LineDef *line;
		byte parts;
	};
	// NOTE: the meaning of the parts is stricly thus:
	// - lower, upper: floor or ceiling height on that side of the line
	// - rail: apply the opening OR texture size.
	// The parts assigned here may not match the user selection parts, so processing may need to be
	// done.

	Entry start = {source.get(), parts};
	if(source->OneSided() && start.parts & PART_RT_LOWER)
		start.parts |= PART_RT_UPPER;

	std::queue<Entry> queue;
	queue.push(start);

	// Also select the current line
	edit.Selected->set_ext(objnum, edit.Selected->get_ext(objnum) | parts);

	while(!queue.empty())
	{
		Entry entry = queue.front();
		queue.pop();

		for(int vertNum : {entry.line->start, entry.line->end})
		{
			for(int neigh : vertLineMap[vertNum])
			{
				const auto &otherLine = level.linedefs[neigh];
				if(otherLine.get() == entry.line)
					continue;
				bool flipped = otherLine->start == entry.line->start ||
							   otherLine->end == entry.line->end;
				struct Iteration
				{
					byte parts;
					Side side;
				};
				for(auto iteration : {Iteration{PART_RT_ALL, Side::right},
									  Iteration{PART_LF_ALL, Side::left}})
				{
					if(!(entry.parts & iteration.parts))
						continue;
					// TODO
				}
			}
			// TODO
		}
		// TODO
	}
}

void Instance::SelectNeighborLines(int objnum, SelectNeighborCriterion option, byte parts,
								   bool forward)
{
	const auto &line1 = level.linedefs[objnum];
	bool frontside = parts < PART_LF_LOWER;

	for (int i = 0; (long unsigned int)i < level.linedefs.size(); i++)
	{
		if (objnum == i || edit.Selected->get(i))
			continue;

		const auto &line2 = level.linedefs[i];

		if (line1->OneSided() != line2->OneSided())
			continue;

		if ((forward && line2->start == line1->end) || (!forward && line2->end == line1->start))
		{
			const SideDef *side1 = frontside ? level.getRight(*line1) : level.getLeft(*line1);
			const SideDef *side2 = frontside ? level.getRight(*line2) : level.getLeft(*line2);

			bool match = false;

			if (option == SelectNeighborCriterion::texture)
			{
				if (line1->OneSided() || (parts & PART_RT_RAIL || parts & PART_LF_RAIL))
					match = (side2->MidTex() == side1->MidTex() && side2->MidTex() != "-");

				else if (parts & PART_RT_LOWER || parts & PART_LF_LOWER)
					match = (side2->LowerTex() == side1->LowerTex() && side2->LowerTex() != "-");

				else if (parts & PART_RT_UPPER || parts & PART_LF_UPPER)
					match = (side2->UpperTex() == side1->UpperTex() && side2->UpperTex() != "-");

			}
			else
			{
				const Sector &l1front = level.getSector(*level.getRight(*line1));
				const Sector &l2front = level.getSector(*level.getRight(*line2));
				const Sector *l1back = NULL, *l2back = NULL;

				if (!line1->OneSided())
				{
					l1back = &level.getSector(*level.getLeft(*line1));
					l2back = &level.getSector(*level.getLeft(*line2));
				}
				if (line1->OneSided())
					match = (l1front.floorh == l2front.floorh && l1front.ceilh == l2front.ceilh);

				else if (parts & PART_RT_LOWER || parts & PART_LF_LOWER)
					match = (l1front.floorh == l2front.floorh && l1back->floorh == l2back->floorh);

				else if (parts & PART_RT_UPPER || parts & PART_LF_UPPER)
					match = (l1front.ceilh == l2front.ceilh && l1back->ceilh == l2back->ceilh);

				else
				{
					int lowestceil1 = std::min(l1front.ceilh, l1back->ceilh);
					int highestfloor1 = std::max(l1front.floorh, l1back->floorh);
					int midheight1 = lowestceil1 - highestfloor1;

					int lowestceil2 = std::min(l2front.ceilh, l2back->ceilh);
					int highestfloor2 = std::max(l2front.floorh, l2back->floorh);
					int midheight2 = lowestceil2 - highestfloor2;

					int texheight1 = wad.images.W_GetTextureHeight(conf, side1->MidTex());
					int texheight2 = wad.images.W_GetTextureHeight(conf, side2->MidTex());

					if (midheight1 == midheight2 && midheight1 < std::min(texheight1, texheight2)
						&& lowestceil1 == lowestceil2 && highestfloor1 == highestfloor2)
					{
						match = true;
					}
					else if (texheight1 == texheight2 && texheight1 < std::min(midheight1, midheight2)
						&& (side1->x_offset == side2->x_offset && side1->y_offset == side2->y_offset))
					{
						match = true;
					}
				}
			}

			if (match)
			{
				edit.Selected->set_ext(i, parts);
				SelectNeighborLines(i, option, parts, true);
				SelectNeighborLines(i, option, parts, false);
			}
		}
	}
}

void Instance::SelectNeighborSectors(int objnum, SString option, byte parts)
{
	const auto &sector1 = level.sectors[objnum];

	for (int i = 0; (long unsigned int)i < level.linedefs.size(); i++)
	{
		const auto &line = level.linedefs[i];

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

			if (option == "texture")
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
				SelectNeighborLines_texture(num, parts);
			else
			{
				SelectNeighborLines(num, criterion, parts, true);
				SelectNeighborLines(num, criterion, parts, false);
			}
		}
		else
		{
			SelectNeighborSectors(num, option, parts);
		}

	}
	RedrawMap();
}

