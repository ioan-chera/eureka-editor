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

#include "LineDef.h"

#include "Document.h"
#include "Errors.h"
#include "SideDef.h"

Vertex * LineDef::Start(const Document &doc) const
{
	return doc.vertices[start];
}

Vertex * LineDef::End(const Document &doc) const
{
	return doc.vertices[end];
}

SideDef * LineDef::Right(const Document &doc) const
{
	return (right >= 0) ? doc.sidedefs[right] : nullptr;
}

SideDef * LineDef::Left(const Document &doc) const
{
	return (left >= 0) ? doc.sidedefs[left] : nullptr;
}

bool LineDef::TouchesCoord(fixcoord_t tx, fixcoord_t ty, const Document &doc) const
{
	return Start(doc)->Matches(tx, ty) || End(doc)->Matches(tx, ty);
}

bool LineDef::TouchesSector(int sec_num, const Document &doc) const
{
	if (right >= 0 && doc.sidedefs[right]->sector == sec_num)
		return true;

	if (left >= 0 && doc.sidedefs[left]->sector == sec_num)
		return true;

	return false;
}


int LineDef::WhatSector(Side side, const Document &doc) const
{
	switch (side)
	{
		case Side::left:
			return Left(doc) ? Left(doc)->sector : -1;

		case Side::right:
			return Right(doc) ? Right(doc)->sector : -1;

		default:
			BugError("bad side : %d\n", (int)side);
			return -1;
	}
}


int LineDef::WhatSideDef(Side side) const
{
	switch (side)
	{
		case Side::left:
			return left;
		case Side::right:
			return right;

		default:
			BugError("bad side : %d\n", (int)side);
			return -1;
	}
}

bool LineDef::IsSelfRef(const Document &doc) const
{
	return (left >= 0) && (right >= 0) &&
		doc.sidedefs[left]->sector == doc.sidedefs[right]->sector;
}

bool LineDef::IsHorizontal(const Document &doc) const
{
	return (Start(doc)->raw_y == End(doc)->raw_y);
}

bool LineDef::IsVertical(const Document &doc) const
{
	return (Start(doc)->raw_x == End(doc)->raw_x);
}

double LineDef::CalcLength(const Document &doc) const
{
	double dx = Start(doc)->x() - End(doc)->x();
	double dy = Start(doc)->y() - End(doc)->y();

	return hypot(dx, dy);
}

bool LineDef::IsZeroLength(const Document &doc) const
{
	return (Start(doc)->raw_x == End(doc)->raw_x) && (Start(doc)->raw_y == End(doc)->raw_y);
}
