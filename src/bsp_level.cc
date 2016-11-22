//------------------------------------------------------------------------
//
//  AJ-BSP  Copyright (C) 2000-2016  Andrew Apted, et al
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

#include "main.h"
#include "bsp.h"

#include "w_rawdef.h"
#include "w_wad.h"

#include <zlib.h>


namespace ajbsp
{

#define DEBUG_BLOCKMAP  0


static int block_x, block_y;
static int block_w, block_h;
static int block_count;

static int block_mid_x = 0;
static int block_mid_y = 0;

static u16_t ** block_lines;

static u16_t *block_ptrs;
static u16_t *block_dups;

static int block_compression;
static int block_overflowed;

#define BLOCK_LIMIT  16000

#define DUMMY_DUP  0xFFFF


void GetBlockmapBounds(int *x, int *y, int *w, int *h)
{
	*x = block_x; *y = block_y;
	*w = block_w; *h = block_h;
}


int CheckLinedefInsideBox(int xmin, int ymin, int xmax, int ymax,
		int x1, int y1, int x2, int y2)
{
	int count = 2;
	int tmp;

	for (;;)
	{
		if (y1 > ymax)
		{
			if (y2 > ymax)
				return false;

			x1 = x1 + (int) ((x2-x1) * (double)(ymax-y1) / (double)(y2-y1));
			y1 = ymax;

			count = 2;
			continue;
		}

		if (y1 < ymin)
		{
			if (y2 < ymin)
				return false;

			x1 = x1 + (int) ((x2-x1) * (double)(ymin-y1) / (double)(y2-y1));
			y1 = ymin;

			count = 2;
			continue;
		}

		if (x1 > xmax)
		{
			if (x2 > xmax)
				return false;

			y1 = y1 + (int) ((y2-y1) * (double)(xmax-x1) / (double)(x2-x1));
			x1 = xmax;

			count = 2;
			continue;
		}

		if (x1 < xmin)
		{
			if (x2 < xmin)
				return false;

			y1 = y1 + (int) ((y2-y1) * (double)(xmin-x1) / (double)(x2-x1));
			x1 = xmin;

			count = 2;
			continue;
		}

		count--;

		if (count == 0)
			break;

		/* swap end points */
		tmp=x1;  x1=x2;  x2=tmp;
		tmp=y1;  y1=y2;  y2=tmp;
	}

	/* linedef touches block */
	return true;
}


/* ----- create blockmap ------------------------------------ */

#define BK_NUM    0
#define BK_MAX    1
#define BK_XOR    2
#define BK_FIRST  3

#define BK_QUANTUM  32

static void BlockAdd(int blk_num, int line_index)
{
	u16_t *cur = block_lines[blk_num];

# if DEBUG_BLOCKMAP
	DebugPrintf("Block %d has line %d\n", blk_num, line_index);
# endif

	if (blk_num < 0 || blk_num >= block_count)
		BugError("BlockAdd: bad block number %d", blk_num);

	if (! cur)
	{
		// create empty block
		block_lines[blk_num] = cur = (u16_t *)UtilCalloc(BK_QUANTUM * sizeof(u16_t));
		cur[BK_NUM] = 0;
		cur[BK_MAX] = BK_QUANTUM;
		cur[BK_XOR] = 0x1234;
	}

	if (BK_FIRST + cur[BK_NUM] == cur[BK_MAX])
	{
		// no more room, so allocate some more...
		cur[BK_MAX] += BK_QUANTUM;

		block_lines[blk_num] = cur = (u16_t *)UtilRealloc(cur, cur[BK_MAX] * sizeof(u16_t));
	}

	// compute new checksum
	cur[BK_XOR] = ((cur[BK_XOR] << 4) | (cur[BK_XOR] >> 12)) ^ line_index;

	cur[BK_FIRST + cur[BK_NUM]] = LE_U16(line_index);
	cur[BK_NUM]++;
}


static void BlockAddLine(linedef_t *L)
{
	int x1 = (int) L->start->x;
	int y1 = (int) L->start->y;
	int x2 = (int) L->end->x;
	int y2 = (int) L->end->y;

	int bx1 = (MIN(x1,x2) - block_x) / 128;
	int by1 = (MIN(y1,y2) - block_y) / 128;
	int bx2 = (MAX(x1,x2) - block_x) / 128;
	int by2 = (MAX(y1,y2) - block_y) / 128;

	int bx, by;
	int line_index = L->index;

# if DEBUG_BLOCKMAP
	DebugPrintf("BlockAddLine: %d (%d,%d) -> (%d,%d)\n", line_index,
			x1, y1, x2, y2);
# endif

	// handle truncated blockmaps
	if (bx1 < 0) bx1 = 0;
	if (by1 < 0) by1 = 0;
	if (bx2 >= block_w) bx2 = block_w - 1;
	if (by2 >= block_h) by2 = block_h - 1;

	if (bx2 < bx1 || by2 < by1)
		return;

	// handle simple case #1: completely horizontal
	if (by1 == by2)
	{
		for (bx=bx1 ; bx <= bx2 ; bx++)
		{
			int blk_num = by1 * block_w + bx;
			BlockAdd(blk_num, line_index);
		}
		return;
	}

	// handle simple case #2: completely vertical
	if (bx1 == bx2)
	{
		for (by=by1 ; by <= by2 ; by++)
		{
			int blk_num = by * block_w + bx1;
			BlockAdd(blk_num, line_index);
		}
		return;
	}

	// handle the rest (diagonals)

	for (by=by1 ; by <= by2 ; by++)
	for (bx=bx1 ; bx <= bx2 ; bx++)
	{
		int blk_num = by * block_w + bx;

		int minx = block_x + bx * 128;
		int miny = block_y + by * 128;
		int maxx = minx + 127;
		int maxy = miny + 127;

		if (CheckLinedefInsideBox(minx, miny, maxx, maxy, x1, y1, x2, y2))
		{
			BlockAdd(blk_num, line_index);
		}
	}
}


static void CreateBlockmap(void)
{
	int i;

	block_lines = (u16_t **) UtilCalloc(block_count * sizeof(u16_t *));

	for (i=0 ; i < num_linedefs ; i++)
	{
		linedef_t *L = LookupLinedef(i);

		// ignore zero-length lines
		if (L->zero_len)
			continue;

		BlockAddLine(L);
	}
}


static int BlockCompare(const void *p1, const void *p2)
{
	int blk_num1 = ((const u16_t *) p1)[0];
	int blk_num2 = ((const u16_t *) p2)[0];

	const u16_t *A = block_lines[blk_num1];
	const u16_t *B = block_lines[blk_num2];

	if (A == B)
		return 0;

	if (A == NULL) return -1;
	if (B == NULL) return +1;

	if (A[BK_NUM] != B[BK_NUM])
	{
		return A[BK_NUM] - B[BK_NUM];
	}

	if (A[BK_XOR] != B[BK_XOR])
	{
		return A[BK_XOR] - B[BK_XOR];
	}

	return memcmp(A+BK_FIRST, B+BK_FIRST, A[BK_NUM] * sizeof(u16_t));
}


static void CompressBlockmap(void)
{
	int i;
	int cur_offset;
	int dup_count=0;

	int orig_size, new_size;

	block_ptrs = (u16_t *)UtilCalloc(block_count * sizeof(u16_t));
	block_dups = (u16_t *)UtilCalloc(block_count * sizeof(u16_t));

	// sort duplicate-detecting array.  After the sort, all duplicates
	// will be next to each other.  The duplicate array gives the order
	// of the blocklists in the BLOCKMAP lump.

	for (i=0 ; i < block_count ; i++)
		block_dups[i] = i;

	qsort(block_dups, block_count, sizeof(u16_t), BlockCompare);

	// scan duplicate array and build up offset array

	cur_offset = 4 + block_count + 2;

	orig_size = 4 + block_count;
	new_size  = cur_offset;

	for (i=0 ; i < block_count ; i++)
	{
		int blk_num = block_dups[i];
		int count;

		// empty block ?
		if (block_lines[blk_num] == NULL)
		{
			block_ptrs[blk_num] = 4 + block_count;
			block_dups[i] = DUMMY_DUP;

			orig_size += 2;
			continue;
		}

		count = 2 + block_lines[blk_num][BK_NUM];

		// duplicate ?  Only the very last one of a sequence of duplicates
		// will update the current offset value.

		if (i+1 < block_count &&
				BlockCompare(block_dups + i, block_dups + i+1) == 0)
		{
			block_ptrs[blk_num] = cur_offset;
			block_dups[i] = DUMMY_DUP;

			// free the memory of the duplicated block
			UtilFree(block_lines[blk_num]);
			block_lines[blk_num] = NULL;

			dup_count++;

			orig_size += count;
			continue;
		}

		// OK, this block is either the last of a series of duplicates, or
		// just a singleton.

		block_ptrs[blk_num] = cur_offset;

		cur_offset += count;

		orig_size += count;
		new_size  += count;
	}

	if (cur_offset > 65535)
	{
		block_overflowed = true;
		return;
	}

# if DEBUG_BLOCKMAP
	DebugPrintf("Blockmap: Last ptr = %d  duplicates = %d\n",
			cur_offset, dup_count);
# endif

	block_compression = (orig_size - new_size) * 100 / orig_size;

	// there's a tiny chance of new_size > orig_size
	if (block_compression < 0)
		block_compression = 0;
}


static int CalcBlockmapSize()
{
	// compute size of final BLOCKMAP lump.
	// it does not need to be exact, but it *does* need to be bigger
	// (or equal) to the actual size of the lump.

	// header + null_block + a bit extra
	int size = 20;

	// the pointers (offsets to the line lists)
	size = size + block_count * 2;

	// add size of each block
	for (int i=0 ; i < block_count ; i++)
	{
		int blk_num = block_dups[i];

		// ignore duplicate or empty blocks
		if (blk_num == DUMMY_DUP)
			continue;

		u16_t *blk = block_lines[blk_num];
		SYS_ASSERT(blk);

		size += (1 + (int)(blk[BK_NUM]) + 1) * 2;
	}

	return size;
}


static void WriteBlockmap(void)
{
	int i;

	int max_size = CalcBlockmapSize();

	Lump_c *lump = CreateLevelLump("BLOCKMAP", max_size);

	u16_t null_block[2] = { 0x0000, 0xFFFF };
	u16_t m_zero = 0x0000;
	u16_t m_neg1 = 0xFFFF;

	// fill in header
	raw_blockmap_header_t header;

	header.x_origin = LE_U16(block_x);
	header.y_origin = LE_U16(block_y);
	header.x_blocks = LE_U16(block_w);
	header.y_blocks = LE_U16(block_h);

	lump->Write(&header, sizeof(header));

	// handle pointers
	for (i=0 ; i < block_count ; i++)
	{
		u16_t ptr = LE_U16(block_ptrs[i]);

		if (ptr == 0)
			BugError("WriteBlockmap: offset %d not set.", i);

		lump->Write(&ptr, sizeof(u16_t));
	}

	// add the null block which *all* empty blocks will use
	lump->Write(null_block, sizeof(null_block));

	// handle each block list
	for (i=0 ; i < block_count ; i++)
	{
		int blk_num = block_dups[i];

		// ignore duplicate or empty blocks
		if (blk_num == DUMMY_DUP)
			continue;

		u16_t *blk = block_lines[blk_num];
		SYS_ASSERT(blk);

		lump->Write(&m_zero, sizeof(u16_t));
		lump->Write(blk + BK_FIRST, blk[BK_NUM] * sizeof(u16_t));
		lump->Write(&m_neg1, sizeof(u16_t));
	}

	lump->Finish();
}


static void FreeBlockmap(void)
{
	for (int i=0 ; i < block_count ; i++)
	{
		if (block_lines[i])
			UtilFree(block_lines[i]);
	}

	UtilFree(block_lines);
	UtilFree(block_ptrs);
	UtilFree(block_dups);
}


static void FindBlockmapLimits(bbox_t *bbox)
{
	int i;

	int mid_x = 0;
	int mid_y = 0;

	bbox->minx = bbox->miny = SHRT_MAX;
	bbox->maxx = bbox->maxy = SHRT_MIN;

	for (i=0 ; i < num_linedefs ; i++)
	{
		linedef_t *L = LookupLinedef(i);

		if (! L->zero_len)
		{
			double x1 = L->start->x;
			double y1 = L->start->y;
			double x2 = L->end->x;
			double y2 = L->end->y;

			int lx = (int)floor(MIN(x1, x2));
			int ly = (int)floor(MIN(y1, y2));
			int hx = (int)ceil(MAX(x1, x2));
			int hy = (int)ceil(MAX(y1, y2));

			if (lx < bbox->minx) bbox->minx = lx;
			if (ly < bbox->miny) bbox->miny = ly;
			if (hx > bbox->maxx) bbox->maxx = hx;
			if (hy > bbox->maxy) bbox->maxy = hy;

			// compute middle of cluster (roughly, so we don't overflow)
			mid_x += (lx + hx) / 32;
			mid_y += (ly + hy) / 32;
		}
	}

	if (num_linedefs > 0)
	{
		block_mid_x = (mid_x / num_linedefs) * 16;
		block_mid_y = (mid_y / num_linedefs) * 16;
	}

# if DEBUG_BLOCKMAP
	DebugPrintf("Blockmap lines centered at (%d,%d)\n", block_mid_x, block_mid_y);
# endif
}


void InitBlockmap()
{
	bbox_t map_bbox;

	/* find limits of linedefs, and store as map limits */
	FindBlockmapLimits(&map_bbox);

	PrintVerbose("Map goes from (%d,%d) to (%d,%d)\n",
			map_bbox.minx, map_bbox.miny, map_bbox.maxx, map_bbox.maxy);

	block_x = map_bbox.minx - (map_bbox.minx & 0x7);
	block_y = map_bbox.miny - (map_bbox.miny & 0x7);

	block_w = ((map_bbox.maxx - block_x) / 128) + 1;
	block_h = ((map_bbox.maxy - block_y) / 128) + 1;

	block_count = block_w * block_h;
}


void PutBlockmap()
{
	if (! cur_info->do_blockmap)
	{
		// just create an empty blockmap lump
		CreateLevelLump("BLOCKMAP")->Finish();
		return;
	}

	block_overflowed = false;

	// initial phase: create internal blockmap containing the index of
	// all lines in each block.

	CreateBlockmap();

	// -AJA- second phase: compress the blockmap.  We do this by sorting
	//       the blocks, which is a typical way to detect duplicates in
	//       a large list.  This also detects BLOCKMAP overflow.

	CompressBlockmap();

	// final phase: write it out in the correct format

	if (block_overflowed)
	{
		// leave an empty blockmap lump
		CreateLevelLump("BLOCKMAP")->Finish();

		PrintWarn("Blockmap overflowed (lump will be empty)\n");
	}
	else
	{
		WriteBlockmap();

		PrintVerbose("Completed blockmap, size %dx%d (compression: %d%%)\n",
				block_w, block_h, block_compression);
	}

	FreeBlockmap();
}


//------------------------------------------------------------------------
// REJECT : Generate the reject table
//------------------------------------------------------------------------


#define DEBUG_REJECT  0

static u8_t *rej_matrix;
static int   rej_total_size;	// in bytes


//
// Allocate the matrix, init sectors into individual groups.
//
static void Reject_Init()
{
	rej_total_size = (num_sectors * num_sectors + 7) / 8;

	rej_matrix = new u8_t[rej_total_size];

	memset(rej_matrix, 0, rej_total_size);


	for (int i=0 ; i < num_sectors ; i++)
	{
		sector_t *sec = LookupSector(i);

		sec->rej_group = i;
		sec->rej_next = sec->rej_prev = sec;
	}
}


static void Reject_Free()
{
	delete[] rej_matrix;
	rej_matrix = NULL;
}


//
// Algorithm: Initially all sectors are in individual groups.  Now we
// scan the linedef list.  For each 2-sectored line, merge the two
// sector groups into one.  That's it !
//
static void Reject_GroupSectors()
{
	int i;

	for (i=0 ; i < num_linedefs ; i++)
	{
		linedef_t *line = LookupLinedef(i);
		sector_t *sec1, *sec2, *tmp;

		if (! line->right || ! line->left)
			continue;

		// the standard DOOM engine will not allow sight past lines
		// lacking the TWOSIDED flag, so we can skip them here too.
		if (! line->two_sided)
			continue;

		sec1 = line->right->sector;
		sec2 = line->left->sector;

		if (! sec1 || ! sec2 || sec1 == sec2)
			continue;

		// already in the same group ?
		if (sec1->rej_group == sec2->rej_group)
			continue;

		// swap sectors so that the smallest group is added to the biggest
		// group.  This is based on the assumption that sector numbers in
		// wads will generally increase over the set of linedefs, and so
		// (by swapping) we'll tend to add small groups into larger
		// groups, thereby minimising the updates to 'rej_group' fields
		// that is required when merging.

		if (sec1->rej_group > sec2->rej_group)
		{
			tmp = sec1; sec1 = sec2; sec2 = tmp;
		}

		// update the group numbers in the second group

		sec2->rej_group = sec1->rej_group;

		for (tmp=sec2->rej_next ; tmp != sec2 ; tmp=tmp->rej_next)
			tmp->rej_group = sec1->rej_group;

		// merge 'em baby...

		sec1->rej_next->rej_prev = sec2;
		sec2->rej_next->rej_prev = sec1;

		tmp = sec1->rej_next;
		sec1->rej_next = sec2->rej_next;
		sec2->rej_next = tmp;
	}
}


#if DEBUG_REJECT
static void Reject_DebugGroups()
{
	// Note: this routine is destructive to the group numbers

	int i;

	for (i=0 ; i < num_sectors ; i++)
	{
		sector_t *sec = LookupSector(i);
		sector_t *tmp;

		int group = sec->rej_group;
		int num = 0;

		if (group < 0)
			continue;

		sec->rej_group = -1;
		num++;

		for (tmp=sec->rej_next ; tmp != sec ; tmp=tmp->rej_next)
		{
			tmp->rej_group = -1;
			num++;
		}

		DebugPrintf("Group %d  Sectors %d\n", group, num);
	}
}
#endif


static void Reject_ProcessSectors()
{
	for (int view=0 ; view < num_sectors ; view++)
	{
		for (int target=0 ; target < view ; target++)
		{
			sector_t *view_sec = LookupSector(view);
			sector_t *targ_sec = LookupSector(target);

			int p1, p2;

			if (view_sec->rej_group == targ_sec->rej_group)
				continue;

			// for symmetry, do both sides at same time

			p1 = view * num_sectors + target;
			p2 = target * num_sectors + view;

			rej_matrix[p1 >> 3] |= (1 << (p1 & 7));
			rej_matrix[p2 >> 3] |= (1 << (p2 & 7));
		}
	}
}


static void Reject_WriteLump()
{
	Lump_c *lump = CreateLevelLump("REJECT", rej_total_size);

	lump->Write(rej_matrix, rej_total_size);

	lump->Finish();
}


//
// For now we only do very basic reject processing, limited to
// determining all isolated groups of sectors (islands that are
// surrounded by void space).
//
void PutReject()
{
	if (! cur_info->do_reject || num_sectors == 0)
	{
		// just create an empty reject lump
		CreateLevelLump("REJECT")->Finish();
		return;
	}

	Reject_Init();
	Reject_GroupSectors();
	Reject_ProcessSectors();

# if DEBUG_REJECT
	Reject_DebugGroups();
# endif

	Reject_WriteLump();
	Reject_Free();

	PrintVerbose("Added simple reject lump\n");
}


//------------------------------------------------------------------------
// LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------


// Note: ZDBSP format support based on code (C) 2002,2003 Randy Heit


#define DEBUG_LOAD      0
#define DEBUG_BSP       0

#define ALLOC_BLKNUM  1024


// per-level variables

const char *lev_current_name;

short lev_current_idx;
short lev_current_start;

bool lev_doing_hexen;

bool lev_force_v5;
bool lev_force_xnod;

bool lev_long_name;


#define LEVELARRAY(TYPE, BASEVAR, NUMVAR)  \
    TYPE ** BASEVAR = NULL;  \
    int NUMVAR = 0;


LEVELARRAY(vertex_t,  lev_vertices,   num_vertices)
LEVELARRAY(linedef_t, lev_linedefs,   num_linedefs)
LEVELARRAY(sidedef_t, lev_sidedefs,   num_sidedefs)
LEVELARRAY(sector_t,  lev_sectors,    num_sectors)
LEVELARRAY(thing_t,   lev_things,     num_things)

static LEVELARRAY(seg_t,     segs,       num_segs)
static LEVELARRAY(subsec_t,  subsecs,    num_subsecs)
static LEVELARRAY(node_t,    nodes,      num_nodes)
static LEVELARRAY(wall_tip_t,wall_tips,  num_wall_tips)


int num_normal_vert = 0;
int num_gl_vert = 0;
int num_complete_seg = 0;


/* ----- allocation routines ---------------------------- */

#define ALLIGATOR(TYPE, BASEVAR, NUMVAR)  \
{  \
  if ((NUMVAR % ALLOC_BLKNUM) == 0)  \
  {  \
    BASEVAR = (TYPE **) UtilRealloc(BASEVAR, (NUMVAR + ALLOC_BLKNUM) * sizeof(TYPE *));  \
  }  \
  BASEVAR[NUMVAR] = (TYPE *) UtilCalloc(sizeof(TYPE));  \
  NUMVAR += 1;  \
  return BASEVAR[NUMVAR - 1];  \
}


vertex_t *NewVertex(void)
	ALLIGATOR(vertex_t, lev_vertices, num_vertices)

linedef_t *NewLinedef(void)
	ALLIGATOR(linedef_t, lev_linedefs, num_linedefs)

sidedef_t *NewSidedef(void)
	ALLIGATOR(sidedef_t, lev_sidedefs, num_sidedefs)

sector_t *NewSector(void)
	ALLIGATOR(sector_t, lev_sectors, num_sectors)

thing_t *NewThing(void)
	ALLIGATOR(thing_t, lev_things, num_things)

seg_t *NewSeg(void)
	ALLIGATOR(seg_t, segs, num_segs)

subsec_t *NewSubsec(void)
	ALLIGATOR(subsec_t, subsecs, num_subsecs)

node_t *NewNode(void)
	ALLIGATOR(node_t, nodes, num_nodes)

wall_tip_t *NewWallTip(void)
	ALLIGATOR(wall_tip_t, wall_tips, num_wall_tips)


/* ----- free routines ---------------------------- */

#define FREEMASON(TYPE, BASEVAR, NUMVAR)  \
{  \
  int i;  \
  for (i=0 ; i < NUMVAR ; i++)  \
    UtilFree(BASEVAR[i]);  \
  if (BASEVAR)  \
    UtilFree(BASEVAR);  \
  BASEVAR = NULL; NUMVAR = 0;  \
}


void FreeVertices(void)
	FREEMASON(vertex_t, lev_vertices, num_vertices)

void FreeLinedefs(void)
	FREEMASON(linedef_t, lev_linedefs, num_linedefs)

void FreeSidedefs(void)
	FREEMASON(sidedef_t, lev_sidedefs, num_sidedefs)

void FreeSectors(void)
	FREEMASON(sector_t, lev_sectors, num_sectors)

void FreeThings(void)
	FREEMASON(thing_t, lev_things, num_things)

void FreeSegs(void)
	FREEMASON(seg_t, segs, num_segs)

void FreeSubsecs(void)
	FREEMASON(subsec_t, subsecs, num_subsecs)

void FreeNodes(void)
	FREEMASON(node_t, nodes, num_nodes)

void FreeWallTips(void)
	FREEMASON(wall_tip_t, wall_tips, num_wall_tips)


/* ----- lookup routines ------------------------------ */

#define LOOKERUPPER(BASEVAR, NUMVAR, NAMESTR)  \
{  \
  if (index < 0 || index >= NUMVAR)  \
    FatalError("No such %s number #%d", NAMESTR, index);  \
    \
  return BASEVAR[index];  \
}

vertex_t *LookupVertex(int index)
	LOOKERUPPER(lev_vertices, num_vertices, "vertex")

linedef_t *LookupLinedef(int index)
	LOOKERUPPER(lev_linedefs, num_linedefs, "linedef")

sidedef_t *LookupSidedef(int index)
	LOOKERUPPER(lev_sidedefs, num_sidedefs, "sidedef")

sector_t *LookupSector(int index)
	LOOKERUPPER(lev_sectors, num_sectors, "sector")

thing_t *LookupThing(int index)
	LOOKERUPPER(lev_things, num_things, "thing")

seg_t *LookupSeg(int index)
	LOOKERUPPER(segs, num_segs, "seg")

subsec_t *LookupSubsec(int index)
	LOOKERUPPER(subsecs, num_subsecs, "subsector")

node_t *LookupNode(int index)
	LOOKERUPPER(nodes, num_nodes, "node")


/* ----- reading routines ------------------------------ */


void GetVertices(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("VERTEXES");

	if (lump)
		count = lump->Length() / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	DebugPrintf("GetVertices: num = %d\n", count);
# endif

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

	for (i = 0 ; i < count ; i++)
	{
		raw_vertex_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading vertices.\n");

		vertex_t *vert = NewVertex();

		vert->x = (double) LE_S16(raw.x);
		vert->y = (double) LE_S16(raw.y);

		vert->index = i;
	}

	num_normal_vert = num_vertices;
	num_gl_vert = 0;
	num_complete_seg = 0;
}


void GetSectors(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("SECTORS");

	if (lump)
		count = lump->Length() / sizeof(raw_sector_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetSectors: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_sector_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sectors.\n");

		sector_t *sector = NewSector();

		sector->floor_h = LE_S16(raw.floorh);
		sector->ceil_h  = LE_S16(raw.ceilh);

		memcpy(sector->floor_tex, raw.floor_tex, sizeof(sector->floor_tex));
		memcpy(sector->ceil_tex,  raw.ceil_tex,  sizeof(sector->ceil_tex));

		sector->light = LE_U16(raw.light);
		sector->special = LE_U16(raw.type);
		sector->tag = LE_S16(raw.tag);

		sector->coalesce = (sector->tag >= 900 && sector->tag < 1000) ?
			true : false;

		/* sector indices never change */
		sector->index = i;

		sector->warned_facing = -1;

		/* Note: rej_* fields are handled completely in reject.c */
	}
}


void GetThings(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("THINGS");

	if (lump)
		count = lump->Length() / sizeof(raw_thing_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetThings: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading things.\n");

		thing_t *thing = NewThing();

		thing->x = LE_S16(raw.x);
		thing->y = LE_S16(raw.y);

		thing->type = LE_U16(raw.type);
		thing->options = LE_U16(raw.options);

		thing->index = i;
	}
}


void GetThingsHexen(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("THINGS");

	if (lump)
		count = lump->Length() / sizeof(raw_hexen_thing_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetThingsHexen: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_hexen_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading things.\n");

		thing_t *thing = NewThing();

		thing->x = LE_S16(raw.x);
		thing->y = LE_S16(raw.y);

		thing->type = LE_U16(raw.type);
		thing->options = LE_U16(raw.options);

		thing->index = i;
	}
}


void GetSidedefs(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("SIDEDEFS");

	if (lump)
		count = lump->Length() / sizeof(raw_sidedef_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetSidedefs: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_sidedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sidedefs.\n");

		sidedef_t *side = NewSidedef();

		side->sector = (LE_S16(raw.sector) == -1) ? NULL :
			LookupSector(LE_U16(raw.sector));

		if (side->sector)
			side->sector->ref_count++;

		side->x_offset = LE_S16(raw.x_offset);
		side->y_offset = LE_S16(raw.y_offset);

		memcpy(side->upper_tex, raw.upper_tex, sizeof(side->upper_tex));
		memcpy(side->lower_tex, raw.lower_tex, sizeof(side->lower_tex));
		memcpy(side->mid_tex,   raw.mid_tex,   sizeof(side->mid_tex));

		/* sidedef indices never change */
		side->index = i;
	}
}

static inline sidedef_t *SafeLookupSidedef(u16_t num)
{
	if (num == 0xFFFF)
		return NULL;

	if ((int)num >= num_sidedefs && (s16_t)(num) < 0)
		return NULL;

	return LookupSidedef(num);
}


void GetLinedefs(void)
{
	int i, count=-1;

	Lump_c *lump = FindLevelLump("LINEDEFS");

	if (lump)
		count = lump->Length() / sizeof(raw_linedef_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetLinedefs: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading linedefs.\n");

		linedef_t *line;

		vertex_t *start = LookupVertex(LE_U16(raw.start));
		vertex_t *end   = LookupVertex(LE_U16(raw.end));

		start->ref_count++;
		end->ref_count++;

		line = NewLinedef();

		line->start = start;
		line->end   = end;

		/* check for zero-length line */
		line->zero_len = (fabs(start->x - end->x) < DIST_EPSILON) &&
			(fabs(start->y - end->y) < DIST_EPSILON);

		line->flags = LE_U16(raw.flags);
		line->type = LE_U16(raw.type);
		line->tag  = LE_S16(raw.tag);

		line->two_sided = (line->flags & MLF_TwoSided) ? true : false;
		line->is_precious = (line->tag >= 900 && line->tag < 1000) ?
			true : false;

		line->right = SafeLookupSidedef(LE_U16(raw.right));
		line->left  = SafeLookupSidedef(LE_U16(raw.left));

		if (line->right)
		{
			line->right->ref_count++;
			line->right->on_special |= (line->type > 0) ? 1 : 0;
		}

		if (line->left)
		{
			line->left->ref_count++;
			line->left->on_special |= (line->type > 0) ? 1 : 0;
		}

		line->self_ref = (line->left && line->right &&
				(line->left->sector == line->right->sector));

		line->index = i;
	}
}


void GetLinedefsHexen(void)
{
	int i, j, count=-1;

	Lump_c *lump = FindLevelLump("LINEDEFS");

	if (lump)
		count = lump->Length() / sizeof(raw_hexen_linedef_t);

	if (!lump || count == 0)
		return;

	if (! lump->Seek())
		FatalError("Error seeking to a lump\n");

# if DEBUG_LOAD
	DebugPrintf("GetLinedefsHexen: num = %d\n", count);
# endif

	for (i = 0 ; i < count ; i++)
	{
		raw_hexen_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading linedefs.\n");

		linedef_t *line;

		vertex_t *start = LookupVertex(LE_U16(raw.start));
		vertex_t *end   = LookupVertex(LE_U16(raw.end));

		start->ref_count++;
		end->ref_count++;

		line = NewLinedef();

		line->start = start;
		line->end   = end;

		// check for zero-length line
		line->zero_len = (fabs(start->x - end->x) < DIST_EPSILON) &&
			(fabs(start->y - end->y) < DIST_EPSILON);

		line->flags = LE_U16(raw.flags);
		line->type = (u8_t)(raw.type);
		line->tag  = 0;

		/* read specials */
		for (j=0 ; j < 5 ; j++)
			line->specials[j] = (u8_t)(raw.args[j]);

		// -JL- Added missing twosided flag handling that caused a broken reject
		line->two_sided = (line->flags & MLF_TwoSided) ? true : false;

		line->right = SafeLookupSidedef(LE_U16(raw.right));
		line->left  = SafeLookupSidedef(LE_U16(raw.left));

		// -JL- Added missing sidedef handling that caused all sidedefs to be pruned
		if (line->right)
		{
			line->right->ref_count++;
			line->right->on_special |= (line->type > 0) ? 1 : 0;
		}

		if (line->left)
		{
			line->left->ref_count++;
			line->left->on_special |= (line->type > 0) ? 1 : 0;
		}

		line->self_ref = (line->left && line->right &&
				(line->left->sector == line->right->sector));

		line->index = i;
	}
}


static inline int TransformSegDist(const seg_t *seg)
{
	double sx = seg->side ? seg->linedef->end->x : seg->linedef->start->x;
	double sy = seg->side ? seg->linedef->end->y : seg->linedef->start->y;

	return (int) ceil(UtilComputeDist(seg->start->x - sx, seg->start->y - sy));
}

static inline int TransformAngle(angle_g angle)
{
	int result;

	result = (int)(angle * 65536.0 / 360.0);

	if (result < 0)
		result += 65536;

	return (result & 0xFFFF);
}

static int SegCompare(const void *p1, const void *p2)
{
	const seg_t *A = ((const seg_t **) p1)[0];
	const seg_t *B = ((const seg_t **) p2)[0];

	if (A->index < 0)
		BugError("Seg %p never reached a subsector !", A);

	if (B->index < 0)
		BugError("Seg %p never reached a subsector !", B);

	return (A->index - B->index);
}


/* ----- writing routines ------------------------------ */

static const u8_t *lev_v2_magic = (u8_t *) "gNd2";
static const u8_t *lev_v5_magic = (u8_t *) "gNd5";


void PutVertices(const char *name, int do_gl)
{
	int count, i;

	// this size is worst-case scenario
	int size = num_vertices * (int)sizeof(raw_vertex_t);

	Lump_c *lump = CreateLevelLump(name, size);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if ((do_gl ? 1 : 0) != ((vert->index & IS_GL_VERTEX) ? 1 : 0))
		{
			continue;
		}

		raw.x = LE_S16(I_ROUND(vert->x));
		raw.y = LE_S16(I_ROUND(vert->y));

		lump->Write(&raw, sizeof(raw));

		count++;
	}

	if (count != (do_gl ? num_gl_vert : num_normal_vert))
		BugError("PutVertices miscounted (%d != %d)", count,
				do_gl ? num_gl_vert : num_normal_vert);

	if (! do_gl && count > 65534)
		MarkHardFailure(LIMIT_VERTEXES);
	else if (count > 32767)
		MarkSoftFailure(do_gl ? LIMIT_GL_VERT : LIMIT_VERTEXES);
}


void PutGLVertices(int do_v5)
{
	int count, i;

	// this size is worst-case scenario
	int size = 4 + num_vertices * (int)sizeof(raw_v2_vertex_t);

	Lump_c *lump = CreateLevelLump("GL_VERT", size);

	if (do_v5)
		lump->Write(lev_v5_magic, 4);
	else
		lump->Write(lev_v2_magic, 4);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_v2_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if (! (vert->index & IS_GL_VERTEX))
			continue;

		raw.x = LE_S32((int)(vert->x * 65536.0));
		raw.y = LE_S32((int)(vert->y * 65536.0));

		lump->Write(&raw, sizeof(raw));

		count++;
	}

	if (count != num_gl_vert)
		BugError("PutGLVertices miscounted (%d != %d)", count,
				num_gl_vert);

	if (count > 32767)
		MarkSoftFailure(LIMIT_GL_VERT);
}


void ValidateSectors(void)
{
	if (num_sectors > 65534)
		MarkHardFailure(LIMIT_SECTORS);
	else if (num_sectors > 32767)
		MarkSoftFailure(LIMIT_SECTORS);
}

void ValidateSidedefs(void)
{
	if (num_sidedefs > 65534)
		MarkHardFailure(LIMIT_SIDEDEFS);
	else if (num_sidedefs > 32767)
		MarkSoftFailure(LIMIT_SIDEDEFS);
}

void ValidateLinedefs(void)
{
	if (num_linedefs > 65534)
		MarkHardFailure(LIMIT_LINEDEFS);
	else if (num_linedefs > 32767)
		MarkSoftFailure(LIMIT_LINEDEFS);
}


static inline u16_t VertexIndex16Bit(const vertex_t *v)
{
	if (v->index & IS_GL_VERTEX)
		return (u16_t) ((v->index & ~IS_GL_VERTEX) | 0x8000U);

	return (u16_t) v->index;
}


static inline u32_t VertexIndex32BitV5(const vertex_t *v)
{
	if (v->index & IS_GL_VERTEX)
		return (u32_t) ((v->index & ~IS_GL_VERTEX) | 0x80000000U);

	return (u32_t) v->index;
}


void PutSegs(void)
{
	int i, count;

	// this size is worst-case scenario
	int size = num_segs * (int)sizeof(raw_seg_t);

	Lump_c *lump = CreateLevelLump("SEGS", size);

	// sort segs into ascending index
	qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_seg_t raw;

		seg_t *seg = segs[i];

		// ignore minisegs and degenerate segs
		if (! seg->linedef || seg->degenerate)
			continue;

		raw.start   = LE_U16(VertexIndex16Bit(seg->start));
		raw.end     = LE_U16(VertexIndex16Bit(seg->end));
		raw.angle   = LE_U16(TransformAngle(seg->p_angle));
		raw.linedef = LE_U16(seg->linedef->index);
		raw.flip    = LE_U16(seg->side);
		raw.dist    = LE_U16(TransformSegDist(seg));

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		DebugPrintf("PUT SEG: %04X  Vert %04X->%04X  Line %04X %s  "
				"Angle %04X  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index,
				LE_U16(raw.start), LE_U16(raw.end), LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.angle),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_complete_seg)
		BugError("PutSegs miscounted (%d != %d)", count,
				num_complete_seg);

	if (count > 65534)
		MarkHardFailure(LIMIT_SEGS);
	else if (count > 32767)
		MarkSoftFailure(LIMIT_SEGS);
}


void PutGLSegs(void)
{
	int i, count;

	// this size is worst-case scenario
	int size = num_segs * (int)sizeof(raw_gl_seg_t);

	Lump_c *lump = CreateLevelLump("GL_SEGS", size);

	// sort segs into ascending index
	qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_gl_seg_t raw;

		seg_t *seg = segs[i];

		// ignore degenerate segs
		if (seg->degenerate)
			continue;

		raw.start = LE_U16(VertexIndex16Bit(seg->start));
		raw.end   = LE_U16(VertexIndex16Bit(seg->end));
		raw.side  = LE_U16(seg->side);

		if (seg->linedef)
			raw.linedef = LE_U16(seg->linedef->index);
		else
			raw.linedef = LE_U16(0xFFFF);

		if (seg->partner)
			raw.partner = LE_U16(seg->partner->index);
		else
			raw.partner = LE_U16(0xFFFF);

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		DebugPrintf("PUT GL SEG: %04X  Line %04X %s  Partner %04X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_complete_seg)
		BugError("PutGLSegs miscounted (%d != %d)", count,
				num_complete_seg);

	if (count > 65534)
		BugError("PutGLSegs with %d (> 65534) segs", count);
	else if (count > 32767)
		MarkSoftFailure(LIMIT_GL_SEGS);
}


void PutGLSegs_V5()
{
	int i, count;

	// this size is worst-case scenario
	int size = num_segs * (int)sizeof(raw_v5_seg_t);

	Lump_c *lump = CreateLevelLump("GL_SEGS", size);

	// sort segs into ascending index
	qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_v5_seg_t raw;

		seg_t *seg = segs[i];

		// ignore degenerate segs
		if (seg->degenerate)
			continue;

		raw.start = LE_U32(VertexIndex32BitV5(seg->start));
		raw.end   = LE_U32(VertexIndex32BitV5(seg->end));

		raw.side  = LE_U16(seg->side);

		if (seg->linedef)
			raw.linedef = LE_U16(seg->linedef->index);
		else
			raw.linedef = LE_U16(0xFFFF);

		if (seg->partner)
			raw.partner = LE_U32(seg->partner->index);
		else
			raw.partner = LE_U32(0xFFFFFFFF);

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		DebugPrintf("PUT V3 SEG: %06X  Line %04X %s  Partner %06X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U32(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_complete_seg)
		BugError("PutGLSegs miscounted (%d != %d)", count,
				num_complete_seg);
}


void PutSubsecs(const char *name, int do_gl)
{
	int i;

	int size = num_subsecs * (int)sizeof(raw_subsec_t);

	Lump_c * lump = CreateLevelLump(name, size);

	for (i=0 ; i < num_subsecs ; i++)
	{
		raw_subsec_t raw;

		subsec_t *sub = subsecs[i];

		raw.first = LE_U16(sub->seg_list->index);
		raw.num   = LE_U16(sub->seg_count);

		lump->Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		DebugPrintf("PUT SUBSEC %04X  First %04X  Num %04X\n",
				sub->index, LE_U16(raw.first), LE_U16(raw.num));
#   endif
	}

	if (num_subsecs > 32767)
		MarkHardFailure(do_gl ? LIMIT_GL_SSECT : LIMIT_SSECTORS);
}


void PutGLSubsecs_V5()
{
	int i;

	int size = num_subsecs * (int)sizeof(raw_v5_subsec_t);

	Lump_c *lump = CreateLevelLump("GL_SSECT", size);

	for (i=0 ; i < num_subsecs ; i++)
	{
		raw_v5_subsec_t raw;

		subsec_t *sub = subsecs[i];

		raw.first = LE_U32(sub->seg_list->index);
		raw.num   = LE_U32(sub->seg_count);

		lump->Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		DebugPrintf("PUT V3 SUBSEC %06X  First %06X  Num %06X\n",
					sub->index, LE_U32(raw.first), LE_U32(raw.num));
#   endif
	}
}


static int node_cur_index;

static void PutOneNode(node_t *node, Lump_c *lump)
{
	raw_node_t raw;

	if (node->r.node)
		PutOneNode(node->r.node, lump);

	if (node->l.node)
		PutOneNode(node->l.node, lump);

	node->index = node_cur_index++;

	raw.x  = LE_S16(node->x);
	raw.y  = LE_S16(node->y);
	raw.dx = LE_S16(node->dx / (node->too_long ? 2 : 1));
	raw.dy = LE_S16(node->dy / (node->too_long ? 2 : 1));

	raw.b1.minx = LE_S16(node->r.bounds.minx);
	raw.b1.miny = LE_S16(node->r.bounds.miny);
	raw.b1.maxx = LE_S16(node->r.bounds.maxx);
	raw.b1.maxy = LE_S16(node->r.bounds.maxy);

	raw.b2.minx = LE_S16(node->l.bounds.minx);
	raw.b2.miny = LE_S16(node->l.bounds.miny);
	raw.b2.maxx = LE_S16(node->l.bounds.maxx);
	raw.b2.maxy = LE_S16(node->l.bounds.maxy);

	if (node->r.node)
		raw.right = LE_U16(node->r.node->index);
	else if (node->r.subsec)
		raw.right = LE_U16(node->r.subsec->index | 0x8000);
	else
		BugError("Bad right child in node %d", node->index);

	if (node->l.node)
		raw.left = LE_U16(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U16(node->l.subsec->index | 0x8000);
	else
		BugError("Bad left child in node %d", node->index);

	lump->Write(&raw, sizeof(raw));

# if DEBUG_BSP
	DebugPrintf("PUT NODE %04X  Left %04X  Right %04X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U16(raw.left),
			LE_U16(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


static void PutOneNode_V5(node_t *node, Lump_c *lump)
{
	raw_v5_node_t raw;

	if (node->r.node)
		PutOneNode_V5(node->r.node, lump);

	if (node->l.node)
		PutOneNode_V5(node->l.node, lump);

	node->index = node_cur_index++;

	raw.x  = LE_S16(node->x);
	raw.y  = LE_S16(node->y);
	raw.dx = LE_S16(node->dx / (node->too_long ? 2 : 1));
	raw.dy = LE_S16(node->dy / (node->too_long ? 2 : 1));

	raw.b1.minx = LE_S16(node->r.bounds.minx);
	raw.b1.miny = LE_S16(node->r.bounds.miny);
	raw.b1.maxx = LE_S16(node->r.bounds.maxx);
	raw.b1.maxy = LE_S16(node->r.bounds.maxy);

	raw.b2.minx = LE_S16(node->l.bounds.minx);
	raw.b2.miny = LE_S16(node->l.bounds.miny);
	raw.b2.maxx = LE_S16(node->l.bounds.maxx);
	raw.b2.maxy = LE_S16(node->l.bounds.maxy);

	if (node->r.node)
		raw.right = LE_U32(node->r.node->index);
	else if (node->r.subsec)
		raw.right = LE_U32(node->r.subsec->index | 0x80000000U);
	else
		BugError("Bad right child in V5 node %d", node->index);

	if (node->l.node)
		raw.left = LE_U32(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U32(node->l.subsec->index | 0x80000000U);
	else
		BugError("Bad left child in V5 node %d", node->index);

	lump->Write(&raw, sizeof(raw));

# if DEBUG_BSP
	DebugPrintf("PUT V5 NODE %08X  Left %08X  Right %08X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U32(raw.left),
			LE_U32(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


void PutNodes(const char *name, int do_gl, int do_v5, node_t *root)
{
	int struct_size = do_v5 ? (int)sizeof(raw_v5_node_t) : (int)sizeof(raw_node_t);

	// this can be bigger than the actual size, but never smaller
	int max_size = (num_nodes + 1) * struct_size;

	Lump_c *lump = CreateLevelLump(name, max_size);

	node_cur_index = 0;

	if (root)
	{
		if (do_v5)
			PutOneNode_V5(root, lump);
		else
			PutOneNode(root, lump);
	}

	if (node_cur_index != num_nodes)
		BugError("PutNodes miscounted (%d != %d)",
				node_cur_index, num_nodes);

	if (!do_v5 && node_cur_index > 32767)
		MarkHardFailure(LIMIT_NODES);
}


/* ----- ZDBSP format writing --------------------------- */

static const u8_t *lev_ZD_magic = (u8_t *) "ZNOD";

void PutZVertices(void)
{
	int count, i;

	u32_t orgverts = LE_U32(num_normal_vert);
	u32_t newverts = LE_U32(num_gl_vert);

	ZLibAppendLump(&orgverts, 4);
	ZLibAppendLump(&newverts, 4);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_v2_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if (! (vert->index & IS_GL_VERTEX))
			continue;

		raw.x = LE_S32((int)(vert->x * 65536.0));
		raw.y = LE_S32((int)(vert->y * 65536.0));

		ZLibAppendLump(&raw, sizeof(raw));

		count++;
	}

	if (count != num_gl_vert)
		BugError("PutZVertices miscounted (%d != %d)",
				count, num_gl_vert);
}


void PutZSubsecs(void)
{
	int i;
	int count;
	u32_t raw_num = LE_U32(num_subsecs);

	int cur_seg_index = 0;

	ZLibAppendLump(&raw_num, 4);

	for (i=0 ; i < num_subsecs ; i++)
	{
		subsec_t *sub = subsecs[i];
		seg_t *seg;

		raw_num = LE_U32(sub->seg_count);

		ZLibAppendLump(&raw_num, 4);

		// sanity check the seg index values
		count = 0;
		for (seg = sub->seg_list ; seg ; seg = seg->next, cur_seg_index++)
		{
			// ignore minisegs and degenerate segs
			if (! seg->linedef || seg->degenerate)
				continue;

			if (cur_seg_index != seg->index)
				BugError("PutZSubsecs: seg index mismatch in sub %d (%d != %d)\n",
						i, cur_seg_index, seg->index);

			count++;
		}

		if (count != sub->seg_count)
			BugError("PutZSubsecs: miscounted segs in sub %d (%d != %d)\n",
					i, count, sub->seg_count);
	}

	if (cur_seg_index != num_complete_seg)
		BugError("PutZSubsecs miscounted segs (%d != %d)",
				cur_seg_index, num_complete_seg);
}


void PutZSegs(void)
{
	int i, count;
	u32_t raw_num = LE_U32(num_complete_seg);

	ZLibAppendLump(&raw_num, 4);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		seg_t *seg = segs[i];

		// ignore minisegs and degenerate segs
		if (! seg->linedef || seg->degenerate)
			continue;

		if (count != seg->index)
			BugError("PutZSegs: seg index mismatch (%d != %d)\n",
					count, seg->index);

		{
			u32_t v1 = LE_U32(VertexIndex32BitV5(seg->start));
			u32_t v2 = LE_U32(VertexIndex32BitV5(seg->end));

			u16_t line = LE_U16(seg->linedef->index);
			u8_t  side = seg->side;

			ZLibAppendLump(&v1,   4);
			ZLibAppendLump(&v2,   4);
			ZLibAppendLump(&line, 2);
			ZLibAppendLump(&side, 1);
		}

		count++;
	}

	if (count != num_complete_seg)
		BugError("PutZSegs miscounted (%d != %d)",
				count, num_complete_seg);
}


static void PutOneZNode(node_t *node)
{
	raw_v5_node_t raw;

	if (node->r.node)
		PutOneZNode(node->r.node);

	if (node->l.node)
		PutOneZNode(node->l.node);

	node->index = node_cur_index++;

	raw.x  = LE_S16(node->x);
	raw.y  = LE_S16(node->y);
	raw.dx = LE_S16(node->dx / (node->too_long ? 2 : 1));
	raw.dy = LE_S16(node->dy / (node->too_long ? 2 : 1));

	ZLibAppendLump(&raw.x,  2);
	ZLibAppendLump(&raw.y,  2);
	ZLibAppendLump(&raw.dx, 2);
	ZLibAppendLump(&raw.dy, 2);

	raw.b1.minx = LE_S16(node->r.bounds.minx);
	raw.b1.miny = LE_S16(node->r.bounds.miny);
	raw.b1.maxx = LE_S16(node->r.bounds.maxx);
	raw.b1.maxy = LE_S16(node->r.bounds.maxy);

	raw.b2.minx = LE_S16(node->l.bounds.minx);
	raw.b2.miny = LE_S16(node->l.bounds.miny);
	raw.b2.maxx = LE_S16(node->l.bounds.maxx);
	raw.b2.maxy = LE_S16(node->l.bounds.maxy);

	ZLibAppendLump(&raw.b1, sizeof(raw.b1));
	ZLibAppendLump(&raw.b2, sizeof(raw.b2));

	if (node->r.node)
		raw.right = LE_U32(node->r.node->index);
	else if (node->r.subsec)
		raw.right = LE_U32(node->r.subsec->index | 0x80000000U);
	else
		BugError("Bad right child in V5 node %d", node->index);

	if (node->l.node)
		raw.left = LE_U32(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U32(node->l.subsec->index | 0x80000000U);
	else
		BugError("Bad left child in V5 node %d", node->index);

	ZLibAppendLump(&raw.right, 4);
	ZLibAppendLump(&raw.left,  4);

# if DEBUG_BSP
	DebugPrintf("PUT Z NODE %08X  Left %08X  Right %08X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U32(raw.left),
			LE_U32(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


void PutZNodes(node_t *root)
{
	u32_t raw_num = LE_U32(num_nodes);

	ZLibAppendLump(&raw_num, 4);

	node_cur_index = 0;

	if (root)
		PutOneZNode(root);

	if (node_cur_index != num_nodes)
		BugError("PutZNodes miscounted (%d != %d)",
				node_cur_index, num_nodes);
}


void SaveZDFormat(node_t *root_node)
{
	// leave SEGS and SSECTORS empty
	CreateLevelLump("SEGS")->Finish();
	CreateLevelLump("SSECTORS")->Finish();

	// TODO : compute worst-case size of the lump (plus a healthy margin)

	Lump_c *lump = CreateLevelLump("NODES");

	lump->Write(lev_ZD_magic, 4);

	ZLibBeginLump(lump);

	PutZVertices();
	PutZSubsecs();
	PutZSegs();
	PutZNodes(root_node);

	ZLibFinishLump();
}


/* ----- whole-level routines --------------------------- */

void LoadLevel()
{
	Lump_c *LEV = edit_wad->GetLump(lev_current_start);

	lev_current_name = LEV->Name();

	GB_PrintMsg("Building nodes on %s\n", lev_current_name);

	// -JL- Identify Hexen mode by presence of BEHAVIOR lump
	lev_doing_hexen = (FindLevelLump("BEHAVIOR") != NULL);

	GetVertices();
	GetSectors();
	GetSidedefs();

	if (lev_doing_hexen)
	{
		GetLinedefsHexen();
		GetThingsHexen();
	}
	else
	{
		GetLinedefs();
		GetThings();
	}

	PrintVerbose("Loaded %d vertices, %d sectors, %d sides, %d lines, %d things\n",
			num_vertices, num_sectors, num_sidedefs, num_linedefs, num_things);

	// always prune vertices at end of lump, otherwise all the
	// unused vertices from seg splits would keep accumulating.
	PruneVerticesAtEnd();

	DetectOverlappingVertices();
	DetectOverlappingLines();

	CalculateWallTips();

	if (lev_doing_hexen)
	{
		// -JL- Find sectors containing polyobjs
		DetectPolyobjSectors();
	}
}


void FreeLevel(void)
{
	FreeVertices();
	FreeSidedefs();
	FreeLinedefs();
	FreeSectors();
	FreeThings();
	FreeSegs();
	FreeSubsecs();
	FreeNodes();
	FreeWallTips();
}


static u32_t CalcGLChecksum(void)
{
	u32_t crc;

	Adler32_Begin(&crc);

	Lump_c *lump = FindLevelLump("VERTEXES");

	if (lump && lump->Length() > 0)
	{
		u8_t *data = new u8_t[lump->Length()];

		if (! lump->Seek() ||
		    ! lump->Read(data, lump->Length()))
			FatalError("Error reading vertices (for checksum).\n");

		Adler32_AddBlock(&crc, data, lump->Length());
		delete[] data;
	}

	lump = FindLevelLump("LINEDEFS");

	if (lump && lump->Length() > 0)
	{
		u8_t *data = new u8_t[lump->Length()];

		if (! lump->Seek() ||
		    ! lump->Read(data, lump->Length()))
			FatalError("Error reading linedefs (for checksum).\n");

		Adler32_AddBlock(&crc, data, lump->Length());
		delete[] data;
	}

	Adler32_Finish(&crc);

	return crc;
}


static const char *CalcOptionsString()
{
	static char buffer[256];

	// FIXME FOR AJ-BSP / EUREKA
	sprintf(buffer, "--factor %d", cur_info->factor);

	if (cur_info->fast) strcat(buffer, "--fast");

	return buffer;
}


void UpdateGLMarker(Lump_c *marker)
{
	// this is very conservative, around 4 times the actual size
	const int max_size = 512;

	// we *must* compute the checksum BEFORE (re)creating the lump
	// [ otherwise we write data into the wrong part of the file ]
	u32_t crc = CalcGLChecksum();

	edit_wad->RecreateLump(marker, max_size);

	if (lev_long_name)
	{
		marker->Printf("LEVEL=%s\n", lev_current_name);
	}

	marker->Printf("BUILDER=%s\n", "Eureka " EUREKA_VERSION);

	marker->Printf("OPTIONS=%s\n", CalcOptionsString());

	char *time_str = UtilTimeString();

	if (time_str)
	{
		marker->Printf("TIME=%s\n", time_str);
		StringFree(time_str);
	}

	marker->Printf("CHECKSUM=0x%08x\n", crc);

	marker->Finish();
}


static void AddMissingLump(const char *name, const char *after)
{
	if (edit_wad->LevelLookupLump(lev_current_idx, name) >= 0)
		return;

	short exist = edit_wad->LevelLookupLump(lev_current_idx, after);

	// FIXME : if this happens, the level structure is very broken, so.... do what?????
	if (exist < 0)
	{
		exist = edit_wad->LevelLastLump(lev_current_idx);
		// PrintMsg ???
	}

	edit_wad->InsertPoint(exist + 1);

	edit_wad->AddLump(name)->Finish();
}


void SaveLevel(node_t *root_node)
{
	edit_wad->BeginWrite();

	// remove any existing GL-Nodes
	edit_wad->RemoveGLNodes(lev_current_idx);

	// ensure all necessary level lumps are present
	AddMissingLump("SEGS",     "VERTEXES");
	AddMissingLump("SSECTORS", "SEGS");
	AddMissingLump("NODES",    "SSECTORS");
	AddMissingLump("REJECT",   "SECTORS");
	AddMissingLump("BLOCKMAP", "REJECT");


	lev_force_v5   = cur_info->force_v5;
	lev_force_xnod = cur_info->force_xnod;

	Lump_c * gl_marker = NULL;


	// Note: RoundOffBspTree will convert the GL vertices in segs to
	// their normal counterparts (pointer change: use normal_dup).

	// check for overflows...

	// FIXME : verify this is correct for XNOD format

	if (num_normal_vert > 32767 || num_gl_vert > 32767)
	{
		if (cur_info->gl_nodes && !cur_info->force_v5)
		{
			lev_force_v5 = true;
			MarkV5Switch(LIMIT_VERTEXES | LIMIT_GL_SEGS);
		}

		if (! cur_info->force_xnod)
		{
			lev_force_xnod = true;
			MarkZDSwitch();
		}
	}

	if (num_segs > 65534)
	{
		if (cur_info->gl_nodes && !cur_info->force_v5)
		{
			lev_force_v5 = true;
			MarkV5Switch(LIMIT_GL_SSECT | LIMIT_GL_SEGS);
		}

		if (! cur_info->force_xnod)
		{
			lev_force_xnod = true;
			MarkZDSwitch();
		}
	}

	if (num_nodes > 32767)
	{
		if (cur_info->gl_nodes && !cur_info->force_v5)
		{
			lev_force_v5 = true;
			MarkV5Switch(LIMIT_GL_NODES);
		}

		if (! cur_info->force_xnod)
		{
			lev_force_xnod = true;
			MarkZDSwitch();
		}
	}


	/* --- GL Nodes --- */

	if (cur_info->gl_nodes)
	{
		// create empty marker now, flesh it out later
		gl_marker = CreateGLMarker();

		PutGLVertices(lev_force_v5);

		if (lev_force_v5)
			PutGLSegs_V5();
		else
			PutGLSegs();

		if (lev_force_v5)
			PutGLSubsecs_V5();
		else
			PutSubsecs("GL_SSECT", true);

		PutNodes("GL_NODES", true, lev_force_v5, root_node);

		// -JL- Add empty PVS lump
		CreateLevelLump("GL_PVS")->Finish();
	}

	/* --- Normal nodes --- */

	{
		// FIXME!!! necessary for XNOD/ZNOD ???
		RoundOffBspTree(root_node);

		// FIXME!!! necessary for XNOD/ZNOD ???
		NormaliseBspTree(root_node);

		// FIXME : check if XNOD supplies its own vertices
		PutVertices("VERTEXES", false);

		ValidateSectors();
		ValidateSidedefs();
		ValidateLinedefs();

		if (lev_force_xnod)
		{
			SaveZDFormat(root_node);
		}
		else
		{
			PutSegs();
			PutSubsecs("SSECTORS", false);
			PutNodes("NODES", false, false, root_node);
		}

		// -JL- Don't touch blockmap and reject if not doing normal nodes
		PutBlockmap();
		PutReject();
	}

	// keyword support (v5.0 of the specs).
	// must be done *after* doing normal nodes, for proper checksum.
	if (gl_marker)
	{
		UpdateGLMarker(gl_marker);
	}

	edit_wad->EndWrite();
}


//----------------------------------------------------------------------


static Lump_c  *zout_lump;
static z_stream zout_stream;
static Bytef    zout_buffer[1024];


void ZLibBeginLump(Lump_c *lump)
{
	zout_lump = lump;

	zout_stream.zalloc = (alloc_func)0;
	zout_stream.zfree  = (free_func)0;
	zout_stream.opaque = (voidpf)0;

	if (Z_OK != deflateInit(&zout_stream, Z_DEFAULT_COMPRESSION))
		FatalError("Trouble setting up zlib compression");

	zout_stream.next_out  = zout_buffer;
	zout_stream.avail_out = sizeof(zout_buffer);
}


void ZLibAppendLump(const void *data, int length)
{
	// ASSERT(zout_lump)
	// ASSERT(length > 0)

	zout_stream.next_in  = (Bytef*)data;   // const override
	zout_stream.avail_in = length;

	while (zout_stream.avail_in > 0)
	{
		int err = deflate(&zout_stream, Z_NO_FLUSH);

		if (err != Z_OK)
			FatalError("Trouble compressing %d bytes (zlib)\n", length);

		if (zout_stream.avail_out == 0)
		{
			zout_lump->Write(zout_buffer, sizeof(zout_buffer));

			zout_stream.next_out  = zout_buffer;
			zout_stream.avail_out = sizeof(zout_buffer);
		}
	}
}


void ZLibFinishLump(void)
{
	int left_over;

	// ASSERT(zout_stream.avail_out > 0)

	zout_stream.next_in  = Z_NULL;
	zout_stream.avail_in = 0;

	for (;;)
	{
		int err = deflate(&zout_stream, Z_FINISH);

		if (err == Z_STREAM_END)
			break;

		if (err != Z_OK)
			FatalError("Trouble finishing compression (zlib)\n");

		if (zout_stream.avail_out == 0)
		{
			zout_lump->Write(zout_buffer, sizeof(zout_buffer));

			zout_stream.next_out  = zout_buffer;
			zout_stream.avail_out = sizeof(zout_buffer);
		}
	}

	left_over = sizeof(zout_buffer) - zout_stream.avail_out;

	if (left_over > 0)
		zout_lump->Write(zout_buffer, left_over);

	deflateEnd(&zout_stream);
	zout_lump = NULL;
}


/* ---------------------------------------------------------------- */


//
// Mark failure routines
//
void MarkSoftFailure(int soft)
{
//TODO  wad.current_level->lev_info->soft_limit |= soft;
}

void MarkHardFailure(int hard)
{
//TODO  wad.current_level->lev_info->hard_limit |= hard;
}

void MarkV5Switch(int v5)
{
//TODO  wad.current_level->lev_info->v5_switch |= v5;
}

void MarkZDSwitch(void)
{
#if 0  // TODO
  level_t *lev = wad.current_level->lev_info;

  lev->v5_switch |= LIMIT_ZDBSP;

  lev->soft_limit &= ~ (LIMIT_VERTEXES);
  lev->hard_limit &= ~ (LIMIT_VERTEXES);
#endif
}


void ReportOneOverflow(const Lump_c *lump, int limit, bool hard)
{
	const char *msg = hard ? "overflowed the absolute limit" :
		"overflowed the original limit";

	PrintMsg("%-8s: ", lump->Name());

	switch (limit)
	{
		case LIMIT_VERTEXES: PrintMsg("Number of Vertices %s.\n", msg); break;
		case LIMIT_SECTORS:  PrintMsg("Number of Sectors %s.\n", msg); break;
		case LIMIT_SIDEDEFS: PrintMsg("Number of Sidedefs %s\n", msg); break;
		case LIMIT_LINEDEFS: PrintMsg("Number of Linedefs %s\n", msg); break;

		case LIMIT_SEGS:     PrintMsg("Number of Segs %s.\n", msg); break;
		case LIMIT_SSECTORS: PrintMsg("Number of Subsectors %s.\n", msg); break;
		case LIMIT_NODES:    PrintMsg("Number of Nodes %s.\n", msg); break;

		case LIMIT_GL_VERT:  PrintMsg("Number of GL vertices %s.\n", msg); break;
		case LIMIT_GL_SEGS:  PrintMsg("Number of GL segs %s.\n", msg); break;
		case LIMIT_GL_SSECT: PrintMsg("Number of GL subsectors %s.\n", msg); break;
		case LIMIT_GL_NODES: PrintMsg("Number of GL nodes %s.\n", msg); break;

		case LIMIT_BAD_SIDE:   PrintMsg("One or more linedefs has a bad sidedef.\n"); break;
		case LIMIT_BMAP_TRUNC: PrintMsg("Blockmap area was too big - truncated.\n"); break;
		case LIMIT_BLOCKMAP:   PrintMsg("Blockmap lump %s.\n", msg); break;

		default:
			BugError("UNKNOWN LIMIT BIT: 0x%06x", limit);
	}
}


void ReportOverflows(bool hard)
{
#if 0
	lump_t *cur;

	if (hard)
	{
		PrintMsg(
				"ERRORS.  The following levels failed to be built, and won't\n"
				"work in any Doom port (and may even crash it).\n\n"
				);
	}
	else  // soft
	{
		PrintMsg(
				"POTENTIAL FAILURES.  The following levels should work in a\n"
				"modern Doom port, but may fail (or even crash) in older ports.\n\n"
				);
	}

	for (cur=wad.dir_head ; cur ; cur=cur->next)
	{
		level_t *lev = cur->lev_info;

		int limits, one_lim;

		if (! (lev && ! (lev->flags & LEVEL_IS_GL)))
			continue;

		limits = hard ? lev->hard_limit : lev->soft_limit;

		if (limits == 0)
			continue;

		for (one_lim = (1<<20) ; one_lim ; one_lim >>= 1)
		{
			if (limits & one_lim)
				ReportOneOverflow(cur, one_lim, hard);
		}
	}
#endif
}


void ReportV5Switches(void)
{
#if 0
	lump_t *cur;

	int saw_zdbsp = false;

	PrintMsg(
			"V5 FORMAT UPGRADES.  The following levels require a Doom port\n"
			"which supports V5 GL-Nodes, otherwise they will fail (or crash).\n\n"
			);

	for (cur=wad.dir_head ; cur ; cur=cur->next)
	{
		level_t *lev = cur->lev_info;

		if (! (lev && ! (lev->flags & LEVEL_IS_GL)))
			continue;

		if (lev->v5_switch == 0)
			continue;

		if ((lev->v5_switch & LIMIT_ZDBSP) && ! saw_zdbsp)
		{
			PrintMsg("ZDBSP FORMAT has also been used for regular nodes.\n\n");
			saw_zdbsp = true;
		}

		if (lev->v5_switch & LIMIT_VERTEXES)
		{
			PrintMsg("%-8s: Number of Vertices overflowed the limit.\n", cur->name);
		}

		if (lev->v5_switch & LIMIT_GL_SSECT)
		{
			PrintMsg("%-8s: Number of GL segs overflowed the limit.\n", cur->name);
		}
	}
#endif
}


void ReportFailedLevels(void)
{
	PrintMsg("*** Problem Report ***\n\n");

#if 0
	lump_t *cur;
	int lev_count = 0;

	int fail_soft = 0;
	int fail_hard = 0;
	int fail_v5   = 0;

	bool need_spacer = false;

	for (cur=wad.dir_head ; cur ; cur=cur->next)
	{
		if (! (cur->lev_info && ! (cur->lev_info->flags & LEVEL_IS_GL)))
			continue;

		lev_count++;

		if (cur->lev_info->soft_limit != 0) fail_soft++;
		if (cur->lev_info->hard_limit != 0) fail_hard++;
		if (cur->lev_info->v5_switch  != 0) fail_v5++;
	}

	PrintMsg("\n");

	if (fail_soft + fail_hard + fail_v5 == 0)
	{
		PrintMsg("All levels were built successfully.\n");
		return;
	}

	if (fail_soft > 0)
	{
		ReportOverflows(false);
		need_spacer = true;
	}

	if (fail_v5 > 0)
	{
		if (need_spacer)
			PrintMsg("\n");

		ReportV5Switches();
		need_spacer = true;
	}

	if (fail_hard > 0)
	{
		if (need_spacer)
			PrintMsg("\n");

		ReportOverflows(true);
	}

#endif
	PrintMsg("\nEnd of problem report.\n");
}


Lump_c * FindLevelLump(const char *name)
{
	short idx = edit_wad->LevelLookupLump(lev_current_idx, name);

	if (idx < 0)
		return NULL;

	return edit_wad->GetLump(idx);
}


Lump_c * CreateLevelLump(const char *name, int max_size)
{
	// look for existing one
	Lump_c *lump = FindLevelLump(name);

	if (lump)
	{
		edit_wad->RecreateLump(lump, max_size);
	}
	else
	{
		short last_idx = edit_wad->LevelLastLump(lev_current_idx);

		edit_wad->InsertPoint(last_idx + 1);

		lump = edit_wad->AddLump(name, max_size);
	}

	return lump;
}


Lump_c * CreateGLMarker()
{
	char name_buf[64];

	if (strlen(lev_current_name) <= 5)
	{
		sprintf(name_buf, "GL_%s", lev_current_name);

		lev_long_name = false;
	}
	else
	{
		// support for level names longer than 5 letters
		strcpy(name_buf, "GL_LEVEL");

		lev_long_name = true;
	}

	short last_idx = edit_wad->LevelLastLump(lev_current_idx);

	edit_wad->InsertPoint(last_idx + 1);

	Lump_c *marker = edit_wad->AddLump(name_buf);

	marker->Finish();

	return marker;
}


//------------------------------------------------------------------------
// MAIN STUFF
//------------------------------------------------------------------------


nodebuildinfo_t * cur_info = NULL;


/* ----- build nodes for a single level --------------------------- */

build_result_e BuildNodesForLevel(nodebuildinfo_t *info, short lev_idx)
{
	cur_info = info;

	superblock_t *seg_list;
	bbox_t seg_bbox;

	node_t *root_node;
	subsec_t *root_sub;

	build_result_e ret;

	if (cur_info->cancelled)
		return BUILD_Cancelled;

	lev_current_idx   = lev_idx;
	lev_current_start = edit_wad->LevelHeader(lev_idx);

	LoadLevel();

	InitBlockmap();

	// create initial segs
	seg_list = CreateSegs();

	FindLimits(seg_list, &seg_bbox);

	// recursively create nodes
	ret = BuildNodes(seg_list, &root_node, &root_sub, 0, &seg_bbox);

	FreeSuper(seg_list);

	if (ret == BUILD_OK)
	{
		ClockwiseBspTree(root_node);

		PrintVerbose("Built %d NODES, %d SSECTORS, %d SEGS, %d VERTEXES\n",
				num_nodes, num_subsecs, num_segs, num_normal_vert + num_gl_vert);

		if (root_node)
			PrintVerbose("Heights of left and right subtrees = (%d,%d)\n",
					ComputeBspHeight(root_node->r.node),
					ComputeBspHeight(root_node->l.node));

		SaveLevel(root_node);
	}

	FreeLevel();
	FreeQuickAllocCuts();
	FreeQuickAllocSupers();

	return ret;
}

}  // namespace ajbsp


build_result_e AJBSP_BuildLevel(nodebuildinfo_t *info, short lev_idx)
{
	return ajbsp::BuildNodesForLevel(info, lev_idx);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
