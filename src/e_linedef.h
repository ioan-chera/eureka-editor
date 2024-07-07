//------------------------------------------------------------------------
//  LINEDEF OPERATIONS
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

#ifndef __EUREKA_E_LINEDEF_H__
#define __EUREKA_E_LINEDEF_H__

#include "DocumentModule.h"

class selection_c;

namespace linemod
{
void moveCoordOntoLinedef(const Document &doc, int ld, v2double_t &v);
}

class LinedefModule : public DocumentModule
{
	friend class Instance;
public:
	enum class Part
	{
		unspecified,
		upper,
		rail,
		lower
	};

	LinedefModule(Document &doc) : DocumentModule(doc)
	{
	}

	void flipLinedef(EditOperation &op, int ld) const;
	void flipLinedefGroup(EditOperation &op, const selection_c *flip) const;

	void setLinedefsLength(int new_len) const;

	bool linedefAlreadyExists(int v1, int v2) const;

	int splitLinedefAtVertex(EditOperation &op, int ld, int v_idx) const;

	void addSecondSidedef(EditOperation &op, int ld, int new_sd, int other_sd) const;
	void removeSidedef(EditOperation &op, int ld, Side ld_side) const;
	void fixForLostSide(EditOperation &op, int ld) const;

	double angleBetweenLines(int A, int B, int C) const;

private:


	void flipLine_verts(EditOperation &op, int ld) const;
	void flipLine_sides(EditOperation &op, int ld) const;
	void flipLinedef_safe(EditOperation &op, int ld) const;
	int pickLinedefToExtend(selection_c& list, bool moving_start) const;
	bool linedefEndWillBeMoved(int ld, selection_c &list) const;
	bool linedefStartWillBeMoved(int ld, selection_c &list) const;
	void linedefSetLength(EditOperation &op, int ld, int new_len, double angle) const;

	bool alignOffsets(EditOperation &op, const Objid& obj, int align_flags) const;
	void alignGroup(EditOperation &op, const std::vector<Objid> & group, int align_flags) const;
	void doClearOfs(EditOperation &op, const Objid& cur, int align_flags) const;
	void doAlignX(EditOperation &op, const Objid &cur, const Objid &adj, int align_flags) const;
	void doAlignY(EditOperation &op, const Objid& cur, const Objid& adj, int align_flags) const;

	inline const LineDef *pointer(const Objid &obj) const;
	inline const SideDef *sidedefPointer(const Objid &obj) const;
	void determineAdjoiner(Objid& result,
						   const Objid& cur, int align_flags) const;
	int scoreAdjoiner(const Objid &adj, const Objid &cur, int align_flags) const;
	int scoreTextureMatch(const Objid &adj, const Objid &cur) const;
	void partCalcExtent(const Objid &obj, Part part, int *z1, int *z2) const;
	bool partIsVisible(const Objid& obj, Part part) const;

	int calcReferenceH(const Objid& obj) const;

	bool alignCheckAdjacent(const std::vector<Objid> & group,
							int j, int k, bool do_right) const;
	int alignPickNextSurface(const std::vector<Objid> & group,
							  const std::vector<byte>& seen, bool do_right) const;

	bool doSplitLineDef(EditOperation &op, int ld) const;

	void mergedSecondSidedef(EditOperation &op, int ld) const;
};

SString LD_RatioName(FFixedPoint idx, FFixedPoint idy, bool number_only);

enum linedef_align_flag_e
{
	LINALIGN_X		= (1 << 0),		// align the X offset
	LINALIGN_Y		= (1 << 1),		// align the Y offset
	LINALIGN_Clear	= (1 << 2),		// clear the offset(s), instead of aligning
	LINALIGN_Unpeg	= (1 << 3),		// change the unpegging flags
	LINALIGN_Right	= (1 << 4)		// align with sidedef on RIGHT of this one [ otherwise do LEFT ]
};

#endif  /* __EUREKA_E_LINEDEF_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
