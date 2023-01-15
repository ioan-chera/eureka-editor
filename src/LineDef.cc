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
#include "Vertex.h"

SideDef * LineDef::Right(const Document &doc) const
{
	return (right >= 0) ? doc.sidedefs[right].get() : nullptr;
}

SideDef * LineDef::Left(const Document &doc) const
{
	return (left >= 0) ? doc.sidedefs[left].get() : nullptr;
}

bool LineDef::TouchesCoord(FFixedPoint tx, FFixedPoint ty, const Document &doc) const
{
	return doc.getStart(*this).Matches(tx, ty) || doc.getEnd(*this).Matches(tx, ty);
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
	return (doc.getStart(*this).raw_y == doc.getEnd(*this).raw_y);
}

bool LineDef::IsVertical(const Document &doc) const
{
	return (doc.getStart(*this).raw_x == doc.getEnd(*this).raw_x);
}

double LineDef::CalcLength(const Document &doc) const
{
	double dx = doc.getStart(*this).x() - doc.getEnd(*this).x();
	double dy = doc.getStart(*this).y() - doc.getEnd(*this).y();

	return hypot(dx, dy);
}

bool LineDef::IsZeroLength(const Document &doc) const
{
	return (doc.getStart(*this).raw_x == doc.getEnd(*this).raw_x) && (doc.getStart(*this).raw_y == doc.getEnd(*this).raw_y);
}
