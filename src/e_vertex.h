//------------------------------------------------------------------------
//  VERTEX STUFF
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

#ifndef __EUREKA_E_VERTEX_H__
#define __EUREKA_E_VERTEX_H__

#include "DocumentModule.h"

struct vert_along_t;

class VertexModule : public DocumentModule
{
	friend class Instance;
public:
	VertexModule(Document &doc) : DocumentModule(doc)
	{
	}

	int findExact(fixcoord_t fx, fixcoord_t fy) const;
	int findDragOther(int v_num) const;
	int howManyLinedefs(int v_num) const;
	void mergeList(EditOperation &op, selection_c *list) const;
	bool tryFixDangler(int v_num) const;

private:
	void mergeSandwichLines(EditOperation &op, int ld1, int ld2, int v, selection_c &del_lines) const;
	void doMergeVertex(EditOperation &op, int v1, int v2, selection_c &del_lines) const;
	void calcDisconnectCoord(const LineDef *L, int v_num, double *x, double *y) const;
	void doDisconnectVertex(EditOperation &op, int v_num, int num_lines) const;
	void doDisconnectLinedef(EditOperation &op, int ld, int which_vert, bool *seen_one) const;
	void verticesOfDetachableSectors(selection_c &verts) const;
	void DETSEC_SeparateLine(EditOperation &op, int ld_num, int start2, int end2, Side in_side) const;
	void DETSEC_CalcMoveVector(selection_c *detach_verts, double *dx, double *dy) const;
	double evaluateCircle(EditOperation *op, double mid_x, double mid_y, double r,
		std::vector< vert_along_t > &along_list,
		unsigned int start_idx, double arc_rad,
		double ang_offset /* radians */,
		bool move_vertices = false) const;
};

#endif  /* __EUREKA_E_VERTEX_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
