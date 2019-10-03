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

	// these are map coordinates
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
};


void Subdiv_InvalidateAll();


sector_subdivision_c *Subdiv_PolygonsForSector(int num);


#endif  /* __EUREKA_R_SUBDIV_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
