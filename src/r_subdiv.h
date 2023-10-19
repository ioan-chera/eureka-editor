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
	void Clear();

	void AddPolygon(float lx1, float lx2, float low_y,
					float hx1, float hx2, float high_y);
};

sector_subdivision_c *Subdiv_PolygonsForSector(Instance &inst, int num);


class extrafloor_c
{
public:
	int ld = -1;     // linedef in the dummy sector
	int sd = -1;     // first sidedef of that line
	int flags = 0;  // bitmask of EXFL_XXX values
};


// vavoom style, dummy sector has floorh > ceilh
#define EXFL_VAVOOM		(1 << 0)
// only draw a single surface (the ceiling of dummy sector)
#define EXFL_TOP		(1 << 1)
// only draw a single surface (the floor of dummy sector)
#define EXFL_BOTTOM		(1 << 2)
// side texture is from upper on sidedef (not middle tex)
#define EXFL_UPPER		(1 << 3)
// side texture is from lower on sidedef (not middle tex)
#define EXFL_LOWER		(1 << 4)
// the 3D floor is translucent
#define EXFL_TRANSLUC	(1 << 5)


class slope_plane_c
{
public:
	bool sloped = false;  // if false, no slope is active

	float xm = 0, ym = 0, zadd = 0;

public:
	void Init(float height);
	void Copy(const slope_plane_c& other);

	inline double SlopeZ(double x, double y) const
	{
		return (x * xm) + (y * ym) + zadd;
	}

	void GetNormal(double& nx, double& ny, double& nz) const;
};


class sector_3dfloors_c
{
public:
	// this is -1 or a sector number of a BOOM 242 dummy sector
	int heightsec = -1;

	std::vector< extrafloor_c > floors;

	slope_plane_c f_plane;
	slope_plane_c c_plane;

public:
	void Clear();

	inline double FloorZ(double x, double y) const
	{
		return f_plane.SlopeZ(x, y);
	}

	inline double CeilZ(double x, double y) const
	{
		return c_plane.SlopeZ(x, y);
	}

	inline double PlaneZ(int znormal, double x, double y) const
	{
		if (znormal > 0)
			return f_plane.SlopeZ(x, y);
		else
			return c_plane.SlopeZ(x, y);
	}
};


struct sector_extra_info_t
{
	// these are < 0 when the sector has no lines
	int first_line;
	int last_line;

	// these are random junk when sector has no lines
	double bound_x1, bound_x2;
	double bound_y1, bound_y2;

	sector_subdivision_c sub;

	sector_3dfloors_c floors;

	// true when polygons have been built for this sector.
	bool built;

	void Clear()
	{
		first_line = last_line = -1;

		bound_x1 = 32767;
		bound_y1 = 32767;
		bound_x2 = -32767;
		bound_y2 = -32767;

		sub.Clear();
		floors.Clear();

		built = false;
	}

	void AddLine(int n)
	{
		if (first_line < 0 || first_line > n)
			first_line = n;

		if (last_line < n)
			last_line = n;
	}

	void AddVertex(const Vertex *V);
};

//
// Sector info cache
//
class sector_info_cache_c
{
public:
	int total = -1;
	std::vector<sector_extra_info_t> infos;
	
public:
	explicit sector_info_cache_c(const Instance &inst) : inst(inst)
	{ }

public:
	void Update();
	
private:
	void Rebuild();
	void CheckBoom242(const LineDef *L);
	void CheckExtraFloor(const LineDef *L, int ld_num);
	void CheckLineSlope(const LineDef *L);
	void CheckPlaneCopy(const LineDef *L);
	void CheckSlopeThing(const Thing *T);
	void CheckSlopeCopyThing(const Thing *T);
	void PlaneAlign(const LineDef *L, int floor_mode, int ceil_mode);
	void PlaneAlignPart(const LineDef *L, Side side, int plane);
	void PlaneCopy(const LineDef *L, int f1_tag, int c1_tag, int f2_tag, int c2_tag, int share);
	void PlaneCopyFromThing(const Thing *T, int plane);
	void PlaneTiltByThing(const Thing *T, int plane);
	void SlopeFromLine(slope_plane_c& pl, double x1, double y1, double z1,
					   double x2, double y2, double z2);

	const Instance &inst;
};

#endif  /* __EUREKA_R_SUBDIV_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
