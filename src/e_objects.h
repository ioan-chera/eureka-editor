//------------------------------------------------------------------------
//  OBJECT STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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

#ifndef __EUREKA_OBJECTS_H__
#define __EUREKA_OBJECTS_H__

#include "DocumentModule.h"

class EditOperation;
class selection_c;
struct ConfigData;

struct transform_t
{
public:
	double mid_x, mid_y;
	double scale_x, scale_y;
	double skew_x, skew_y;

	double rotate;	// radians

public:
	void Clear();

	void Apply(double *x, double *y) const;
};

//
// e_objects module
//
class ObjectsModule : public DocumentModule
{
	friend class Instance;
public:
	explicit ObjectsModule(Document &doc) : DocumentModule(doc)
	{
	}
	void move(selection_c *list, double delta_x, double delta_y, double delta_z) const;
	void singleDrag(const Objid &obj, double delta_x, double delta_y, double delta_z) const;
	void del(EditOperation &op, selection_c *list) const;
	bool lineTouchesBox(int ld, double x0, double y0, double x1, double y1) const;
	void getDragFocus(double *x, double *y, double ptr_x, double ptr_y) const;
	void calcMiddle(selection_c *list, double *x, double *y) const;
	void calcBBox(selection_c *list, double *x1, double *y1, double *x2, double *y2) const;
	void transform(transform_t &param) const;
	void scale3(double scale_x, double scale_y, double pos_x, double pos_y) const;
	void scale4(double scale_x, double scale_y, double scale_z,
		double pos_x, double pos_y, double pos_z) const;
	void rotate3(double deg, double pos_x, double pos_y) const;


private:
	void insertSector() const;
	void createSquare(EditOperation &op, int model) const;
	void insertThing() const;
	void insertVertex(bool force_continue, bool no_fill) const;
	void insertLinedefAutosplit(EditOperation &op, int v1, int v2, bool no_fill) const;
	void insertLinedef(EditOperation &op, int v1, int v2, bool no_fill) const;
	bool checkClosedLoop(EditOperation &op, int new_ld, int v1, int v2, selection_c *flip) const;
	int sectorNew(EditOperation &op, int model, int model2, int model3) const;
	void doMoveObjects(selection_c *list, double delta_x, double delta_y, double delta_z) const;
	void transferThingProperties(int src_thing, int dest_thing) const;
	void transferSectorProperties(int src_sec, int dest_sec) const;
	void transferLinedefProperties(int src_line, int dest_line, bool do_tex) const;
	void dragCountOnGrid(int *count, int *total) const;
	void dragCountOnGridWorker(ObjType obj_type, int objnum, int *count, int *total) const;
	void dragUpdateCurrentDist(ObjType obj_type, int objnum, double *x, double *y,
		double *best_dist, double ptr_x, double ptr_y,
		bool only_grid) const;
	void doMirrorThings(selection_c *list, bool is_vert, double mid_x, double mid_y) const;
	void doMirrorStuff(selection_c *list, bool is_vert, double mid_x, double mid_y) const;
	void doMirrorVertices(selection_c *list, bool is_vert, double mid_x, double mid_y) const;
	void doRotate90Things(selection_c *list, bool anti_clockwise,
		double mid_x, double mid_y) const;
	void doEnlargeOrShrink(bool do_shrink) const;
	void doScaleTwoThings(selection_c *list, transform_t &param) const;
	void doScaleTwoStuff(selection_c *list, transform_t &param) const;
	void doScaleTwoVertices(selection_c *list, transform_t &param) const;
	void determineOrigin(transform_t &param, double pos_x, double pos_y) const;
	void doScaleSectorHeights(selection_c *list, double scale_z, int pos_z) const;
	void quantizeThings(selection_c *list) const;
	void quantizeVertices(selection_c *list) const;
	bool spotInUse(ObjType obj_type, int x, int y) const;

	int findLineBetweenLineAndVertex(int lineID, int vertID) const;
	void splitLinedefAndMergeSandwich(EditOperation &op, int splitLineID, int vertID, double delta_x, double delta_y) const;
};




enum transform_keyword_e
{
	TRANS_K_Scale = 0,		// scale and keep aspect
	TRANS_K_Stretch,		// scale X and Y independently
	TRANS_K_Rotate,			// rotate
	TRANS_K_RotScale,		// rotate and scale at same time
	TRANS_K_Skew			// skew (shear) along an axis
};

//
// Information gathered from a linedef with a special
//
struct SpecialTagInfo
{
    ObjType type;
    int objnum;

    int tags[5];
    int numtags = 0;
    int hitags = 0;

    int tids[5];
    int numtids = 0;

    int lineids[5];
    int numlineids = 0;

    int selflineid = 0;

    int po[5];
    int numpo = 0;
};

bool getSpecialTagInfo(ObjType objtype, int objnum, int special, const void *obj,
                       const ConfigData &config, SpecialTagInfo &info);

#endif  /* __EUREKA_OBJECTS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
