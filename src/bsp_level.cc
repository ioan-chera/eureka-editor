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

#include "Errors.h"
#include "Instance.h"
#include "main.h"
#include "bsp.h"

#include "w_rawdef.h"
#include "w_wad.h"

#include <zlib.h>


namespace ajbsp
{

#define DEBUG_BLOCKMAP  0
#define DEBUG_REJECT    0
#define DEBUG_LOAD      0
#define DEBUG_BSP       0


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

		// swap end points
		tmp=x1;  x1=x2;  x2=tmp;
		tmp=y1;  y1=y2;  y2=tmp;
	}

	// linedef touches block
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
	gLog.debugPrintf("Block %d has line %d\n", blk_num, line_index);
# endif

	if (blk_num < 0 || blk_num >= block_count)
		BugError("BlockAdd: bad block number %d\n", blk_num);

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
	cur[BK_XOR] = static_cast<u16_t>(((cur[BK_XOR] << 4) | (cur[BK_XOR] >> 12)) ^ line_index);

	cur[BK_FIRST + cur[BK_NUM]] = LE_U16(line_index);
	cur[BK_NUM]++;
}


static void BlockAddLine(int line_index, const Document &doc)
{
	const LineDef *L = doc.linedefs[line_index];

	int x1 = (int) L->Start(doc)->x();
	int y1 = (int) L->Start(doc)->y();
	int x2 = (int) L->End(doc)->x();
	int y2 = (int) L->End(doc)->y();

	int bx1 = (std::min(x1,x2) - block_x) / 128;
	int by1 = (std::min(y1,y2) - block_y) / 128;
	int bx2 = (std::max(x1,x2) - block_x) / 128;
	int by2 = (std::max(y1,y2) - block_y) / 128;

	int bx, by;

# if DEBUG_BLOCKMAP
	gLog.debugPrintf("BlockAddLine: %d (%d,%d) -> (%d,%d)\n", line_index,
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


static void CreateBlockmap(const Document &doc)
{
	block_lines = (u16_t **) UtilCalloc(block_count * sizeof(u16_t *));

	for (int i=0 ; i < doc.numLinedefs() ; i++)
	{
		// ignore zero-length lines
		if (doc.linedefs[i]->IsZeroLength(doc))
			continue;

		BlockAddLine(i, doc);
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
		block_dups[i] = static_cast<u16_t>(i);

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
			block_ptrs[blk_num] = static_cast<u16_t>(4 + block_count);
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
			block_ptrs[blk_num] = static_cast<u16_t>(cur_offset);
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

		block_ptrs[blk_num] = static_cast<u16_t>(cur_offset);

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
	gLog.debugPrintf("Blockmap: Last ptr = %d  duplicates = %d\n",
			cur_offset, dup_count);
# endif

	block_compression = (orig_size - new_size) * 100 / orig_size;

	// there's a tiny chance of new_size > orig_size
	if (block_compression < 0)
		block_compression = 0;
}

static Lump_c *CreateLevelLump(const Instance &inst, const char *name);

static void WriteBlockmap(const Instance &inst)
{
	int i;

	Lump_c *lump = CreateLevelLump(inst, "BLOCKMAP");

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
			BugError("WriteBlockmap: offset %d not set.\n", i);

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


static void FindBlockmapLimits(bbox_t *bbox, const Document &doc)
{
	int mid_x = 0;
	int mid_y = 0;

	bbox->minx = bbox->miny = SHRT_MAX;
	bbox->maxx = bbox->maxy = SHRT_MIN;

	for (int i=0 ; i < doc.numLinedefs() ; i++)
	{
		const LineDef *L = doc.linedefs[i];

		if (! L->IsZeroLength(doc))
		{
			double x1 = L->Start(doc)->x();
			double y1 = L->Start(doc)->y();
			double x2 = L->End(doc)->x();
			double y2 = L->End(doc)->y();

			int lx = (int)floor(std::min(x1, x2));
			int ly = (int)floor(std::min(y1, y2));
			int hx = (int)ceil(std::max(x1, x2));
			int hy = (int)ceil(std::max(y1, y2));

			if (lx < bbox->minx) bbox->minx = lx;
			if (ly < bbox->miny) bbox->miny = ly;
			if (hx > bbox->maxx) bbox->maxx = hx;
			if (hy > bbox->maxy) bbox->maxy = hy;

			// compute middle of cluster (roughly, so we don't overflow)
			mid_x += (lx + hx) / 32;
			mid_y += (ly + hy) / 32;
		}
	}

	if (doc.numLinedefs() > 0)
	{
		block_mid_x = (mid_x / doc.numLinedefs()) * 16;
		block_mid_y = (mid_y / doc.numLinedefs()) * 16;
	}

# if DEBUG_BLOCKMAP
	gLog.debugPrintf("Blockmap lines centered at (%d,%d)\n", block_mid_x, block_mid_y);
# endif
}

//
// compute blockmap origin & size (the block_x/y/w/h variables)
// based on the set of loaded linedefs.
//
static void InitBlockmap(const Document &doc)
{
	bbox_t map_bbox;

	// find limits of linedefs, and store as map limits
	FindBlockmapLimits(&map_bbox, doc);

	PrintDetail("Map goes from (%d,%d) to (%d,%d)\n",
			map_bbox.minx, map_bbox.miny, map_bbox.maxx, map_bbox.maxy);

	block_x = map_bbox.minx - (map_bbox.minx & 0x7);
	block_y = map_bbox.miny - (map_bbox.miny & 0x7);

	block_w = ((map_bbox.maxx - block_x) / 128) + 1;
	block_h = ((map_bbox.maxy - block_y) / 128) + 1;

	block_count = block_w * block_h;
}

//
// build the blockmap and write the data into the BLOCKMAP lump
//
static void PutBlockmap(const Instance &inst)
{
	if (! cur_info->do_blockmap || inst.level.numLinedefs() == 0)
	{
		// just create an empty blockmap lump
		CreateLevelLump(inst, "BLOCKMAP");
		return;
	}

	block_overflowed = false;

	// initial phase: create internal blockmap containing the index of
	// all lines in each block.

	CreateBlockmap(inst.level);

	// -AJA- second phase: compress the blockmap.  We do this by sorting
	//       the blocks, which is a typical way to detect duplicates in
	//       a large list.  This also detects BLOCKMAP overflow.

	CompressBlockmap();

	// final phase: write it out in the correct format

	if (block_overflowed)
	{
		// leave an empty blockmap lump
		CreateLevelLump(inst, "BLOCKMAP");

		Warning(inst, "Blockmap overflowed (lump will be empty)\n");
	}
	else
	{
		WriteBlockmap(inst);

		PrintDetail("Completed blockmap, size %dx%d (compression: %d%%)\n",
				block_w, block_h, block_compression);
	}

	FreeBlockmap();
}


//------------------------------------------------------------------------
// REJECT : Generate the reject table
//------------------------------------------------------------------------


static u8_t *rej_matrix;
static int   rej_total_size;	// in bytes

static std::vector<int> rej_sector_groups;


//
// Allocate the matrix, init sectors into individual groups.
//
static void Reject_Init(const Document &doc)
{
	rej_total_size = (doc.numSectors() * doc.numSectors() + 7) / 8;

	rej_matrix = new u8_t[rej_total_size];
	memset(rej_matrix, 0, rej_total_size);

	rej_sector_groups.resize(doc.numSectors());

	for (int i=0 ; i < doc.numSectors() ; i++)
	{
		rej_sector_groups[i] = i;
	}
}


static void Reject_Free()
{
	delete[] rej_matrix;
	rej_matrix = NULL;

	rej_sector_groups.clear();
}


//
// Algorithm: Initially all sectors are in individual groups.
// Now we scan the linedef list.  For each two-sectored line,
// merge the two sector groups into one.  That's it!
//
static void Reject_GroupSectors(const Document &doc)
{
	for(const LineDef *L : doc.linedefs)
	{
		if (L->right < 0 || L->left < 0)
			continue;

		int sec1 = L->Right(doc)->sector;
		int sec2 = L->Left(doc) ->sector;

		if (sec1 < 0 || sec2 < 0 || sec1 == sec2)
			continue;

		// already in the same group ?
		int group1 = rej_sector_groups[sec1];
		int group2 = rej_sector_groups[sec2];

		if (group1 == group2)
			continue;

		// prefer the group numbers to become lower
		if (group1 > group2)
			std::swap(group1, group2);

		// merge the groups
		for (int s = 0 ; s < doc.numSectors() ; s++)
			if (rej_sector_groups[s] == group2)
				rej_sector_groups[s] =  group1;
	}
}


#if DEBUG_REJECT
static void Reject_DebugGroups()
{
	// Note: this routine is destructive to the group numbers

	for (int i=0 ; i < doc.numSectors(); i++)
	{
		int group = rej_sector_groups[i];
		int count = 0;

		if (group < 0)
			continue;

		for (int k = i ; k < doc.numSectors() ; k++)
		{
			if (rej_sector_groups[k] == group)
			{
				rej_sector_groups[k] = -1;
				count++;
			}
		}

		gLog.debugPrintf("Group %4d : Sectors %d\n", group, count);
	}
}
#endif


static void Reject_ProcessSectors(const Document &doc)
{
	for (int view=0 ; view < doc.numSectors() ; view++)
	{
		for (int target=0 ; target < view ; target++)
		{
			if (rej_sector_groups[view] == rej_sector_groups[target])
				continue;

			int p1 = view * doc.numSectors() + target;
			int p2 = target * doc.numSectors() + view;

			// must do both directions at same time
			rej_matrix[p1 >> 3] |= (1 << (p1 & 7));
			rej_matrix[p2 >> 3] |= (1 << (p2 & 7));
		}
	}
}


static void Reject_WriteLump(const Instance &inst)
{
	Lump_c *lump = CreateLevelLump(inst, "REJECT");

	lump->Write(rej_matrix, rej_total_size);
}


//
// build the reject table and write it into the REJECT lump
//
// For now we only do very basic reject processing, limited to
// determining all isolated groups of sectors (islands that are
// surrounded by void space).
//
static void PutReject(const Instance &inst)
{
	if (! cur_info->do_reject || inst.level.numSectors() == 0)
	{
		// just create an empty reject lump
		CreateLevelLump(inst, "REJECT");
		return;
	}

	Reject_Init(inst.level);
	Reject_GroupSectors(inst.level);
	Reject_ProcessSectors(inst.level);

# if DEBUG_REJECT
	Reject_DebugGroups();
# endif

	Reject_WriteLump(inst);
	Reject_Free();

	PrintDetail("Added simple reject lump\n");
}


//------------------------------------------------------------------------
// LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------


// Note: ZDoom format support based on code (C) 2002,2003 Randy Heit


#define ALLOC_BLKNUM  1024


// per-level variables

static SString lev_current_name;

int lev_current_idx;
int lev_current_start;

int lev_overflows;


std::vector<vertex_t *>  lev_vertices;
std::vector<seg_t *>     lev_segs;
std::vector<subsec_t *>  lev_subsecs;
std::vector<node_t *>    lev_nodes;
std::vector<walltip_t *> lev_walltips;

int num_old_vert = 0;
int num_new_vert = 0;
int num_real_lines = 0;


/* ----- allocation routines ---------------------------- */

vertex_t *NewVertex()
{
	vertex_t *V = (vertex_t *) UtilCalloc(sizeof(vertex_t));
	lev_vertices.push_back(V);
	return V;
}

seg_t *NewSeg()
{
	seg_t *S = (seg_t *) UtilCalloc(sizeof(seg_t));
	lev_segs.push_back(S);
	return S;
}

subsec_t *NewSubsec()
{
	subsec_t *S = (subsec_t *) UtilCalloc(sizeof(subsec_t));
	lev_subsecs.push_back(S);
	return S;
}

node_t *NewNode()
{
	node_t *N = (node_t *) UtilCalloc(sizeof(node_t));
	lev_nodes.push_back(N);
	return N;
}

walltip_t *NewWallTip()
{
	walltip_t *WT = (walltip_t *) UtilCalloc(sizeof(walltip_t));
	lev_walltips.push_back(WT);
	return WT;
}


/* ----- free routines ---------------------------- */

void FreeVertices()
{
	for (unsigned int i = 0 ; i < lev_vertices.size() ; i++)
		UtilFree((void *) lev_vertices[i]);

	lev_vertices.clear();
}

void FreeSegs()
{
	for (unsigned int i = 0 ; i < lev_segs.size() ; i++)
		UtilFree((void *) lev_segs[i]);

	lev_segs.clear();
}

void FreeSubsecs()
{
	for (unsigned int i = 0 ; i < lev_subsecs.size() ; i++)
		UtilFree((void *) lev_subsecs[i]);

	lev_subsecs.clear();
}

void FreeNodes()
{
	for (unsigned int i = 0 ; i < lev_nodes.size() ; i++)
		UtilFree((void *) lev_nodes[i]);

	lev_nodes.clear();
}

void FreeWallTips()
{
	for (unsigned int i = 0 ; i < lev_walltips.size() ; i++)
		UtilFree((void *) lev_walltips[i]);

	lev_walltips.clear();
}


/* ----- reading routines ------------------------------ */

static void GetVertices(const Document &doc)
{
	for (int i = 0 ; i < doc.numVertices() ; i++)
	{
		vertex_t *vert = NewVertex();

		vert->x = doc.vertices[i]->x();
		vert->y = doc.vertices[i]->y();

		vert->index = i;
	}

	num_old_vert = num_vertices;
}


#if 0
static inline SideDef *SafeLookupSidedef(u16_t num)
{
	if (num == 0xFFFF)
		return NULL;

	if ((int)num >= doc.numSidedefs() && (s16_t)(num) < 0)
		return NULL;

	return SideDefs[num];
}
#endif


static inline int VanillaSegDist(const seg_t *seg, const Document &doc)
{
	const LineDef *L = doc.linedefs[seg->linedef];

	double lx = seg->side ? L->End(doc)->x() : L->Start(doc)->x();
	double ly = seg->side ? L->End(doc)->y() : L->Start(doc)->y();

	// use the "true" starting coord (as stored in the wad)
	double sx = round(seg->start->x);
	double sy = round(seg->start->y);

	return (int) floor(hypot(sx - lx, sy - ly) + 0.5);
}

static inline int VanillaSegAngle(const seg_t *seg)
{
	// compute the "true" delta
	double dx = round(seg->end->x) - round(seg->start->x);
	double dy = round(seg->end->y) - round(seg->start->y);

	double angle = UtilComputeAngle(dx, dy);

	if (angle < 0)
		angle += 360.0;

	int result = (int) floor(angle * 65536.0 / 360.0 + 0.5);

	return (result & 0xFFFF);
}


/* ----- writing routines ------------------------------ */

static const u8_t *lev_v2_magic = (u8_t *) "gNd2";
static const u8_t *lev_v5_magic = (u8_t *) "gNd5";


void MarkOverflow(int flags)
{
	// flags are ignored

	lev_overflows++;
}


static void PutVertices(const Instance &inst, const char *name, int do_gl)
{
	int count, i;

	Lump_c *lump = CreateLevelLump(inst, name);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if ((do_gl ? 1 : 0) != (vert->is_new ? 1 : 0))
		{
			continue;
		}

		raw.x = LE_S16(iround(vert->x));
		raw.y = LE_S16(iround(vert->y));

		lump->Write(&raw, sizeof(raw));

		count++;
	}

	if (count != (do_gl ? num_new_vert : num_old_vert))
		BugError("PutVertices miscounted (%d != %d)\n", count,
				do_gl ? num_new_vert : num_old_vert);

	if (! do_gl && count > 65534)
	{
		Failure(inst, "Number of vertices has overflowed.\n");
		MarkOverflow(LIMIT_VERTEXES);
	}
}


static void PutGLVertices(const Instance &inst, int do_v5)
{
	int count, i;

	Lump_c *lump = CreateLevelLump(inst, "GL_VERT");

	if (do_v5)
		lump->Write(lev_v5_magic, 4);
	else
		lump->Write(lev_v2_magic, 4);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_v2_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if (! vert->is_new)
			continue;

		raw.x = LE_S32((int)(vert->x * 65536.0));
		raw.y = LE_S32((int)(vert->y * 65536.0));

		lump->Write(&raw, sizeof(raw));

		count++;
	}

	if (count != num_new_vert)
		BugError("PutGLVertices miscounted (%d != %d)\n", count, num_new_vert);
}


static inline u16_t VertexIndex16Bit(const vertex_t *v)
{
	if (v->is_new)
		return (u16_t) (v->index | 0x8000U);

	return (u16_t) v->index;
}


static inline u32_t VertexIndex_V5(const vertex_t *v)
{
	if (v->is_new)
		return (u32_t) (v->index | 0x80000000U);

	return (u32_t) v->index;
}


static inline u32_t VertexIndex_XNOD(const vertex_t *v)
{
	if (v->is_new)
		return (u32_t) (num_old_vert + v->index);

	return (u32_t) v->index;
}


static void PutSegs(const Instance &inst)
{
	int i, count;

	Lump_c *lump = CreateLevelLump(inst, "SEGS");

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_seg_t raw;

		seg_t *seg = lev_segs[i];

		raw.start   = LE_U16(VertexIndex16Bit(seg->start));
		raw.end     = LE_U16(VertexIndex16Bit(seg->end));
		raw.angle   = LE_U16(VanillaSegAngle(seg));
		raw.linedef = LE_U16(seg->linedef);
		raw.flip    = LE_U16(seg->side);
		raw.dist    = LE_U16(VanillaSegDist(seg, inst.level));

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT SEG: %04X  Vert %04X->%04X  Line %04X %s  "
				"Angle %04X  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index,
				LE_U16(raw.start), LE_U16(raw.end), LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.angle),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_segs)
		BugError("PutSegs miscounted (%d != %d)\n", count, num_segs);

	if (count > 65534)
	{
		Failure(inst, "Number of segs has overflowed.\n");
		MarkOverflow(LIMIT_SEGS);
	}
}


static void PutGLSegs(const Instance &inst)
{
	int i, count;

	Lump_c *lump = CreateLevelLump(inst, "GL_SEGS");

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_gl_seg_t raw;

		seg_t *seg = lev_segs[i];

		raw.start = LE_U16(VertexIndex16Bit(seg->start));
		raw.end   = LE_U16(VertexIndex16Bit(seg->end));
		raw.side  = LE_U16(seg->side);

		if (seg->linedef >= 0)
			raw.linedef = LE_U16(seg->linedef);
		else
			raw.linedef = LE_U16(0xFFFF);

		if (seg->partner)
			raw.partner = LE_U16(seg->partner->index);
		else
			raw.partner = LE_U16(0xFFFF);

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT GL SEG: %04X  Line %04X %s  Partner %04X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_segs)
		BugError("PutGLSegs miscounted (%d != %d)\n", count, num_segs);

	if (count > 65534)
		BugError("PutGLSegs with %d (> 65534) segs\n", count);
}


static void PutGLSegs_V5(const Instance &inst)
{
	int i, count;

	Lump_c *lump = CreateLevelLump(inst, "GL_SEGS");

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		raw_v5_seg_t raw;

		seg_t *seg = lev_segs[i];

		raw.start = LE_U32(VertexIndex_V5(seg->start));
		raw.end   = LE_U32(VertexIndex_V5(seg->end));

		raw.side  = LE_U16(seg->side);

		if (seg->linedef >= 0)
			raw.linedef = LE_U16(seg->linedef);
		else
			raw.linedef = LE_U16(0xFFFF);

		if (seg->partner)
			raw.partner = LE_U32(seg->partner->index);
		else
			raw.partner = LE_U32(0xFFFFFFFF);

		lump->Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT V3 SEG: %06X  Line %04X %s  Partner %06X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U32(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != num_segs)
		BugError("PutGLSegs miscounted (%d != %d)\n", count, num_segs);
}


static void PutSubsecs(const Instance &inst, const char *name, int do_gl)
{
	int i;

	Lump_c * lump = CreateLevelLump(inst, name);

	for (i=0 ; i < num_subsecs ; i++)
	{
		raw_subsec_t raw;

		subsec_t *sub = lev_subsecs[i];

		raw.first = LE_U16(sub->seg_list->index);
		raw.num   = LE_U16(sub->seg_count);

		lump->Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		gLog.debugPrintf("PUT SUBSEC %04X  First %04X  Num %04X\n",
				sub->index, LE_U16(raw.first), LE_U16(raw.num));
#   endif
	}

	if (num_subsecs > 32767)
	{
		Failure(inst, "Number of %s has overflowed.\n", do_gl ? "GL subsectors" : "subsectors");
		MarkOverflow(do_gl ? LIMIT_GL_SSECT : LIMIT_SSECTORS);
	}
}


static void PutGLSubsecs_V5(const Instance &inst)
{
	int i;

	Lump_c *lump = CreateLevelLump(inst, "GL_SSECT");

	for (i=0 ; i < num_subsecs ; i++)
	{
		raw_v5_subsec_t raw;

		subsec_t *sub = lev_subsecs[i];

		raw.first = LE_U32(sub->seg_list->index);
		raw.num   = LE_U32(sub->seg_count);

		lump->Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		gLog.debugPrintf("PUT V3 SUBSEC %06X  First %06X  Num %06X\n",
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

	// Note that x/y/dx/dy are always integral in non-UDMF maps
	raw.x  = LE_S16(iround(node->x));
	raw.y  = LE_S16(iround(node->y));
	raw.dx = LE_S16(iround(node->dx));
	raw.dy = LE_S16(iround(node->dy));

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
		BugError("Bad right child in node %d\n", node->index);

	if (node->l.node)
		raw.left = LE_U16(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U16(node->l.subsec->index | 0x8000);
	else
		BugError("Bad left child in node %d\n", node->index);

	lump->Write(&raw, sizeof(raw));

# if DEBUG_BSP
	gLog.debugPrintf("PUT NODE %04X  Left %04X  Right %04X  "
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

	raw.x  = LE_S16(iround(node->x));
	raw.y  = LE_S16(iround(node->y));
	raw.dx = LE_S16(iround(node->dx));
	raw.dy = LE_S16(iround(node->dy));

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
		BugError("Bad right child in V5 node %d\n", node->index);

	if (node->l.node)
		raw.left = LE_U32(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U32(node->l.subsec->index | 0x80000000U);
	else
		BugError("Bad left child in V5 node %d\n", node->index);

	lump->Write(&raw, sizeof(raw));

# if DEBUG_BSP
	gLog.debugPrintf("PUT V5 NODE %08X  Left %08X  Right %08X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U32(raw.left),
			LE_U32(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


void PutNodes(const Instance &inst, const char *name, int do_v5, node_t *root)
{
	Lump_c *lump = CreateLevelLump(inst, name);

	node_cur_index = 0;

	if (root)
	{
		if (do_v5)
			PutOneNode_V5(root, lump);
		else
			PutOneNode(root, lump);
	}

	if (node_cur_index != num_nodes)
		BugError("PutNodes miscounted (%d != %d)\n",
				node_cur_index, num_nodes);

	if (!do_v5 && node_cur_index > 32767)
	{
		Failure(inst, "Number of nodes has overflowed.\n");
		MarkOverflow(LIMIT_NODES);
	}
}


static void CheckLimits(bool& force_v5, bool& force_xnod, const Instance &inst)
{
	if (inst.level.numSectors() > 65534)
	{
		Failure(inst, "Map has too many sectors.\n");
		MarkOverflow(LIMIT_SECTORS);
	}

	if (inst.level.numSidedefs() > 65534)
	{
		Failure(inst, "Map has too many sidedefs.\n");
		MarkOverflow(LIMIT_SIDEDEFS);
	}

	if (inst.level.numLinedefs() > 65534)
	{
		Failure(inst, "Map has too many linedefs.\n");
		MarkOverflow(LIMIT_LINEDEFS);
	}

	if (cur_info->gl_nodes && !cur_info->force_v5)
	{
		if (num_old_vert > 32767 ||
			num_new_vert > 32767 ||
			num_segs > 65534 ||
			num_nodes > 32767)
		{
			Warning(inst, "Forcing V5 of GL-Nodes due to overflows.\n");
			force_v5 = true;
		}
	}

	if (! cur_info->force_xnod)
	{
		if (num_old_vert > 32767 ||
			num_new_vert > 32767 ||
			num_segs > 32767 ||
			num_nodes > 32767)
		{
			Warning(inst, "Forcing XNOD format nodes due to overflows.\n");
			force_xnod = true;
		}
	}
}


struct seg_index_CMP_pred
{
	inline bool operator() (const seg_t *A, const seg_t *B) const
	{
		return A->index < B->index;
	}
};

void SortSegs()
{
	// do a sanity check
	for (int i = 0 ; i < num_segs ; i++)
		if (lev_segs[i]->index < 0)
			BugError("Seg %d never reached a subsector!\n", i);

	// sort segs into ascending index
	std::sort(lev_segs.begin(), lev_segs.end(), seg_index_CMP_pred());

	// remove unwanted segs
	while (lev_segs.size() > 0 && lev_segs.back()->index == SEG_IS_GARBAGE)
	{
		UtilFree((void *) lev_segs.back());
		lev_segs.pop_back();
	}
}


/* ----- ZDoom format writing --------------------------- */

static const u8_t *lev_XNOD_magic = (u8_t *) "XNOD";
static const u8_t *lev_XGL3_magic = (u8_t *) "XGL3";
static const u8_t *lev_ZNOD_magic = (u8_t *) "ZNOD";

void PutZVertices()
{
	int count, i;

	u32_t orgverts = LE_U32(num_old_vert);
	u32_t newverts = LE_U32(num_new_vert);

	ZLibAppendLump(&orgverts, 4);
	ZLibAppendLump(&newverts, 4);

	for (i=0, count=0 ; i < num_vertices ; i++)
	{
		raw_v2_vertex_t raw;

		vertex_t *vert = lev_vertices[i];

		if (! vert->is_new)
			continue;

		raw.x = LE_S32(iround(vert->x * 65536.0));
		raw.y = LE_S32(iround(vert->y * 65536.0));

		ZLibAppendLump(&raw, sizeof(raw));

		count++;
	}

	if (count != num_new_vert)
		BugError("PutZVertices miscounted (%d != %d)\n", count, num_new_vert);
}


void PutZSubsecs()
{
	int i;
	int count;
	u32_t raw_num = LE_U32(num_subsecs);

	int cur_seg_index = 0;

	ZLibAppendLump(&raw_num, 4);

	for (i=0 ; i < num_subsecs ; i++)
	{
		subsec_t *sub = lev_subsecs[i];
		seg_t *seg;

		raw_num = LE_U32(sub->seg_count);

		ZLibAppendLump(&raw_num, 4);

		// sanity check the seg index values
		count = 0;
		for (seg = sub->seg_list ; seg ; seg = seg->next, cur_seg_index++)
		{
			if (cur_seg_index != seg->index)
				BugError("PutZSubsecs: seg index mismatch in sub %d (%d != %d)\n",
						i, cur_seg_index, seg->index);

			count++;
		}

		if (count != sub->seg_count)
			BugError("PutZSubsecs: miscounted segs in sub %d (%d != %d)\n",
					i, count, sub->seg_count);
	}

	if (cur_seg_index != num_segs)
		BugError("PutZSubsecs miscounted segs (%d != %d)\n",
				cur_seg_index, num_segs);
}


void PutZSegs()
{
	int i, count;
	u32_t raw_num = LE_U32(num_segs);

	ZLibAppendLump(&raw_num, 4);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		seg_t *seg = lev_segs[i];

		if (count != seg->index)
			BugError("PutZSegs: seg index mismatch (%d != %d)\n",
					count, seg->index);

		{
			u32_t v1 = LE_U32(VertexIndex_XNOD(seg->start));
			u32_t v2 = LE_U32(VertexIndex_XNOD(seg->end));

			u16_t line = LE_U16(seg->linedef);
			u8_t  side = static_cast<u8_t>(seg->side);

			ZLibAppendLump(&v1,   4);
			ZLibAppendLump(&v2,   4);
			ZLibAppendLump(&line, 2);
			ZLibAppendLump(&side, 1);
		}

		count++;
	}

	if (count != num_segs)
		BugError("PutZSegs miscounted (%d != %d)\n", count, num_segs);
}


void PutXGL3Segs()
{
	int i, count;
	u32_t raw_num = LE_U32(num_segs);

	ZLibAppendLump(&raw_num, 4);

	for (i=0, count=0 ; i < num_segs ; i++)
	{
		seg_t *seg = lev_segs[i];

		if (count != seg->index)
			BugError("PutXGL3Segs: seg index mismatch (%d != %d)\n",
					count, seg->index);

		{
			u32_t v1   = LE_U32(VertexIndex_XNOD(seg->start));
			u32_t partner = LE_U32(seg->partner ? seg->partner->index : -1);
			u32_t line = LE_U32(seg->linedef);
			u8_t  side = static_cast<u8_t>(seg->side);

# if DEBUG_BSP
			fprintf(stderr, "SEG[%d] v1=%d partner=%d line=%d side=%d\n", i, v1, partner, line, side);
# endif

			ZLibAppendLump(&v1,      4);
			ZLibAppendLump(&partner, 4);
			ZLibAppendLump(&line,    4);
			ZLibAppendLump(&side,    1);
		}

		count++;
	}

	if (count != num_segs)
		BugError("PutXGL3Segs miscounted (%d != %d)\n", count, num_segs);
}


static void PutOneZNode(node_t *node, bool do_xgl3)
{
	raw_v5_node_t raw;

	if (node->r.node)
		PutOneZNode(node->r.node, do_xgl3);

	if (node->l.node)
		PutOneZNode(node->l.node, do_xgl3);

	node->index = node_cur_index++;

	if (do_xgl3)
	{
		u32_t x  = LE_S32(iround(node->x  * 65536.0));
		u32_t y  = LE_S32(iround(node->y  * 65536.0));
		u32_t dx = LE_S32(iround(node->dx * 65536.0));
		u32_t dy = LE_S32(iround(node->dy * 65536.0));

		ZLibAppendLump(&x,  4);
		ZLibAppendLump(&y,  4);
		ZLibAppendLump(&dx, 4);
		ZLibAppendLump(&dy, 4);
	}
	else
	{
		raw.x  = LE_S16(iround(node->x));
		raw.y  = LE_S16(iround(node->y));
		raw.dx = LE_S16(iround(node->dx));
		raw.dy = LE_S16(iround(node->dy));

		ZLibAppendLump(&raw.x,  2);
		ZLibAppendLump(&raw.y,  2);
		ZLibAppendLump(&raw.dx, 2);
		ZLibAppendLump(&raw.dy, 2);
	}

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
		BugError("Bad right child in V5 node %d\n", node->index);

	if (node->l.node)
		raw.left = LE_U32(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U32(node->l.subsec->index | 0x80000000U);
	else
		BugError("Bad left child in V5 node %d\n", node->index);

	ZLibAppendLump(&raw.right, 4);
	ZLibAppendLump(&raw.left,  4);

# if DEBUG_BSP
	gLog.debugPrintf("PUT Z NODE %08X  Left %08X  Right %08X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U32(raw.left),
			LE_U32(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


void PutZNodes(node_t *root, bool do_xgl3)
{
	u32_t raw_num = LE_U32(num_nodes);

	ZLibAppendLump(&raw_num, 4);

	node_cur_index = 0;

	if (root)
		PutOneZNode(root, do_xgl3);

	if (node_cur_index != num_nodes)
		BugError("PutZNodes miscounted (%d != %d)\n",
				node_cur_index, num_nodes);
}

static void SaveZDFormat(const Instance &inst, node_t *root_node)
{
	// leave SEGS and SSECTORS empty
	CreateLevelLump(inst, "SEGS");
	CreateLevelLump(inst, "SSECTORS");

	Lump_c *lump = CreateLevelLump(inst, "NODES");

	if (cur_info->force_compress)
		lump->Write(lev_ZNOD_magic, 4);
	else
		lump->Write(lev_XNOD_magic, 4);

	// the ZLibXXX functions do no compression for XNOD format
	ZLibBeginLump(lump);

	PutZVertices();
	PutZSubsecs();
	PutZSegs();
	PutZNodes(root_node, false /* do_xgl3 */);

	ZLibFinishLump();
}


static void SaveXGL3Format(const Instance &inst, node_t *root_node)
{
	// WISH : compute a max_size

	Lump_c *lump = CreateLevelLump(inst, "ZNODES");

	lump->Write(lev_XGL3_magic, 4);

	// disable compression
	cur_info->force_compress = false;

	ZLibBeginLump(lump);

	PutZVertices();
	PutZSubsecs();
	PutXGL3Segs();
	PutZNodes(root_node, true /* do_xgl3 */);

	ZLibFinishLump();
}


/* ----- whole-level routines --------------------------- */

static void LoadLevel(const Instance &inst)
{
	Lump_c *LEV = inst.wad.master.edit_wad->GetLump(lev_current_start);

	lev_current_name = LEV->Name();
	lev_overflows = 0;

	inst.GB_PrintMsg("Building nodes on %s\n", lev_current_name.c_str());

	num_new_vert = 0;
	num_real_lines = 0;

	GetVertices(inst.level);

	for(LineDef *L : inst.level.linedefs)
	{
		if (L->right >= 0 || L->left >= 0)
			num_real_lines++;

		// init some fake flags
		L->flags &= ~(MLF_IS_PRECIOUS | MLF_IS_OVERLAP);

		if (L->tag >= 900 && L->tag < 1000)
			L->flags |= MLF_IS_PRECIOUS;
	}

	PrintDetail("Loaded %d vertices, %d sectors, %d sides, %d lines, %d things\n",
			inst.level.numVertices(), inst.level.numSectors(), inst.level.numSidedefs(), inst.level.numLinedefs(), inst.level.numThings());

	DetectOverlappingVertices(inst.level);
	DetectOverlappingLines(inst.level);

	CalculateWallTips(inst.level);

	if (inst.loaded.levelFormat != MapFormat::doom)
	{
		// -JL- Find sectors containing polyobjs
		DetectPolyobjSectors(inst);
	}
}


void FreeLevel(void)
{
	FreeVertices();
	FreeSegs();
	FreeSubsecs();
	FreeNodes();
	FreeWallTips();
}

static Lump_c *FindLevelLump(const Instance &inst, const char *name);

static u32_t CalcGLChecksum(const Instance &inst)
{
	u32_t crc;

	Adler32_Begin(&crc);

	Lump_c *lump = FindLevelLump(inst, "VERTEXES");

	if (lump && lump->Length() > 0)
	{
		u8_t *data = new u8_t[lump->Length()];

		lump->Seek();
		if (! lump->Read(data, lump->Length()))
			FatalError("Error reading vertices (for checksum).\n");

		Adler32_AddBlock(&crc, data, lump->Length());
		delete[] data;
	}

	lump = FindLevelLump(inst, "LINEDEFS");

	if (lump && lump->Length() > 0)
	{
		u8_t *data = new u8_t[lump->Length()];
		lump->Seek();
		if (! lump->Read(data, lump->Length()))
			FatalError("Error reading linedefs (for checksum).\n");

		Adler32_AddBlock(&crc, data, lump->Length());
		delete[] data;
	}

	Adler32_Finish(&crc);

	return crc;
}


inline static SString CalcOptionsString()
{
	return SString::printf("--cost %d%s", cur_info->factor, cur_info->fast ? " --fast" : "");
}


static void UpdateGLMarker(const Instance &inst, Lump_c *marker)
{
	// we *must* compute the checksum BEFORE (re)creating the lump
	// [ otherwise we write data into the wrong part of the file ]
	u32_t crc = CalcGLChecksum(inst);

	// when original name is long, need to specify it here
	if (lev_current_name.length() > 5)
	{
		marker->Printf("LEVEL=%s\n", lev_current_name.c_str());
	}

	marker->Printf("BUILDER=%s\n", "Eureka " EUREKA_VERSION);
	marker->Printf("OPTIONS=%s\n", CalcOptionsString().c_str());

	SString time_str = UtilTimeString();

	if (!time_str.empty())
	{
		marker->Printf("TIME=%s\n", time_str.c_str());
	}

	marker->Printf("CHECKSUM=0x%08x\n", crc);
}


static void AddMissingLump(const Instance &inst, const char *name, const char *after)
{
	if (inst.wad.master.edit_wad->LevelLookupLump(lev_current_idx, name) >= 0)
		return;

	int exist = inst.wad.master.edit_wad->LevelLookupLump(lev_current_idx, after);

	// if this happens, the level structure is very broken
	if (exist < 0)
	{
		Warning(inst, "Missing %s lump -- level structure is broken\n", after);

		exist = inst.wad.master.edit_wad->LevelLastLump(lev_current_idx);
	}

	inst.wad.master.edit_wad->InsertPoint(exist + 1);

	inst.wad.master.edit_wad->AddLump(name);
}

static Lump_c *CreateGLMarker(const Instance &inst);

static build_result_e SaveLevel(node_t *root_node, const Instance &inst)
{
	// Note: root_node may be NULL

	// remove any existing GL-Nodes
	inst.wad.master.edit_wad->RemoveGLNodes(lev_current_idx);

	// ensure all necessary level lumps are present
	AddMissingLump(inst, "SEGS",     "VERTEXES");
	AddMissingLump(inst, "SSECTORS", "SEGS");
	AddMissingLump(inst, "NODES",    "SSECTORS");
	AddMissingLump(inst, "REJECT",   "SECTORS");
	AddMissingLump(inst, "BLOCKMAP", "REJECT");

	// user preferences
	bool force_v5   = cur_info->force_v5;
	bool force_xnod = cur_info->force_xnod;

	// check for overflows...
	// this sets the force_xxx vars if certain limits are breached
	CheckLimits(force_v5, force_xnod, inst);


	/* --- GL Nodes --- */

	Lump_c * gl_marker = NULL;

	if (cur_info->gl_nodes && num_real_lines > 0)
	{
		SortSegs();

		// create empty marker now, flesh it out later
		gl_marker = CreateGLMarker(inst);

		PutGLVertices(inst, force_v5);

		if (force_v5)
			PutGLSegs_V5(inst);
		else
			PutGLSegs(inst);

		if (force_v5)
			PutGLSubsecs_V5(inst);
		else
			PutSubsecs(inst, "GL_SSECT", true);

		PutNodes(inst, "GL_NODES", force_v5, root_node);

		// -JL- Add empty PVS lump
		CreateLevelLump(inst, "GL_PVS");
	}


	/* --- Normal nodes --- */

	// remove all the mini-segs from subsectors
	NormaliseBspTree();

	if (force_xnod && num_real_lines > 0)
	{
		SortSegs();

		SaveZDFormat(inst, root_node);
	}
	else
	{
		// reduce vertex precision for classic DOOM nodes.
		// some segs can become "degenerate" after this, and these
		// are removed from subsectors.
		RoundOffBspTree();

		// this also removes minisegs and degenerate segs
		SortSegs();

		PutVertices(inst, "VERTEXES", false);

		PutSegs(inst);
		PutSubsecs(inst, "SSECTORS", false);
		PutNodes(inst, "NODES", false, root_node);
	}

	PutBlockmap(inst);
	PutReject(inst);

	// keyword support (v5.0 of the specs).
	// must be done *after* doing normal nodes (for proper checksum).
	if (gl_marker)
	{
		UpdateGLMarker(inst, gl_marker);
	}

	inst.wad.master.edit_wad->writeToDisk();

	if (lev_overflows > 0)
	{
		cur_info->total_failed_maps++;
		inst.GB_PrintMsg("FAILED with %d overflowed lumps\n", lev_overflows);

		return BUILD_LumpOverflow;
	}

	return BUILD_OK;
}


static build_result_e SaveUDMF(const Instance &inst, node_t *root_node)
{
	// remove any existing ZNODES lump
	inst.wad.master.edit_wad->RemoveZNodes(lev_current_idx);

	if (num_real_lines >= 0)
	{
		SortSegs();

		SaveXGL3Format(inst, root_node);
	}

	inst.wad.master.edit_wad->writeToDisk();

	if (lev_overflows > 0)
	{
		cur_info->total_failed_maps++;
		inst.GB_PrintMsg("FAILED with %d overflowed lumps\n", lev_overflows);

		return BUILD_LumpOverflow;
	}

	return BUILD_OK;
}


//----------------------------------------------------------------------


static Lump_c  *zout_lump;
static z_stream zout_stream;
static Bytef    zout_buffer[1024];


void ZLibBeginLump(Lump_c *lump)
{
	zout_lump = lump;

	if (! cur_info->force_compress)
		return;

	zout_stream.zalloc = (alloc_func)0;
	zout_stream.zfree  = (free_func)0;
	zout_stream.opaque = (voidpf)0;

	if (Z_OK != deflateInit(&zout_stream, Z_DEFAULT_COMPRESSION))
		FatalError("Trouble setting up zlib compression\n");

	zout_stream.next_out  = zout_buffer;
	zout_stream.avail_out = sizeof(zout_buffer);
}


void ZLibAppendLump(const void *data, int length)
{
	// ASSERT(zout_lump)
	// ASSERT(length > 0)

	if (! cur_info->force_compress)
	{
		zout_lump->Write(data, length);
		return;
	}

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
	if (! cur_info->force_compress)
	{
		zout_lump = NULL;
		return;
	}

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


static Lump_c * FindLevelLump(const Instance &inst, const char *name)
{
	int idx = inst.wad.master.edit_wad->LevelLookupLump(lev_current_idx, name);

	if (idx < 0)
		return NULL;

	return inst.wad.master.edit_wad->GetLump(idx);
}


static Lump_c * CreateLevelLump(const Instance &inst, const char *name)
{
	// look for existing one
	Lump_c *lump = FindLevelLump(inst, name);

	if(!lump)
	{
		int last_idx = inst.wad.master.edit_wad->LevelLastLump(lev_current_idx);

		// in UDMF maps, insert before the ENDMAP lump, otherwise insert
		// after the last known lump of the level.
		if (inst.loaded.levelFormat != MapFormat::udmf)
			last_idx++;

		inst.wad.master.edit_wad->InsertPoint(last_idx);

		lump = inst.wad.master.edit_wad->AddLump(name);
	}

    lump->clearData();
	return lump;
}


static Lump_c * CreateGLMarker(const Instance &inst)
{
	SString name_buf;

	if (lev_current_name.length() <= 5)
	{
		name_buf = "GL_" + lev_current_name;
	}
	else
	{
		// names longer than 5 chars use "GL_LEVEL" as marker name
		name_buf = "GL_LEVEL";
	}

	int last_idx = inst.wad.master.edit_wad->LevelLastLump(lev_current_idx);

	inst.wad.master.edit_wad->InsertPoint(last_idx + 1);

	Lump_c *marker = inst.wad.master.edit_wad->AddLump(name_buf);

	return marker;
}


//------------------------------------------------------------------------
// MAIN STUFF
//------------------------------------------------------------------------

nodebuildinfo_t * cur_info = NULL;


static build_result_e BuildLevel(nodebuildinfo_t *info, int lev_idx, const Instance &inst)
{
	cur_info = info;

	node_t *root_node  = NULL;
	subsec_t *root_sub = NULL;
	bbox_t root_bbox;

	if (cur_info->cancelled)
		return BUILD_Cancelled;

	lev_current_idx   = lev_idx;
	lev_current_start = inst.wad.master.edit_wad->LevelHeader(lev_idx);

	LoadLevel(inst);

	InitBlockmap(inst.level);


	build_result_e ret = BUILD_OK;

	if (num_real_lines > 0)
	{
		// create initial segs
		seg_t *list = CreateSegs(inst);

		// recursively create nodes
		ret = BuildNodes(list, &root_bbox, &root_node, &root_sub, 0, inst);
	}

	if (ret == BUILD_OK)
	{
		PrintDetail("Built %d NODES, %d SSECTORS, %d SEGS, %d VERTEXES\n",
					num_nodes, num_subsecs, num_segs, num_old_vert + num_new_vert);

		if (root_node)
		{
			PrintDetail("Heights of left and right subtrees = (%d,%d)\n",
					ComputeBspHeight(root_node->r.node),
					ComputeBspHeight(root_node->l.node));
		}

		ClockwiseBspTree(inst.level);

		if (inst.loaded.levelFormat == MapFormat::udmf)
			ret = SaveUDMF(inst, root_node);
		else
			ret = SaveLevel(root_node, inst);
	}
	else
	{
		/* build was Cancelled by the user */
	}

	FreeLevel();
	FreeQuickAllocCuts();

	// clear some fake line flags
	for(LineDef *linedef : inst.level.linedefs)
		linedef->flags &= ~(MLF_IS_PRECIOUS | MLF_IS_OVERLAP);

	return ret;
}

}  // namespace ajbsp


build_result_e AJBSP_BuildLevel(nodebuildinfo_t *info, int lev_idx, const Instance &inst)
{
	return ajbsp::BuildLevel(info, lev_idx, inst);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
