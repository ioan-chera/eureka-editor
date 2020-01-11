//------------------------------------------------------------------------
//  SECTOR SUBDIVISION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2008-2019 Andrew Apted
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

#ifndef __EUREKA_R_SUBDIV_H__
#define __EUREKA_R_SUBDIV_H__


struct sector_polygon_t
{
	// number of sides, either 3 or 4
	int count;

	// the map coordinates.
	//
	// NOTE: this shape is actually a trapezoid where the top and
	//       bottom are always purely horizontal, only the sides
	//       can be sloped.  the software renderer relies on this.
	float mx[4];
	float my[4];
};


class sector_subdivision_c
{
public:
	std::vector<sector_polygon_t> polygons;

public:
	sector_subdivision_c();
	~sector_subdivision_c();

	void Clear();

	void AddPolygon(float lx1, float lx2, float low_y,
					float hx1, float hx2, float high_y);
};


void Subdiv_InvalidateAll();


bool Subdiv_SectorOnScreen(int num, double map_lx, double map_ly, double map_hx, double map_hy);

sector_subdivision_c *Subdiv_PolygonsForSector(int num);


class sector_3dfloors_c
{
public:
	// this is -1 or a sector number of a BOOM 242 dummy sector
	int heightsec;

	// TODO 3D floor list

public:
	sector_3dfloors_c();
	~sector_3dfloors_c();

	void Clear();
};

sector_3dfloors_c *Subdiv_3DFloorsForSector(int num);


#endif  /* __EUREKA_R_SUBDIV_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
