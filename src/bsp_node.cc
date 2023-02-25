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


//
// To be able to divide the nodes down, this routine must decide which
// is the best Seg to use as a nodeline. It does this by selecting the
// line with least splits and has least difference of Segs on either
// side of it.
//
// Credit to Raphael Quinet and DEU, this routine is a copy of the
// nodeline picker used in DEU5beta. I am using this method because
// the method I originally used was not so good.
//
// Rewritten by Lee Killough to significantly improve performance,
// while not affecting results one bit in >99% of cases (some tiny
// differences due to roundoff error may occur, but they are
// insignificant).
//
// Rewritten again by Andrew Apted (-AJA-), 1999-2000.
//

#include "Errors.h"
#include "Instance.h"
#include "LineDef.h"
#include "SideDef.h"
#include "Vertex.h"
#include "bsp.h"

#include "w_rawdef.h"


namespace ajbsp
{

#define PRECIOUS_MULTIPLY  100

#define SEG_FAST_THRESHHOLD  200


#define DEBUG_BUILDER  0
#define DEBUG_SORTER   0
#define DEBUG_SUBSEC   0

#define DEBUG_PICKNODE  0
#define DEBUG_SPLIT     0
#define DEBUG_CUTLIST   0


// smallest distance between two points before being considered equal
#define DIST_EPSILON  (1.0 / 1024.0)


static int current_seg_index;


typedef struct eval_info_s
{
	int cost;
	int splits;
	int iffy;
	int near_miss;

	int real_left;
	int real_right;
	int mini_left;
	int mini_right;

public:
	void BumpLeft(int linedef)
	{
		if (linedef >= 0)
			real_left++;
		else
			mini_left++;
	}

	void BumpRight(int linedef)
	{
		if (linedef >= 0)
			real_right++;
		else
			mini_right++;
	}
}
eval_info_t;


static intersection_t *quick_alloc_cuts = NULL;


static intersection_t *NewIntersection(void)
{
	intersection_t *cut;

	if (quick_alloc_cuts)
	{
		cut = quick_alloc_cuts;
		quick_alloc_cuts = cut->next;
	}
	else
	{
		cut = (intersection_t *) UtilCalloc(sizeof(intersection_t));
	}

	return cut;
}


void FreeQuickAllocCuts(void)
{
	while (quick_alloc_cuts)
	{
		intersection_t *cut = quick_alloc_cuts;
		quick_alloc_cuts = cut->next;

		UtilFree(cut);
	}
}


//
// Fill in the fields 'angle', 'len', 'pdx', 'pdy', etc...
//
void seg_t::Recompute()
{
	psx = start->x;
	psy = start->y;
	pex = end->x;
	pey = end->y;
	pdx = pex - psx;
	pdy = pey - psy;

	p_length = hypot(pdx, pdy);

	if (p_length <= 0)
		BugError("Seg %p has zero p_length.\n", this);

	p_perp =  psy * pdx - psx * pdy;
	p_para = -psx * pdx - psy * pdy;
}


//
// -AJA- Splits the given seg at the point (x,y).  The new seg is
//       returned.  The old seg is shortened (the original start
//       vertex is unchanged), whereas the new seg becomes the cut-off
//       tail (keeping the original end vertex).
//
//       If the seg has a partner, than that partner is also split.
//       NOTE WELL: the new piece of the partner seg is inserted into
//       the same list as the partner seg (and after it) -- thus ALL
//       segs (except the one we are currently splitting) must exist
//       on a singly-linked list somewhere.
//
static seg_t * SplitSeg(seg_t *old_seg, double x, double y, const Document &doc)
{
	seg_t *new_seg;
	vertex_t *new_vert;

# if DEBUG_SPLIT
	if (old_seg->linedef >= 0)
		gLog.debugPrintf("Splitting Linedef %d (%p) at (%1.1f,%1.1f)\n",
				old_seg->linedef, old_seg, x, y);
	else
		gLog.debugPrintf("Splitting Miniseg %p at (%1.1f,%1.1f)\n", old_seg, x, y);
# endif

	new_vert = NewVertexFromSplitSeg(old_seg, x, y, doc);
	new_seg  = NewSeg();

	// copy seg info
	new_seg[0] = old_seg[0];
	new_seg->next = NULL;

	old_seg->end   = new_vert;
	new_seg->start = new_vert;

	old_seg->Recompute();
	new_seg->Recompute();

# if DEBUG_SPLIT
	gLog.debugPrintf("Splitting Vertex is %04X at (%1.1f,%1.1f)\n",
			new_vert->index, new_vert->x, new_vert->y);
# endif

	// handle partners

	if (old_seg->partner)
	{
#   if DEBUG_SPLIT
		gLog.debugPrintf("Splitting Partner %p\n", old_seg->partner);
#   endif

		new_seg->partner = NewSeg();

		// copy seg info
		// [ including the "next" field ]
		new_seg->partner[0] = old_seg->partner[0];

		// IMPORTANT: keep partner relationship valid.
		new_seg->partner->partner = new_seg;

		old_seg->partner->start = new_vert;
		new_seg->partner->end   = new_vert;

		old_seg->partner->Recompute();
		new_seg->partner->Recompute();

		// link it into list
		old_seg->partner->next = new_seg->partner;
	}

	return new_seg;
}


//
// -AJA- In the quest for slime-trail annihilation :->, this routine
//       calculates the intersection location between the current seg
//       and the partitioning seg, and takes advantage of some common
//       situations like horizontal/vertical lines.
//
static inline void ComputeIntersection(seg_t *seg, seg_t *part,
		double perp_c, double perp_d, double *x, double *y)
{
	double ds;

	// horizontal partition against vertical seg
	if (part->pdy == 0 && seg->pdx == 0)
	{
		*x = seg->psx;
		*y = part->psy;
		return;
	}

	// vertical partition against horizontal seg
	if (part->pdx == 0 && seg->pdy == 0)
	{
		*x = part->psx;
		*y = seg->psy;
		return;
	}

	// 0 = start, 1 = end
	ds = perp_c / (perp_c - perp_d);

	if (seg->pdx == 0)
		*x = seg->psx;
	else
		*x = seg->psx + (seg->pdx * ds);

	if (seg->pdy == 0)
		*y = seg->psy;
	else
		*y = seg->psy + (seg->pdy * ds);
}


static void AddIntersection(intersection_t ** cut_list,
		vertex_t *vert, seg_t *part, bool self_ref)
{
	bool open_before = VertexCheckOpen(vert, -part->pdx, -part->pdy);
	bool open_after  = VertexCheckOpen(vert,  part->pdx,  part->pdy);

	double along_dist = part->ParallelDist(vert->x, vert->y);

	intersection_t *cut;
	intersection_t *after;

	/* merge with any existing vertex? */
	for (cut=(*cut_list) ; cut ; cut=cut->next)
	{
		if (vert == cut->vertex)
		{
			return;
		}
		else if (fabs(along_dist - cut->along_dist) < DIST_EPSILON*2)
		{
			// an OPEN aspect always overrides a CLOSED one.
			// [ though a mismatch should only occur with broken geometry ]
			if (open_before) cut->open_before = true;
			if (open_after)  cut->open_after  = true;

			return;
		}
	}

	/* create new intersection */
	cut = NewIntersection();

	cut->vertex = vert;
	cut->along_dist = along_dist;
	cut->self_ref = self_ref;
	cut->open_before = open_before;
	cut->open_after  = open_after;

	/* insert the new intersection into the list */

	for (after=(*cut_list) ; after && after->next ; after=after->next)
	{ }

	while (after && cut->along_dist < after->along_dist)
		after = after->prev;

	/* link it in */
	cut->next = after ? after->next : (*cut_list);
	cut->prev = after;

	if (after)
	{
		if (after->next)
			after->next->prev = cut;

		after->next = cut;
	}
	else
	{
		if (*cut_list)
			(*cut_list)->prev = cut;

		(*cut_list) = cut;
	}
}


//
// Returns true if a "bad seg" was found early.
//
static int EvalPartitionWorker(quadtree_c *tree, seg_t *part,
		int best_cost, eval_info_t *info, const Document &doc)
{
	double qnty;
	double a, b, fa, fb;

	int factor = cur_info->factor;

	// -AJA- this is the heart of the superblock idea, it tests the
	//       *whole* quad against the partition line to quickly handle
	//       all the segs within it at once.  Only when the partition
	//       line intercepts the box do we need to go deeper into it.

	switch (tree->OnLineSide(part))
	{
	case Side::left:
		info->real_left += tree->real_num;
		info->mini_left += tree->mini_num;

		return false;

	case Side::right:
		info->real_right += tree->real_num;
		info->mini_right += tree->mini_num;

		return false;
	default:
		break;
	}

	/* check partition against all Segs */

	seg_t *check;

	for (check=tree->list ; check ; check=check->next)
	{
		// This is the heart of my pruning idea - it catches
		// bad segs early on. Killough

		if (info->cost > best_cost)
			return true;

		/* get state of lines' relation to each other */
		if (check->source_line == part->source_line)
		{
			a = b = fa = fb = 0;
		}
		else
		{
			a = part->PerpDist(check->psx, check->psy);
			b = part->PerpDist(check->pex, check->pey);

			fa = fabs(a);
			fb = fabs(b);
		}

		/* check for being on the same line */
		if (fa <= DIST_EPSILON && fb <= DIST_EPSILON)
		{
			// this seg runs along the same line as the partition.  Check
			// whether it goes in the same direction or the opposite.

			if (check->pdx*part->pdx + check->pdy*part->pdy < 0)
			{
				info->BumpLeft(check->linedef);
			}
			else
			{
				info->BumpRight(check->linedef);
			}
			continue;
		}

		// -AJA- check for passing through a vertex.  Normally this is fine
		//       (even ideal), but the vertex could on a sector that we
		//       DONT want to split, and the normal linedef-based checks
		//       may fail to detect the sector being cut in half.  Thanks
		//       to Janis Legzdinsh for spotting this obscure bug.

		if (fa <= DIST_EPSILON || fb <= DIST_EPSILON)
		{
			if (check->linedef >= 0 && (doc.getLinedef(check->linedef).flags & MLF_IS_PRECIOUS))
				info->cost += 40 * factor * PRECIOUS_MULTIPLY;
		}

		/* check for right side */
		if (a > -DIST_EPSILON && b > -DIST_EPSILON)
		{
			info->BumpRight(check->linedef);

			/* check for a near miss */
			if ((a >= IFFY_LEN && b >= IFFY_LEN) ||
				(a <= DIST_EPSILON && b >= IFFY_LEN) ||
				(b <= DIST_EPSILON && a >= IFFY_LEN))
			{
				continue;
			}

			info->near_miss++;

			// -AJA- near misses are bad, since they have the potential to
			//       cause really short minisegs to be created in future
			//       processing.  Thus the closer the near miss, the higher
			//       the cost.

			if (a <= DIST_EPSILON || b <= DIST_EPSILON)
				qnty = IFFY_LEN / std::max(a, b);
			else
				qnty = IFFY_LEN / std::min(a, b);

			info->cost += (int) (100 * factor * (qnty * qnty - 1.0));
			continue;
		}

		/* check for left side */
		if (a < DIST_EPSILON && b < DIST_EPSILON)
		{
			info->BumpLeft(check->linedef);

			/* check for a near miss */
			if ((a <= -IFFY_LEN && b <= -IFFY_LEN) ||
				(a >= -DIST_EPSILON && b <= -IFFY_LEN) ||
				(b >= -DIST_EPSILON && a <= -IFFY_LEN))
			{
				continue;
			}

			info->near_miss++;

			// the closer the miss, the higher the cost (see note above)
			if (a >= -DIST_EPSILON || b >= -DIST_EPSILON)
				qnty = IFFY_LEN / -std::min(a, b);
			else
				qnty = IFFY_LEN / -std::max(a, b);

			info->cost += (int) (70 * factor * (qnty * qnty - 1.0));
			continue;
		}

		// When we reach here, we have a and b non-zero and opposite sign,
		// hence this seg will be split by the partition line.

		info->splits++;

		// If the linedef associated with this seg has a tag >= 900, treat
		// it as precious; i.e. don't split it unless all other options
		// are exhausted.  This is used to protect deep water and invisible
		// lifts/stairs from being messed up accidentally by splits.

		if (check->linedef >= 0 && (doc.getLinedef(check->linedef).flags & MLF_IS_PRECIOUS))
			info->cost += 100 * factor * PRECIOUS_MULTIPLY;
		else
			info->cost += 100 * factor;

		// -AJA- check if the split point is very close to one end, which
		//       an undesirable situation (producing really short segs).
		//       This is perhaps _one_ source of those darn slime trails.
		//       Hence the name "IFFY segs", and a rather hefty surcharge.

		if (fa < IFFY_LEN || fb < IFFY_LEN)
		{
			info->iffy++;

			// the closer to the end, the higher the cost
			qnty = IFFY_LEN / std::min(fa, fb);
			info->cost += (int) (140 * factor * (qnty * qnty - 1.0));
		}
	}

	/* handle sub-blocks recursively */

	for (int c=0 ; c < 2 ; c++)
	{
		if (tree->subs[c] && !tree->subs[c]->Empty())
		{
			if (EvalPartitionWorker(tree->subs[c], part, best_cost, info, doc))
				return true;
		}
	}

	/* no "bad seg" was found */
	return false;
}

//
// -AJA- Evaluate a partition seg & determine the cost, taking into
//       account the number of splits, difference between left &
//       right, and linedefs that are tagged 'precious'.
//
// Returns the computed cost, or a negative value if the seg should be
// skipped altogether.
//
static int EvalPartition(quadtree_c *tree, seg_t *part, int best_cost, const Document &doc)
{
	eval_info_t info;

	/* initialise info structure */
	info.cost   = 0;
	info.splits = 0;
	info.iffy   = 0;
	info.near_miss  = 0;

	info.real_left  = 0;
	info.real_right = 0;
	info.mini_left  = 0;
	info.mini_right = 0;

	if (EvalPartitionWorker(tree, part, best_cost, &info, doc))
		return -1;

	/* make sure there is at least one real seg on each side */
	if (info.real_left == 0 || info.real_right == 0)
	{
#   if DEBUG_PICKNODE
		gLog.debugPrintf("Eval : No real segs on %s%sside\n",
				info.real_left  ? "" : "left ",
				info.real_right ? "" : "right ");
#   endif

		return -1;
	}

	/* increase cost by the difference between left & right */
	info.cost += 100 * abs(info.real_left - info.real_right);

	// -AJA- allow miniseg counts to affect the outcome, but to a
	//       lesser degree than real segs.

	info.cost += 50 * abs(info.mini_left - info.mini_right);

	// -AJA- Another little twist, here we show a slight preference for
	//       partition lines that lie either purely horizontally or
	//       purely vertically.

	if (part->pdx != 0 && part->pdy != 0)
		info.cost += 25;

# if DEBUG_PICKNODE
	gLog.debugPrintf("Eval %p: splits=%d iffy=%d near=%d left=%d+%d right=%d+%d "
			"cost=%d.%02d\n", part, info.splits, info.iffy, info.near_miss,
			info.real_left, info.mini_left, info.real_right, info.mini_right,
			info.cost / 100, info.cost % 100);
# endif

	return info.cost;
}


static void EvaluateFastWorker(quadtree_c *tree,
		seg_t **best_H, seg_t **best_V, int mid_x, int mid_y)
{
	seg_t *part;

	for (part=tree->list ; part ; part = part->next)
	{
		/* ignore minisegs as partition candidates */
		if (part->linedef < 0)
			continue;

		if (part->pdy == 0)
		{
			// horizontal seg
			if (! *best_H)
			{
				*best_H = part;
			}
			else
			{
				int old_dist = abs((int)(*best_H)->psy - mid_y);
				int new_dist = abs((int)(   part)->psy - mid_y);

				if (new_dist < old_dist)
					*best_H = part;
			}
		}
		else if (part->pdx == 0)
		{
			// vertical seg
			if (! *best_V)
			{
				*best_V = part;
			}
			else
			{
				int old_dist = abs((int)(*best_V)->psx - mid_x);
				int new_dist = abs((int)(   part)->psx - mid_x);

				if (new_dist < old_dist)
					*best_V = part;
			}
		}
	}

	/* handle sub-blocks recursively */

	for (int c=0 ; c < 2 ; c++)
	{
		if (tree->subs[c] && !tree->subs[c]->Empty())
		{
			EvaluateFastWorker(tree->subs[c], best_H, best_V, mid_x, mid_y);
		}
	}
}


static seg_t *FindFastSeg(quadtree_c *tree, const Document &doc)
{
	seg_t *best_H = NULL;
	seg_t *best_V = NULL;

	int mid_x = (tree->x1 + tree->x2) / 2;
	int mid_y = (tree->y1 + tree->y2) / 2;

	EvaluateFastWorker(tree, &best_H, &best_V, mid_x, mid_y);

	int H_cost = -1;
	int V_cost = -1;

	if (best_H)
		H_cost = EvalPartition(tree, best_H, 99999999, doc);

	if (best_V)
		V_cost = EvalPartition(tree, best_V, 99999999, doc);

# if DEBUG_PICKNODE
	gLog.debugPrintf("FindFastSeg: best_H=%p (cost %d) | best_V=%p (cost %d)\n",
			best_H, H_cost, best_V, V_cost);
# endif

	if (H_cost < 0 && V_cost < 0)
		return NULL;

	if (H_cost < 0) return best_V;
	if (V_cost < 0) return best_H;

	return (V_cost < H_cost) ? best_V : best_H;
}


/* returns false if cancelled */
static bool PickNodeWorker(quadtree_c *part_list,
		quadtree_c *tree, seg_t ** best, int *best_cost, const Document &doc)
{
	// try each partition
	for (seg_t *part=part_list->list ; part ; part = part->next)
	{
		if (cur_info->cancelled)
			return false;

#   if DEBUG_PICKNODE
		gLog.debugPrintf("PickNode:   %sSEG %p  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				part->linedef >= 0 ? "" : "MINI", part,
				part->start->x, part->start->y, part->end->x, part->end->y);
#   endif

		/* ignore minisegs as partition candidates */
		if (part->linedef < 0)
			continue;

		int cost = EvalPartition(tree, part, *best_cost, doc);

		/* seg unsuitable or too costly ? */
		if (cost < 0 || cost >= *best_cost)
			continue;

		/* we have a new better choice */
		(*best_cost) = cost;

		/* remember which Seg */
		(*best) = part;
	}

	/* recursively handle sub-blocks */

	for (int c=0 ; c < 2 ; c++)
	{
		if (part_list->subs[c] && !part_list->subs[c]->Empty())
		{
			PickNodeWorker(part_list->subs[c], tree, best, best_cost, doc);
		}
	}

	return true;
}


//
// Find the best seg in the seg_list to use as a partition line.
//
static seg_t *PickNode(quadtree_c *tree, int depth, const Document &doc)
{
	seg_t *best=NULL;

	int best_cost=INT_MAX;

# if DEBUG_PICKNODE
	gLog.debugPrintf("PickNode: BEGUN (depth %d)\n", depth);
# endif

	/* -AJA- here is the logic for "fast mode".  We look for segs which
	 *       are axis-aligned and roughly divide the current group into
	 *       two halves.  This can save *heaps* of times on large levels.
	 */
	if (cur_info->fast && tree->real_num >= SEG_FAST_THRESHHOLD)
	{
#   if DEBUG_PICKNODE
		gLog.debugPrintf("PickNode: Looking for Fast node...\n");
#   endif

		best = FindFastSeg(tree, doc);

		if (best)
		{
#     if DEBUG_PICKNODE
			gLog.debugPrintf("PickNode: Using Fast node (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
					best->start->x, best->start->y, best->end->x, best->end->y);
#     endif

			return best;
		}
	}

	if (! PickNodeWorker(tree, tree, &best, &best_cost, doc))
	{
		/* hack here : BuildNodes will detect the cancellation */
		return NULL;
	}

# if DEBUG_PICKNODE
	if (! best)
	{
		gLog.debugPrintf("PickNode: NO BEST FOUND !\n");
	}
	else
	{
		gLog.debugPrintf("PickNode: Best has score %d.%02d  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				best_cost / 100, best_cost % 100, best->start->x, best->start->y,
				best->end->x, best->end->y);
	}
# endif

	return best;
}


//
// Apply the partition line to the given seg, taking the necessary
// action (moving it into either the left list, right list, or
// splitting it).
//
// -AJA- I have rewritten this routine based on the EvalPartition
//       routine above (which I've also reworked, heavily).  I think
//       it is important that both these routines follow the exact
//       same logic when determining which segs should go left, right
//       or be split.
//
static void DivideOneSeg(seg_t *seg, seg_t *part,
		seg_t **left_list, seg_t **right_list,
		intersection_t ** cut_list, const Document &doc)
{
	seg_t *new_seg;

	double x, y;

	/* get state of lines' relation to each other */
	double a = part->PerpDist(seg->psx, seg->psy);
	double b = part->PerpDist(seg->pex, seg->pey);

	bool self_ref = (seg->linedef >= 0) ? doc.isSelfRef(doc.getLinedef(seg->linedef)) : false;

	if (seg->source_line == part->source_line)
		a = b = 0;

	/* check for being on the same line */
	if (fabs(a) <= DIST_EPSILON && fabs(b) <= DIST_EPSILON)
	{
		AddIntersection(cut_list, seg->start, part, self_ref);
		AddIntersection(cut_list, seg->end,   part, self_ref);

		// this seg runs along the same line as the partition.  check
		// whether it goes in the same direction or the opposite.

		if (seg->pdx*part->pdx + seg->pdy*part->pdy < 0)
		{
			ListAddSeg(left_list, seg);
		}
		else
		{
			ListAddSeg(right_list, seg);
		}

		return;
	}

	/* check for right side */
	if (a > -DIST_EPSILON && b > -DIST_EPSILON)
	{
		if (a < DIST_EPSILON)
			AddIntersection(cut_list, seg->start, part, self_ref);
		else if (b < DIST_EPSILON)
			AddIntersection(cut_list, seg->end, part, self_ref);

		ListAddSeg(right_list, seg);
		return;
	}

	/* check for left side */
	if (a < DIST_EPSILON && b < DIST_EPSILON)
	{
		if (a > -DIST_EPSILON)
			AddIntersection(cut_list, seg->start, part, self_ref);
		else if (b > -DIST_EPSILON)
			AddIntersection(cut_list, seg->end, part, self_ref);

		ListAddSeg(left_list, seg);
		return;
	}

	// when we reach here, we have a and b non-zero and opposite sign,
	// hence this seg will be split by the partition line.

	ComputeIntersection(seg, part, a, b, &x, &y);

	new_seg = SplitSeg(seg, x, y, doc);

	AddIntersection(cut_list, seg->end, part, self_ref);

	if (a < 0)
	{
		ListAddSeg(left_list,  seg);
		ListAddSeg(right_list, new_seg);
	}
	else
	{
		ListAddSeg(right_list, seg);
		ListAddSeg(left_list,  new_seg);
	}
}


static void SeparateSegs(quadtree_c *tree, seg_t *part,
		seg_t **left_list, seg_t **right_list,
		intersection_t ** cut_list, const Document &doc)
{
	while (tree->list != NULL)
	{
		seg_t *seg = tree->list;
		tree->list = seg->next;

		seg->quad = NULL;

		DivideOneSeg(seg, part, left_list, right_list, cut_list, doc);
	}

	// recursively handle sub-blocks
	if (tree->subs[0])
	{
		SeparateSegs(tree->subs[0], part, left_list, right_list, cut_list, doc);
		SeparateSegs(tree->subs[1], part, left_list, right_list, cut_list, doc);
	}

	// this quadtree_c is empty now
}


void FindLimits2(seg_t *list, bbox_t *bbox)
{
	// empty list?
	if (list == NULL)
	{
		bbox->minx = 0;
		bbox->miny = 0;
		bbox->maxx = 2;
		bbox->maxy = 2;
		return;
	}

	bbox->minx = bbox->miny = SHRT_MAX;
	bbox->maxx = bbox->maxy = SHRT_MIN;

	for ( ; list != NULL ; list = list->next)
	{
		double x1 = list->start->x;
		double y1 = list->start->y;
		double x2 = list->end->x;
		double y2 = list->end->y;

		int lx = (int) floor(std::min(x1, x2) - 0.2);
		int ly = (int) floor(std::min(y1, y2) - 0.2);
		int hx = (int)  ceil(std::max(x1, x2) + 0.2);
		int hy = (int)  ceil(std::max(y1, y2) + 0.2);

		if (lx < bbox->minx) bbox->minx = lx;
		if (ly < bbox->miny) bbox->miny = ly;
		if (hx > bbox->maxx) bbox->maxx = hx;
		if (hy > bbox->maxy) bbox->maxy = hy;
	}
}


void AddMinisegs(intersection_t *cut_list, seg_t *part,
		seg_t **left_list, seg_t **right_list)
{
	if (! cut_list)
		return;

	intersection_t *cur, *next;
	seg_t *seg, *buddy;

# if DEBUG_CUTLIST
	gLog.debugPrintf("CUT LIST:\n");
	gLog.debugPrintf("PARTITION: (%1.1f,%1.1f) += (%1.1f,%1.1f)\n",
			part->psx, part->psy, part->pdx, part->pdy);

	for (cur=cut_list ; cur ; cur=cur->next)
	{
		gLog.debugPrintf("  Vertex %8X (%1.1f,%1.1f)  Along %1.2f  [%d/%d]  %s\n",
				cur->vertex->index, cur->vertex->x, cur->vertex->y,
				cur->along_dist,
				cur->open_before ? 1 : 0,
				cur->open_after  ? 1 : 0,
				cur->self_ref ? "SELFREF" : "");
	}
# endif

	// find open gaps in the intersection list, convert to minisegs

	for (cur = cut_list ; cur && cur->next ; cur = cur->next)
	{
		next = cur->next;

		// sanity check
		double len = next->along_dist - cur->along_dist;
		if (len < -0.01)
			BugError("Bad order in intersect list: %1.3f > %1.3f\n",
					cur->along_dist, next->along_dist);

		bool A = cur->open_after;
		bool B = next->open_before;

		// nothing possible is both ends are CLOSED
		if (! (A || B))
			continue;

		if (A != B)
		{
			// a mismatch indicates something wrong with level geometry.
			// warning about it is probably not worth it, so ignore it.
			continue;
		}

		// righteo, here we have definite open space.
		// create a miniseg pair...

		seg = NewSeg();
		buddy = NewSeg();

		seg->partner = buddy;
		buddy->partner = seg;

		seg->start = cur->vertex;
		seg->end   = next->vertex;

		buddy->start = next->vertex;
		buddy->end   = cur->vertex;

		// leave 'side' as zero too (not needed for minisegs).

		seg->index = buddy->index = -1;
		seg->linedef = buddy->linedef = -1;
		seg->source_line = buddy->source_line = part->linedef;

		  seg->Recompute();
		buddy->Recompute();

		// add the new segs to the appropriate list
		ListAddSeg(right_list, seg);
		ListAddSeg(left_list, buddy);

#   if DEBUG_CUTLIST
		gLog.debugPrintf("AddMiniseg: %p RIGHT  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				seg, seg->start->x, seg->start->y, seg->end->x, seg->end->y);

		gLog.debugPrintf("AddMiniseg: %p LEFT   (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				buddy, buddy->start->x, buddy->start->y, buddy->end->x, buddy->end->y);
#   endif
	}

	// free intersection structures into quick-alloc list
	while (cut_list)
	{
		cur = cut_list;
		cut_list = cur->next;

		cur->next = quick_alloc_cuts;
		quick_alloc_cuts = cur;
	}
}


//------------------------------------------------------------------------
// NODE : Recursively create nodes and return the pointers.
//------------------------------------------------------------------------


//
// Split a list of segs into two using the method described at bottom
// of the file, this was taken from OBJECTS.C in the DEU5beta source.
//
// This is done by scanning all of the segs and finding the one that
// does the least splitting and has the least difference in numbers of
// segs on either side.
//
// If the ones on the left side make a SSector, then create another SSector
// else put the segs into lefts list.
// If the ones on the right side make a SSector, then create another SSector
// else put the segs into rights list.
//
// Rewritten by Andrew Apted (-AJA-), 1999-2000.
//


//
// Returns SIDE_LEFT, SIDE_RIGHT, or 0 for intersect.
//
Side seg_t::PointOnLineSide(double x, double y) const
{
	double perp = PerpDist(x, y);

	if (fabs(perp) <= DIST_EPSILON)
		return Side::neither;

	return (perp < 0) ? Side::left : Side::right;
}


Side quadtree_c::OnLineSide(const seg_t *part) const
{
	double tx1 = (double)x1 - IFFY_LEN;
	double ty1 = (double)y1 - IFFY_LEN;
	double tx2 = (double)x2 + IFFY_LEN;
	double ty2 = (double)y2 + IFFY_LEN;

	Side p1, p2;

	// handle simple cases (vertical & horizontal lines)
	if (part->pdx == 0)
	{
		p1 = (tx1 > part->psx) ? Side::right : Side::left;
		p2 = (tx2 > part->psx) ? Side::right : Side::left;

		if (part->pdy < 0)
		{
			p1 = -p1;
			p2 = -p2;
		}
	}
	else if (part->pdy == 0)
	{
		p1 = (ty1 < part->psy) ? Side::right : Side::left;
		p2 = (ty2 < part->psy) ? Side::right : Side::left;

		if (part->pdx < 0)
		{
			p1 = -p1;
			p2 = -p2;
		}
	}
	// now handle the cases of positive and negative slope
	else if (part->pdx * part->pdy > 0)
	{
		p1 = part->PointOnLineSide(tx1, ty2);
		p2 = part->PointOnLineSide(tx2, ty1);
	}
	else  // NEGATIVE
	{
		p1 = part->PointOnLineSide(tx1, ty1);
		p2 = part->PointOnLineSide(tx2, ty2);
	}

	// line goes through or touches the box?
	if (p1 != p2)
		return Side::neither;

	return p1;
}


#if 0  // DEBUG HELPER
void quadtree_c::VerifySide(seg_t *part, int side)
{
	for (seg_t *seg = list ; seg != NULL ; seg = seg->next)
	{
		Side p1 = part->PointOnLineSide(seg->psx, seg->psy);
		if (p1 != side) BugError("VerifySide failed.\n");

		Side p2 = part->PointOnLineSide(seg->pex, seg->pey);
		if (p2 != side) BugError("VerifySide failed.\n");
	}

	if (subs[0]) subs[0]->VerifySide(part, side);
	if (subs[1]) subs[1]->VerifySide(part, side);
}
#endif


void node_t::SetPartition(const seg_t *part, const Instance &inst)
{
	SYS_ASSERT(part->linedef >= 0);

	const auto &part_L = inst.level.getLinedef(part->linedef);

	if (part->side == 0)  /* right side */
	{
		x  = inst.level.getStart(part_L).x();
		y  = inst.level.getStart(part_L).y();
		dx = inst.level.getEnd(part_L).x() - x;
		dy = inst.level.getEnd(part_L).y() - y;
	}
	else  /* left side */
	{
		x  = inst.level.getEnd(part_L).x();
		y  = inst.level.getEnd(part_L).y();
		dx = inst.level.getStart(part_L).x() - x;
		dy = inst.level.getStart(part_L).y() - y;
	}

	/* check for very long partition (overflow of dx,dy in NODES) */

	if (fabs(dx) > 32000 || fabs(dy) > 32000)
	{
		if (inst.loaded.levelFormat == MapFormat::udmf)
		{
			// XGL3 nodes are 16.16 fixed point, hence we still need
			// to reduce the delta.
			dx *= 0.5;
			dy *= 0.5;
		}
		else
		{
			if (((int)dx | (int)dy) & 1)
			{
				Warning(inst, "Loss of accuracy on VERY long node: "
						"(%f,%f) -> (%f,%f)\n", x, y, x + dx, y+ dy);
			}

			dx = round(dx * 0.5);
			dy = round(dy * 0.5);
		}
	}
}


/* ----- quad-tree routines ------------------------------------ */

quadtree_c::quadtree_c(int _x1, int _y1, int _x2, int _y2) :
	x1(_x1), y1(_y1),
	x2(_x2), y2(_y2),
	real_num(0), mini_num(0),
	list(NULL)
{
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (dx <= 320 && dy <= 320)
	{
		// leaf node
		subs[0] = NULL;
		subs[1] = NULL;
	}
	else if (dx >= dy)
	{
		subs[0] = new quadtree_c(x1, y1, x1 + dx/2, y2);
		subs[1] = new quadtree_c(x1 + dx/2, y1, x2, y2);
	}
	else
	{
		subs[0] = new quadtree_c(x1, y1, x2, y1 + dy/2);
		subs[1] = new quadtree_c(x1, y1 + dy/2, x2, y2);
	}
}


quadtree_c::~quadtree_c()
{
	if (subs[0] != NULL) delete subs[0];
	if (subs[1] != NULL) delete subs[1];
}


void quadtree_c::AddSeg(seg_t *seg)
{
	// update seg counts
	if (seg->linedef >= 0)
		real_num++;
	else
		mini_num++;

	if (subs[0] != NULL)
	{
		double x_min = std::min(seg->start->x, seg->end->x);
		double y_min = std::min(seg->start->y, seg->end->y);

		double x_max = std::max(seg->start->x, seg->end->x);
		double y_max = std::max(seg->start->y, seg->end->y);

		if ((x2 - x1) >= (y2 - y1))
		{
			if (x_min > subs[1]->x1)
			{
				subs[1]->AddSeg(seg);
				return;
			}
			else if (x_max < subs[0]->x2)
			{
				subs[0]->AddSeg(seg);
				return;
			}
		}
		else
		{
			if (y_min > subs[1]->y1)
			{
				subs[1]->AddSeg(seg);
				return;
			}
			else if (y_max < subs[0]->y2)
			{
				subs[0]->AddSeg(seg);
				return;
			}
		}
	}

	// link into this node

	ListAddSeg(&list, seg);

	seg->quad = this;
}


void quadtree_c::AddList(seg_t *_list)
{
	while (_list != NULL)
	{
		seg_t *seg = _list;
		_list = seg->next;

		AddSeg(seg);
   }
}


void quadtree_c::ConvertToList(seg_t **_list)
{
	while (list != NULL)
	{
		seg_t *seg = list;
		list = seg->next;

		ListAddSeg(_list, seg);
	}

	if (subs[0] != NULL)
	{
		subs[0]->ConvertToList(_list);
		subs[1]->ConvertToList(_list);
	}

	// this quadtree is empty now
}


static seg_t *CreateOneSeg(int line, vertex_t *start, vertex_t *end,
		int sidedef, int what_side /* 0 or 1 */, const Instance &inst)
{
	const SideDef *sd = NULL;
	if (sidedef >= 0)
		sd = &inst.level.sidedefs[sidedef];

	// check for bad sidedef
	if (sd && !inst.level.isSector(sd->sector))
	{
		Warning(inst, "Bad sidedef on linedef #%d (Z_CheckHeap error)\n", line);
	}

	// handle overlapping vertices, pick a nominal one
	if (start->overlap) start = start->overlap;
	if (  end->overlap)   end =   end->overlap;

	seg_t *seg = NewSeg();

	seg->start   = start;
	seg->end     = end;
	seg->linedef = line;
	seg->side    = what_side;
	seg->partner = NULL;

	seg->source_line = seg->linedef;
	seg->index = -1;

	seg->Recompute();

	return seg;
}


//
// Initially create all segs, one for each linedef.
// Must be called *after* InitBlockmap().
//
seg_t *CreateSegs(const Instance &inst)
{
	seg_t *list = NULL;

	for (int i=0 ; i < inst.level.numLinedefs() ; i++)
	{
		const auto &line = inst.level.getLinedef(i);

		seg_t *left  = NULL;
		seg_t *right = NULL;

		// ignore zero-length lines
		if (inst.level.isZeroLength(line))
			continue;

		// ignore overlapping lines
		if (line.flags & MLF_IS_OVERLAP)
			continue;

		// check for extremely long lines
		if (inst.level.calcLength(line) >= 30000)
			Warning(inst, "Linedef #%d is VERY long, it may cause problems\n", i);

		if (line.right >= 0)
		{
			right = CreateOneSeg(i, lev_vertices[line.start], lev_vertices[line.end], line.right, 0, inst);

			ListAddSeg(&list, right);
		}
		else
		{
			Warning(inst, "Linedef #%d has no right sidedef!\n", i);
		}

		if (line.left >= 0)
		{
			left = CreateOneSeg(i, lev_vertices[line.end], lev_vertices[line.start], line.left, 1, inst);

			ListAddSeg(&list, left);

			if (right)
			{
				// -AJA- Partner segs.  These always maintain a one-to-one
				//       correspondence, so if one of the gets split, the
				//       other one must be split too.

				left->partner = right;
				right->partner = left;
			}
		}
		else
		{
			if (line.flags & MLF_TwoSided)
				Warning(inst, "Linedef #%d is 2s but has no left sidedef\n", i);
		}
	}

	return list;
}


static quadtree_c *TreeFromSegList(seg_t *list, const bbox_t *bounds)
{
	quadtree_c *tree = new quadtree_c(bounds->minx, bounds->miny, bounds->maxx, bounds->maxy);

	tree->AddList(list);

	return tree;
}


void subsec_t::DetermineMiddle()
{
	mid_x = 0.0;
	mid_y = 0.0;

	int total=0;

	// compute middle coordinates
	for (seg_t *seg=seg_list ; seg ; seg=seg->next)
	{
		mid_x += seg->start->x + seg->end->x;
		mid_y += seg->start->y + seg->end->y;

		total += 2;
	}

	if (total > 0)
	{
		mid_x /= (double)total;
		mid_y /= (double)total;
	}
}


void subsec_t::ClockwiseOrder(const Document &doc)
{
	seg_t *seg;

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: Clockwising %d\n", index);
# endif

	// count segs and create an array to manipulate them
	int total = 0;

	for (seg=seg_list ; seg ; seg=seg->next)
		total++;

	// use local array if large enough
	seg_t *seg_buffer[32];
	seg_t ** array = seg_buffer;

	if (total > 32)
		array = new seg_t* [total];

	int i = 0;
	for (seg=seg_list ; seg ; seg=seg->next)
	{
		array[i++] = seg;

		// compute angles now
		seg->cmp_angle = UtilComputeAngle(seg->start->x - mid_x, seg->start->y - mid_y);
	}

	if (i != total)
		BugError("ClockwiseOrder miscounted.\n");

	// sort segs by angle (from the middle point to the start vertex).
	// the desired order (clockwise) means descending angles.
	// since # of segs is usually small, a bubble sort is fast enough.

	i = 0;

	while (i+1 < total)
	{
		seg_t *A = array[i];
		seg_t *B = array[i+1];

		if (A->cmp_angle < B->cmp_angle)
		{
			// swap 'em
			array[i] = B;
			array[i+1] = A;

			// bubble down
			if (i > 0)
				i--;
		}
		else
		{
			// bubble up
			i++;
		}
	}

	// choose the seg that will be first (the game engine will typically use
	// that to determine the sector).  while we don't like self referencing
	// linedefs (they are often used for deep-water effects), we definitely
	// have to skip minisegs.
	int first = 0;
	int best_score = -1;

	for (i=0 ; i < total ; i++)
	{
		int cur_score = 3;

		// miniseg?
		if (array[i]->linedef < 0)
			cur_score = 0;
		else if (doc.isSelfRef(doc.getLinedef(array[i]->linedef)))
			cur_score = 2;

		if (cur_score > best_score)
		{
			first = i;
			best_score = cur_score;
		}
	}

	// transfer sorted array back into subsec_t
	seg_list = NULL;

	for (i=total-1 ; i >= 0 ; i--)
	{
		int k = (i + first) % total;

		ListAddSeg(&seg_list, array[k]);
	}

	// free seg array, unless local
	if (total > 32)
		delete[] array;

# if DEBUG_SORTER
	gLog.debugPrintf("Sorted SEGS around (%1.1f,%1.1f)\n", mid_x, mid_y);

	for (seg=seg_list ; seg ; seg=seg->next)
	{
		gLog.debugPrintf("  Seg %p: Angle %1.6f  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				seg, seg->cmp_angle, seg->start->x, seg->start->y, seg->end->x, seg->end->y);
	}
# endif
}


void subsec_t::SanityCheckClosed()
{
	seg_t *seg, *next;
	int total=0, gaps=0;

	for (seg=seg_list ; seg ; seg=seg->next)
	{
		next = seg->next ? seg->next : seg_list;

		if (seg->end->x != next->start->x || seg->end->y != next->start->y)
			gaps++;

		total++;
	}

	if (gaps > 0)
	{
		gLog.debugPrintf("Subsector #%d near (%1.1f,%1.1f) is not closed "
				"(%d gaps, %d segs)\n", index,
				mid_x, mid_y, gaps, total);

#   if DEBUG_SUBSEC
		for (seg=seg_list ; seg ; seg=seg->next)
		{
			gLog.debugPrintf("  SEG %p  (%1.1f,%1.1f) --> (%1.1f,%1.1f)\n", seg,
					seg->start->x, seg->start->y, seg->end->x, seg->end->y);
		}
#   endif
	}
}


void subsec_t::SanityCheckHasRealSeg()
{
	seg_t *seg;

	for (seg=seg_list ; seg ; seg=seg->next)
		if (seg->linedef >= 0)
			return;

	BugError("Subsector #%d near (%1.1f,%1.1f) has no real seg!\n",
			index, mid_x, mid_y);
}


void subsec_t::RenumberSegs()
{
	seg_t *seg;

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: Renumbering %d\n", index);
# endif

	seg_count = 0;

	for (seg=seg_list ; seg ; seg=seg->next)
	{
		seg->index = current_seg_index;
		current_seg_index++;

		seg_count++;

#   if DEBUG_SUBSEC
		gLog.debugPrintf("Subsec:   %d: Seg %p  Index %d\n", seg_count,
				seg, seg->index);
#   endif
	}
}


//
// Create a subsector from a list of segs.
//
static subsec_t *CreateSubsector(quadtree_c *tree)
{
	subsec_t *sub = NewSubsec();

	// compute subsector's index
	sub->index = num_subsecs - 1;

	// copy segs into subsector
	// [ assumes seg_list field is NULL ]
	tree->ConvertToList(&sub->seg_list);

	sub->DetermineMiddle();

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: Creating %d\n", sub->index);
# endif

	return sub;
}


int ComputeBspHeight(node_t *node)
{
	if (node)
	{
		int left, right;

		right = ComputeBspHeight(node->r.node);
		left  = ComputeBspHeight(node->l.node);

		return std::max(left, right) + 1;
	}

	return 1;
}


#if DEBUG_BUILDER
static void DebugShowSegs(seg_t *list)
{
	for ( ; list != NULL ; list = list->next)
	{
		seg_t *seg = list;

		gLog.debugPrintf("Build:   %sSEG %p  linedef=%d  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n",
				(seg->linedef >= 0) ? "" : "MINI", seg, seg->linedef,
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
	}
}
#endif


build_result_e BuildNodes(seg_t *list, bbox_t *bounds /* output */,
						  node_t ** N, subsec_t ** S, int depth, const Instance &inst)
{
	*N = NULL;
	*S = NULL;

	if (cur_info->cancelled)
		return BUILD_Cancelled;

# if DEBUG_BUILDER
	gLog.debugPrintf("Build: BEGUN @ %d\n", depth);
	DebugShowSegs(list);
# endif

	// determine bounds of segs
	FindLimits2(list, bounds);

	quadtree_c *tree = TreeFromSegList(list, bounds);


	/* pick partition line  None indicates convexicity */
	seg_t *part = PickNode(tree, depth, inst.level);

	if (part == NULL)
	{
#   if DEBUG_BUILDER
		gLog.debugPrintf("Build: CONVEX\n");
#   endif

		*S = CreateSubsector(tree);

		delete tree;

		if (cur_info->cancelled)
			return BUILD_Cancelled;

		return BUILD_OK;
	}

# if DEBUG_BUILDER
	gLog.debugPrintf("Build: PARTITION %p (%1.0f,%1.0f) -> (%1.0f,%1.0f)\n",
			part, part->start->x, part->start->y, part->end->x, part->end->y);
# endif

	node_t *node = NewNode();
	*N = node;

	/* divide the segs into two lists: left & right */
	seg_t *lefts  = NULL;
	seg_t *rights = NULL;
	intersection_t *cut_list = NULL;

	SeparateSegs(tree, part, &lefts, &rights, &cut_list, inst.level);

	delete tree;
	tree = NULL;

	/* sanity checks... */
	if (rights == NULL)
		BugError("Separated seg-list has empty RIGHT side\n");

	if (lefts == NULL)
		BugError("Separated seg-list has empty LEFT side\n");

	AddMinisegs(cut_list, part, &lefts, &rights);

	node->SetPartition(part, inst);

# if DEBUG_BUILDER
	gLog.debugPrintf("Build: Going LEFT\n");
# endif

	build_result_e ret;
	ret = BuildNodes(lefts, &node->l.bounds, &node->l.node, &node->l.subsec, depth+1, inst);

	if (ret != BUILD_OK)
		return ret;

# if DEBUG_BUILDER
	gLog.debugPrintf("Build: Going RIGHT\n");
# endif

	ret = BuildNodes(rights, &node->r.bounds, &node->r.node, &node->r.subsec, depth+1, inst);

# if DEBUG_BUILDER
	gLog.debugPrintf("Build: DONE\n");
# endif

	return ret;
}


void ClockwiseBspTree(const Document &doc)
{
	current_seg_index = 0;

	for (int i=0 ; i < num_subsecs ; i++)
	{
		subsec_t *sub = lev_subsecs[i];

		sub->ClockwiseOrder(doc);
		sub->RenumberSegs();

		// do some sanity checks
		sub->SanityCheckClosed();
		sub->SanityCheckHasRealSeg();
	}
}


void subsec_t::Normalise()
{
	// use head + tail to maintain same order of segs
	seg_t *new_head = NULL;
	seg_t *new_tail = NULL;

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: Normalising %d\n", index);
# endif

	while (seg_list)
	{
		// remove head
		seg_t *seg = seg_list;
		seg_list = seg->next;

		// only add non-minisegs to new list
		if (seg->linedef < 0)
		{
#     if DEBUG_SUBSEC
			gLog.debugPrintf("Subsec: Removing miniseg %p\n", seg);
#     endif

			// this causes SortSegs() to remove the seg
			seg->index = SEG_IS_GARBAGE;
			continue;
		}

		// add it to new list
		seg->next = NULL;

		if (new_tail)
			new_tail->next = seg;
		else
			new_head = seg;

		new_tail = seg;

		// this updated later
		seg->index = -1;
	}

	if (new_head == NULL)
		BugError("Subsector %d normalised to being EMPTY\n", index);

	seg_list = new_head;
}


void NormaliseBspTree()
{
	// unlinks all minisegs from each subsector

	current_seg_index = 0;

	for (int i=0 ; i < num_subsecs ; i++)
	{
		subsec_t *sub = lev_subsecs[i];

		sub->Normalise();
		sub->RenumberSegs();
	}
}


static void RoundOffVertices()
{
	for (int i = 0 ; i < num_vertices ; i++)
	{
		vertex_t *vert = lev_vertices[i];

		if (vert->is_new)
		{
			vert->is_new = false;

			vert->index = num_old_vert;
			num_old_vert++;
		}
	}
}


void subsec_t::RoundOff()
{
	// use head + tail to maintain same order of segs
	seg_t *new_head = NULL;
	seg_t *new_tail = NULL;

	seg_t *seg;
	seg_t *last_real_degen = NULL;

	int real_total  = 0;
	int degen_total = 0;

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: Rounding off %d\n", index);
# endif

	// do an initial pass, just counting the degenerates
	for (seg=seg_list ; seg ; seg=seg->next)
	{
		// is the seg degenerate ?
		if (iround(seg->start->x) == iround(seg->end->x) &&
			iround(seg->start->y) == iround(seg->end->y))
		{
			seg->is_degenerate = true;

			if (seg->linedef >= 0)
				last_real_degen = seg;

			degen_total++;
			continue;
		}

		if (seg->linedef >= 0)
			real_total++;
	}

# if DEBUG_SUBSEC
	gLog.debugPrintf("Subsec: degen=%d real=%d\n", degen_total, real_total);
# endif

	// handle the (hopefully rare) case where all of the real segs
	// became degenerate.
	if (real_total == 0)
	{
		if (last_real_degen == NULL)
			BugError("Subsector %d rounded off with NO real segs\n",
					index);

#   if DEBUG_SUBSEC
		gLog.debugPrintf("Degenerate before: (%1.2f,%1.2f) -> (%1.2f,%1.2f)\n",
				last_real_degen->start->x, last_real_degen->start->y,
				last_real_degen->end->x, last_real_degen->end->y);
#   endif

		// create a new vertex for this baby
		last_real_degen->end = NewVertexDegenerate(
				last_real_degen->start, last_real_degen->end);

#   if DEBUG_SUBSEC
		gLog.debugPrintf("Degenerate after:  (%d,%d) -> (%d,%d)\n",
						 iround(last_real_degen->start->x),
						 iround(last_real_degen->start->y),
						 iround(last_real_degen->end->x),
						 iround(last_real_degen->end->y));
#   endif

		last_real_degen->is_degenerate = false;
	}

	// second pass, remove the blighters...
	while (seg_list)
	{
		// remove head
		seg = seg_list;
		seg_list = seg->next;

		if (seg->is_degenerate)
		{
#     if DEBUG_SUBSEC
			gLog.debugPrintf("Subsec: Removing degenerate %p\n", seg);
#     endif

			// this causes SortSegs() to remove the seg
			seg->index = SEG_IS_GARBAGE;
			continue;
		}

		// add it to new list
		seg->next = NULL;

		if (new_tail)
			new_tail->next = seg;
		else
			new_head = seg;

		new_tail = seg;

		// this updated later
		seg->index = -1;
	}

	if (new_head == NULL)
		BugError("Subsector %d rounded off to being EMPTY\n", index);

	seg_list = new_head;
}


void RoundOffBspTree()
{
	current_seg_index = 0;

	RoundOffVertices();

	for (int i=0 ; i < num_subsecs ; i++)
	{
		subsec_t *sub = lev_subsecs[i];

		sub->RoundOff();
		sub->RenumberSegs();
	}
}


}  // namespace ajbsp

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
