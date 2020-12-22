//------------------------------------------------------------------------
//
//  AJ-BSP  Copyright (C) 2000-2019  Andrew Apted, et al
//          Copyright (C) 1994-1998  Colin Reed
//          Copyright (C) 1997-1998  Lee Killough
//
//  Originally based on the program 'BSP', version 2.3.
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

#include "Errors.hpp"
#include "Document.h"
#include "main.h"
#include "bsp.h"

#include "w_rawdef.h"


namespace ajbsp
{

#define DEBUG_ENABLED    0
#define DEBUG_WALLTIPS   0
#define DEBUG_POLYOBJ    0


#define SYS_MSG_BUFLEN  4000

void PrintDetail(const char *fmt, ...)
{
	(void) fmt;
}


void Failure(const char *fmt, ...)
{
	va_list args;


	va_start(args, fmt);
	SString message = SString::vprintf(fmt, args);
	va_end(args);

	if (cur_info->warnings)
		GB_PrintMsg("Failure: %s", message.c_str());

	cur_info->total_warnings++;

#if DEBUG_ENABLED
	DebugPrintf("Failure: %s", message.c_str());
#endif
}


void Warning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	SString message = SString::vprintf(fmt, args);
	va_end(args);

	if (cur_info->warnings)
		GB_PrintMsg("Warning: %s", message.c_str());

	cur_info->total_warnings++;

#if DEBUG_ENABLED
	DebugPrintf("Warning: %s", message.c_str());
#endif
}


//------------------------------------------------------------------------
// UTILITY : general purpose functions
//------------------------------------------------------------------------

#ifndef WIN32
#include <time.h>
#endif


//
// Allocate memory with error checking.  Zeros the memory.
//
void *UtilCalloc(int size)
{
	void *ret = calloc(1, size);

	if (!ret)
		FatalError("Out of memory (cannot allocate %d bytes)\n", size);

	return ret;
}


//
// Reallocate memory with error checking.
//
void *UtilRealloc(void *old, int size)
{
	void *ret = realloc(old, size);

	if (!ret)
		FatalError("Out of memory (cannot reallocate %d bytes)\n", size);

	return ret;
}


//
// Free the memory with error checking.
//
void UtilFree(void *data)
{
	if (data == NULL)
		BugError("Trying to free a NULL pointer\n");

	free(data);
}


//
// Translate (dx, dy) into an angle value (degrees)
//
angle_g UtilComputeAngle(double dx, double dy)
{
	double angle;

	if (dx == 0)
		return (dy > 0) ? 90.0 : 270.0;

	angle = atan2((double) dy, (double) dx) * 180.0 / M_PI;

	if (angle < 0)
		angle += 360.0;

	return angle;
}


SString UtilTimeString(void)
{
#ifdef WIN32

	SYSTEMTIME sys_time;

	GetSystemTime(&sys_time);

	return SString::printf("%04d-%02d-%02d %02d:%02d:%02d.%04d",
			sys_time.wYear, sys_time.wMonth, sys_time.wDay,
			sys_time.wHour, sys_time.wMinute, sys_time.wSecond,
			sys_time.wMilliseconds * 10);

#else // LINUX or MACOSX

	time_t epoch_time;
	struct tm *calend_time;

	if (time(&epoch_time) == (time_t)-1)
		return NULL;

	calend_time = localtime(&epoch_time);
	if (! calend_time)
		return NULL;

	return SString::printf("%04d-%02d-%02d %02d:%02d:%02d.%04d",
			calend_time->tm_year + 1900, calend_time->tm_mon + 1,
			calend_time->tm_mday,
			calend_time->tm_hour, calend_time->tm_min,
			calend_time->tm_sec,  0);
#endif
}


//------------------------------------------------------------------------
//  Adler-32 CHECKSUM Code
//------------------------------------------------------------------------

void Adler32_Begin(u32_t *crc)
{
	*crc = 1;
}

void Adler32_AddBlock(u32_t *crc, const u8_t *data, int length)
{
	u32_t s1 = (*crc) & 0xFFFF;
	u32_t s2 = ((*crc) >> 16) & 0xFFFF;

	for ( ; length > 0 ; data++, length--)
	{
		s1 = (s1 + *data) % 65521;
		s2 = (s2 + s1)    % 65521;
	}

	*crc = (s2 << 16) | s1;
}

void Adler32_Finish(u32_t *crc)
{
	/* nothing to do */
}


//------------------------------------------------------------------------
// ANALYZE : Analyzing level structures
//------------------------------------------------------------------------


#define POLY_BOX_SZ  10


/* ----- polyobj handling ----------------------------- */

static void MarkPolyobjSector(int sector)
{
	int i;

	if (! is_sector(sector))
		return;

# if DEBUG_POLYOBJ
	DebugPrintf("  Marking SECTOR %d\n", sector);
# endif

	for (i = 0 ; i < NumLineDefs ; i++)
	{
		LineDef *L = gDocument.linedefs[i];

		if ((L->right >= 0 && L->Right(gDocument)->sector == sector) ||
			(L->left  >= 0 && L->Left(gDocument)->sector  == sector))
		{
			L->flags |= MLF_IS_PRECIOUS;
		}
	}
}

static void MarkPolyobjPoint(double x, double y)
{
	int i;
	int inside_count = 0;

	double best_dist = 999999;
	int best_match = -1;
	int sector = -1;

	double x1, y1;
	double x2, y2;

	double EPSILON = 0.01;

	// -AJA- First we handle the "awkward" cases where the polyobj sits
	//       directly on a linedef or even a vertex.  We check all lines
	//       that intersect a small box around the spawn point.

	int bminx = (int) (x - POLY_BOX_SZ);
	int bminy = (int) (y - POLY_BOX_SZ);
	int bmaxx = (int) (x + POLY_BOX_SZ);
	int bmaxy = (int) (y + POLY_BOX_SZ);

	for (i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef *L = gDocument.linedefs[i];

		if (CheckLinedefInsideBox(bminx, bminy, bmaxx, bmaxy,
					(int) L->Start(gDocument)->x(), (int) L->Start(gDocument)->y(),
					(int) L->End(gDocument)->x(),   (int) L->End(gDocument)->y()))
		{
#     if DEBUG_POLYOBJ
			DebugPrintf("  Touching line was %d\n", L->index);
#     endif

			if (L->left >= 0)
				MarkPolyobjSector(L->Left(gDocument)->sector);

			if (L->right >= 0)
				MarkPolyobjSector(L->Right(gDocument)->sector);

			inside_count++;
		}
	}

	if (inside_count > 0)
		return;

	// -AJA- Algorithm is just like in DEU: we cast a line horizontally
	//       from the given (x,y) position and find all linedefs that
	//       intersect it, choosing the one with the closest distance.
	//       If the point is sitting directly on a (two-sided) line,
	//       then we mark the sectors on both sides.

	for (i = 0 ; i < NumLineDefs ; i++)
	{
		LineDef *L = gDocument.linedefs[i];

		double x_cut;

		x1 = L->Start(gDocument)->x();
		y1 = L->Start(gDocument)->y();
		x2 = L->End(gDocument)->x();
		y2 = L->End(gDocument)->y();

		/* check vertical range */
		if (fabs(y2 - y1) < EPSILON)
			continue;

		if ((y > (y1 + EPSILON) && y > (y2 + EPSILON)) ||
			(y < (y1 - EPSILON) && y < (y2 - EPSILON)))
			continue;

		x_cut = x1 + (x2 - x1) * (y - y1) / (y2 - y1) - x;

		if (fabs(x_cut) < fabs(best_dist))
		{
			/* found a closer linedef */

			best_match = i;
			best_dist = x_cut;
		}
	}

	if (best_match < 0)
	{
		Warning("Bad polyobj thing at (%1.0f,%1.0f).\n", x, y);
		return;
	}

	const LineDef *best_ld = gDocument.linedefs[best_match];

	y1 = best_ld->Start(gDocument)->y();
	y2 = best_ld->End(gDocument)->y();

# if DEBUG_POLYOBJ
	DebugPrintf("  Closest line was %d Y=%1.0f..%1.0f (dist=%1.1f)\n",
			best_match, y1, y2, best_dist);
# endif

	/* sanity check: shouldn't be directly on the line */
# if DEBUG_POLYOBJ
	if (fabs(best_dist) < EPSILON)
	{
		DebugPrintf("  Polyobj FAILURE: directly on the line (%d)\n", best_match);
	}
# endif

	/* check orientation of line, to determine which side the polyobj is
	 * actually on.
	 */
	if ((y1 > y2) == (best_dist > 0))
		sector = (best_ld->right >= 0) ? best_ld->Right(gDocument)->sector : -1;
	else
		sector = (best_ld->left >= 0) ? best_ld->Left(gDocument)->sector : -1;

# if DEBUG_POLYOBJ
	DebugPrintf("  Sector %d contains the polyobj.\n", sector);
# endif

	if (sector < 0)
	{
		Warning("Invalid Polyobj thing at (%1.0f,%1.0f).\n", x, y);
		return;
	}

	MarkPolyobjSector(sector);
}


//
// Based on code courtesy of Janis Legzdinsh.
//
void DetectPolyobjSectors(void)
{
	int i;

	// -JL- There's a conflict between Hexen polyobj thing types and Doom thing
	//      types. In Doom type 3001 is for Imp and 3002 for Demon. To solve
	//      this problem, first we are going through all lines to see if the
	//      level has any polyobjs. If found, we also must detect what polyobj
	//      thing types are used - Hexen ones or ZDoom ones. That's why we
	//      are going through all things searching for ZDoom polyobj thing
	//      types. If any found, we assume that ZDoom polyobj thing types are
	//      used, otherwise Hexen polyobj thing types are used.

	// -JL- First go through all lines to see if level contains any polyobjs
	for (i = 0 ; i < NumLineDefs ; i++)
	{
		const LineDef *L = gDocument.linedefs[i];

		if (L->type == HEXTYPE_POLY_START || L->type == HEXTYPE_POLY_EXPLICIT)
			break;
	}

	if (i == NumLineDefs)
	{
		// -JL- No polyobjs in this level
		return;
	}

	// -JL- Detect what polyobj thing types are used - Hexen ones or ZDoom ones
	bool hexen_style = true;

	for (i = 0 ; i < NumThings ; i++)
	{
		const Thing *T = gDocument.things[i];

		if (T->type == ZDOOM_PO_SPAWN_TYPE || T->type == ZDOOM_PO_SPAWNCRUSH_TYPE)
		{
			// -JL- A ZDoom style polyobj thing found
			hexen_style = false;
			break;
		}
	}

# if DEBUG_POLYOBJ
	DebugPrintf("Using %s style polyobj things\n",
			hexen_style ? "HEXEN" : "ZDOOM");
# endif

	for (i = 0 ; i < NumThings ; i++)
	{
		const Thing *T = gDocument.things[i];

		double x = T->x();
		double y = T->y();

		// ignore everything except polyobj start spots
		if (hexen_style)
		{
			// -JL- Hexen style polyobj things
			if (T->type != PO_SPAWN_TYPE && T->type != PO_SPAWNCRUSH_TYPE)
				continue;
		}
		else
		{
			// -JL- ZDoom style polyobj things
			if (T->type != ZDOOM_PO_SPAWN_TYPE && T->type != ZDOOM_PO_SPAWNCRUSH_TYPE)
				continue;
		}

#   if DEBUG_POLYOBJ
		DebugPrintf("Thing %d at (%1.0f,%1.0f) is a polyobj spawner.\n", i, x, y);
#   endif

		MarkPolyobjPoint(x, y);
	}
}


/* ----- analysis routines ----------------------------- */

static int VertexCompare(const void *p1, const void *p2)
{
	int vert1 = static_cast<const u16_t *>(p1)[0];
	int vert2 = static_cast<const u16_t *>(p2)[0];

	if (vert1 == vert2)
		return 0;

	const Vertex *A = gDocument.vertices[vert1];
	const Vertex *B = gDocument.vertices[vert2];

	if (A->raw_x != B->raw_x)
		return A->raw_x - B->raw_x;

	return A->raw_y - B->raw_y;
}


void DetectOverlappingVertices(void)
{
	SYS_ASSERT(num_vertices == NumVertices);

	u16_t *array = new u16_t[num_vertices];

	// sort array of indices
	int i;
	for (i=0 ; i < num_vertices ; i++)
		array[i] = i;

	qsort(array, num_vertices, sizeof(u16_t), VertexCompare);

	// now mark them off
	for (i=0 ; i < num_vertices - 1 ; i++)
	{
		if (VertexCompare(array + i, array + i + 1) == 0)
		{
			// found an overlap!

			vertex_t *A = lev_vertices[array[i]];
			vertex_t *B = lev_vertices[array[i+1]];

			B->overlap = A->overlap ? A->overlap : A;
		}
	}

	delete[] array;
}


static inline int LineVertexLowest(const LineDef *L)
{
	// returns the "lowest" vertex (normally the left-most, but if the
	// line is vertical, then the bottom-most) => 0 for start, 1 for end.

	return ( L->Start(gDocument)->raw_x <  L->End(gDocument)->raw_x ||
			(L->Start(gDocument)->raw_x == L->End(gDocument)->raw_x &&
			 L->Start(gDocument)->raw_y <  L->End(gDocument)->raw_y)) ? 0 : 1;
}

static int LineStartCompare(const void *p1, const void *p2)
{
	int line1 = ((const int *) p1)[0];
	int line2 = ((const int *) p2)[0];

	if (line1 == line2)
		return 0;

	const LineDef *A = gDocument.linedefs[line1];
	const LineDef *B = gDocument.linedefs[line2];

	// determine left-most vertex of each line
	const Vertex *C = LineVertexLowest(A) ? A->End(gDocument) : A->Start(gDocument);
	const Vertex *D = LineVertexLowest(B) ? B->End(gDocument) : B->Start(gDocument);

	if (C->raw_x != D->raw_x)
		return C->raw_x - D->raw_x;

	return C->raw_y - D->raw_y;
}

static int LineEndCompare(const void *p1, const void *p2)
{
	int line1 = ((const int *) p1)[0];
	int line2 = ((const int *) p2)[0];

	if (line1 == line2)
		return 0;

	const LineDef *A = gDocument.linedefs[line1];
	const LineDef *B = gDocument.linedefs[line2];

	// determine right-most vertex of each line
	const Vertex * C = LineVertexLowest(A) ? A->Start(gDocument) : A->End(gDocument);
	const Vertex * D = LineVertexLowest(B) ? B->Start(gDocument) : B->End(gDocument);

	if (C->raw_x != D->raw_x)
		return C->raw_x - D->raw_x;

	return C->raw_y - D->raw_y;
}


void DetectOverlappingLines(void)
{
	// Algorithm:
	//   Sort all lines by left-most vertex.
	//   Overlapping lines will then be near each other in this set.
	//   Note: does not detect partially overlapping lines.

	int i;
	int *array = (int *)UtilCalloc(NumLineDefs * sizeof(int));
	int count = 0;

	// sort array of indices
	for (i=0 ; i < NumLineDefs ; i++)
		array[i] = i;

	qsort(array, NumLineDefs, sizeof(int), LineStartCompare);

	for (i=0 ; i < NumLineDefs - 1 ; i++)
	{
		int j;

		for (j = i+1 ; j < NumLineDefs ; j++)
		{
			if (LineStartCompare(array + i, array + j) != 0)
				break;

			if (LineEndCompare(array + i, array + j) == 0)
			{
				// found an overlap !

				LineDef *L = gDocument.linedefs[array[j]];
				L->flags |= MLF_IS_OVERLAP;
				count++;
			}
		}
	}

	if (count > 0)
	{
		PrintDetail("Detected %d overlapped linedefs\n", count);
	}

	UtilFree(array);
}


/* ----- vertex routines ------------------------------- */

// smallest degrees between two angles before being considered equal
#define ANG_EPSILON  (1.0 / 1024.0)

static void VertexAddWallTip(vertex_t *vert, double dx, double dy,
		int open_left, int open_right)
{
	if (vert->overlap)
		vert = vert->overlap;

	walltip_t *tip = NewWallTip();
	walltip_t *after;

	tip->angle = UtilComputeAngle(dx, dy);
	tip->open_left  = open_left;
	tip->open_right = open_right;

	// find the correct place (order is increasing angle)
	for (after=vert->tip_set ; after && after->next ; after=after->next)
	{ }

	while (after && tip->angle + ANG_EPSILON < after->angle)
		after = after->prev;

	// link it in
	tip->next = after ? after->next : vert->tip_set;
	tip->prev = after;

	if (after)
	{
		if (after->next)
			after->next->prev = tip;

		after->next = tip;
	}
	else
	{
		if (vert->tip_set)
			vert->tip_set->prev = tip;

		vert->tip_set = tip;
	}
}


void CalculateWallTips()
{
	int i;

	for (i=0 ; i < NumLineDefs ; i++)
	{
		const LineDef *L = gDocument.linedefs[i];

		if ((L->flags & MLF_IS_OVERLAP) || L->IsZeroLength(gDocument))
			continue;

		double x1 = L->Start(gDocument)->x();
		double y1 = L->Start(gDocument)->y();
		double x2 = L->End(gDocument)->x();
		double y2 = L->End(gDocument)->y();

		bool left  = (L->left  >= 0) && is_sector(L->Left(gDocument)->sector);
		bool right = (L->right >= 0) && is_sector(L->Right(gDocument)->sector);

		VertexAddWallTip(lev_vertices[L->start], x2-x1, y2-y1, left, right);
		VertexAddWallTip(lev_vertices[L->end],   x1-x2, y1-y2, right, left);
	}

# if DEBUG_WALLTIPS
	for (i=0 ; i < num_vertices ; i++)
	{
		vertex_t *V = lev_vertices[i];

		DebugPrintf("WallTips for vertex %d:\n", i);

		for (walltip_t *tip = V->tip_set ; tip ; tip = tip->next)
		{
			DebugPrintf("  Angle=%1.1f left=%d right=%d\n", tip->angle,
					tip->open_left ? 1 : 0, tip->open_right ? 1 : 0);
		}
	}
# endif
}


vertex_t *NewVertexFromSplitSeg(seg_t *seg, double x, double y)
{
	vertex_t *vert = NewVertex();

	vert->x = x;
	vert->y = y;
	vert->is_new = true;

	vert->index = num_new_vert;
	num_new_vert++;

	// compute wall-tip info
	if (seg->linedef < 0 || gDocument.linedefs[seg->linedef]->TwoSided())
	{
		VertexAddWallTip(vert, -seg->pdx, -seg->pdy, true, true);
		VertexAddWallTip(vert,  seg->pdx,  seg->pdy, true, true);
	}
	else
	{
		const LineDef *L = gDocument.linedefs[seg->linedef];

		bool front_open = ((seg->side ? L->left : L->right) >= 0);

		VertexAddWallTip(vert, -seg->pdx, -seg->pdy, front_open, !front_open);
		VertexAddWallTip(vert,  seg->pdx,  seg->pdy, !front_open, front_open);
	}

	return vert;
}


vertex_t *NewVertexDegenerate(vertex_t *start, vertex_t *end)
{
	// this is only called when rounding off the BSP tree and
	// all the segs are degenerate (zero length), hence we need
	// to create at least one seg which won't be zero length.

	double dx = end->x - start->x;
	double dy = end->y - start->y;

	double dlen = hypot(dx, dy);

	vertex_t *vert = NewVertex();

	vert->is_new = false;

	vert->index = num_old_vert;
	num_old_vert++;

	// compute new coordinates

	vert->x = start->x;
	vert->y = start->x;

	if (dlen == 0)
		BugError("NewVertexDegenerate: bad delta!\n");

	dx /= dlen;
	dy /= dlen;

	while (I_ROUND(vert->x) == I_ROUND(start->x) &&
		   I_ROUND(vert->y) == I_ROUND(start->y))
	{
		vert->x += dx;
		vert->y += dy;
	}

	return vert;
}


bool VertexCheckOpen(vertex_t *vert, double dx, double dy)
{
	if (vert->overlap)
		vert = vert->overlap;

	walltip_t *tip;

	angle_g angle = UtilComputeAngle(dx, dy);

	// first check whether there's a wall-tip that lies in the exact
	// direction of the given direction (which is relative to the
	// vertex).

	for (tip=vert->tip_set ; tip ; tip=tip->next)
	{
		if (fabs(tip->angle - angle) < ANG_EPSILON ||
			fabs(tip->angle - angle) > (360.0 - ANG_EPSILON))
		{
			// found one, hence closed
			return false;
		}
	}

	// OK, now just find the first wall-tip whose angle is greater than
	// the angle we're interested in.  Therefore we'll be on the RIGHT
	// side of that wall-tip.

	for (tip=vert->tip_set ; tip ; tip=tip->next)
	{
		if (angle + ANG_EPSILON < tip->angle)
		{
			// found it
			return tip->open_right;
		}

		if (! tip->next)
		{
			// no more tips, thus we must be on the LEFT side of the tip
			// with the largest angle.

			return tip->open_left;
		}
	}

	// usually won't get here
	return true;
}


}  // namespace ajbsp

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
