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

#include "bsp.h"
#include "Instance.h"
#include "w_rawdef.h"

#include <zlib.h>


namespace ajbsp
{

#define DEBUG_BLOCKMAP  0
#define DEBUG_REJECT    0
#define DEBUG_LOAD      0
#define DEBUG_BSP       0

#define BLOCK_LIMIT  16000

#define DUMMY_DUP  0xFFFF


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

void LevelData::Block::Add(int blk_num, int line_index)
{
	uint16_t *cur = block_lines[blk_num];

# if DEBUG_BLOCKMAP
	gLog.debugPrintf("Block %d has line %d\n", blk_num, line_index);
# endif

	if (blk_num < 0 || blk_num >= block_count)
		BugError("BlockAdd: bad block number %d\n", blk_num);

	if (! cur)
	{
		// create empty block
		block_lines[blk_num] = cur = (uint16_t *)UtilCalloc(BK_QUANTUM * sizeof(uint16_t));
		cur[BK_NUM] = 0;
		cur[BK_MAX] = BK_QUANTUM;
		cur[BK_XOR] = 0x1234;
	}

	if (BK_FIRST + cur[BK_NUM] == cur[BK_MAX])
	{
		// no more room, so allocate some more...
		cur[BK_MAX] += BK_QUANTUM;

		block_lines[blk_num] = cur = (uint16_t *)UtilRealloc(cur, cur[BK_MAX] * sizeof(uint16_t));
	}

	// compute new checksum
	cur[BK_XOR] = static_cast<uint16_t>(((cur[BK_XOR] << 4) | (cur[BK_XOR] >> 12)) ^ line_index);

	cur[BK_FIRST + cur[BK_NUM]] = LE_U16(line_index);
	cur[BK_NUM]++;
}


void LevelData::Block::AddLine(int line_index, const Document &doc)
{
	const auto L = doc.linedefs[line_index];

	int x1 = (int) doc.getStart(*L).x();
	int y1 = (int) doc.getStart(*L).y();
	int x2 = (int) doc.getEnd(*L).x();
	int y2 = (int) doc.getEnd(*L).y();

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
			Add(blk_num, line_index);
		}
		return;
	}

	// handle simple case #2: completely vertical
	if (bx1 == bx2)
	{
		for (by=by1 ; by <= by2 ; by++)
		{
			int blk_num = by * block_w + bx1;
			Add(blk_num, line_index);
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
			Add(blk_num, line_index);
		}
	}
}


void LevelData::Block::CreateMap(const Document &doc)
{
	block_lines = (uint16_t **) UtilCalloc(block_count * sizeof(uint16_t *));

	for (int i=0 ; i < doc.numLinedefs() ; i++)
	{
		// ignore zero-length lines
		if (doc.isZeroLength(*doc.linedefs[i]))
			continue;

		AddLine(i, doc);
	}
}


int LevelData::Block::Compare(const void *p1, const void *p2) const
{
	int blk_num1 = ((const uint16_t *) p1)[0];
	int blk_num2 = ((const uint16_t *) p2)[0];

	const uint16_t *A = block_lines[blk_num1];
	const uint16_t *B = block_lines[blk_num2];

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

	return memcmp(A+BK_FIRST, B+BK_FIRST, A[BK_NUM] * sizeof(uint16_t));
}


void LevelData::Block::CompressMap()
{
	int i;
	int cur_offset;
	int dup_count=0;
   (void)dup_count;

	int orig_size, new_size;

	block_ptrs = (uint16_t *)UtilCalloc(block_count * sizeof(uint16_t));
	block_dups = (uint16_t *)UtilCalloc(block_count * sizeof(uint16_t));

	// sort duplicate-detecting array.  After the sort, all duplicates
	// will be next to each other.  The duplicate array gives the order
	// of the blocklists in the BLOCKMAP lump.

	for (i=0 ; i < block_count ; i++)
		block_dups[i] = static_cast<uint16_t>(i);
	
	std::sort(block_dups, block_dups + block_count, [this](uint16_t a, uint16_t b)
			  {
		return Compare(&a, &b) < 0;
	});

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
			block_ptrs[blk_num] = static_cast<uint16_t>(4 + this->block_count);
			block_dups[i] = DUMMY_DUP;

			orig_size += 2;
			continue;
		}

		count = 2 + block_lines[blk_num][BK_NUM];

		// duplicate ?  Only the very last one of a sequence of duplicates
		// will update the current offset value.

		if (i+1 < this->block_count &&
				Compare(block_dups + i, block_dups + i+1) == 0)
		{
			block_ptrs[blk_num] = static_cast<uint16_t>(cur_offset);
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

		block_ptrs[blk_num] = static_cast<uint16_t>(cur_offset);

		cur_offset += count;

		orig_size += count;
		new_size  += count;
	}

	if (cur_offset > 65535)
	{
		overflowed = true;
		return;
	}

# if DEBUG_BLOCKMAP
	gLog.debugPrintf("Blockmap: Last ptr = %d  duplicates = %d\n",
			cur_offset, dup_count);
# endif

	compression = (orig_size - new_size) * 100 / orig_size;

	// there's a tiny chance of new_size > orig_size
	if (compression < 0)
		compression = 0;
}

void LevelData::WriteBlockmap() const
{
	int i;

	Lump_c &lump = CreateLevelLump("BLOCKMAP");

	uint16_t null_block[2] = { 0x0000, 0xFFFF };
	uint16_t m_zero = 0x0000;
	uint16_t m_neg1 = 0xFFFF;

	// fill in header
	raw_blockmap_header_t header;

	header.x_origin = LE_U16(block.block_x);
	header.y_origin = LE_U16(block.block_y);
	header.x_blocks = LE_U16(block.block_w);
	header.y_blocks = LE_U16(block.block_h);

	lump.Write(&header, sizeof(header));

	// handle pointers
	for (i=0 ; i < block.block_count ; i++)
	{
		uint16_t ptr = LE_U16(block.block_ptrs[i]);

		if (ptr == 0)
			BugError("WriteBlockmap: offset %d not set.\n", i);

		lump.Write(&ptr, sizeof(uint16_t));
	}

	// add the null block which *all* empty blocks will use
	lump.Write(null_block, sizeof(null_block));

	// handle each block list
	for (i=0 ; i < block.block_count ; i++)
	{
		int blk_num = block.block_dups[i];

		// ignore duplicate or empty blocks
		if (blk_num == DUMMY_DUP)
			continue;

		uint16_t *blk = block.block_lines[blk_num];
		SYS_ASSERT(blk);

		lump.Write(&m_zero, sizeof(uint16_t));
		lump.Write(blk + BK_FIRST, blk[BK_NUM] * sizeof(uint16_t));
		lump.Write(&m_neg1, sizeof(uint16_t));
	}
}


void LevelData::Block::FreeMap()
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


static void FindMapLimits(bbox_t *bbox, const Document &doc)
{
	bbox->minx = bbox->miny = SHRT_MAX;
	bbox->maxx = bbox->maxy = SHRT_MIN;

	for (int i=0 ; i < doc.numLinedefs() ; i++)
	{
		const auto L = doc.linedefs[i];

		if (!doc.isZeroLength(*L))
		{
			double x1 = doc.getStart(*L).x();
			double y1 = doc.getStart(*L).y();
			double x2 = doc.getEnd(*L).x();
			double y2 = doc.getEnd(*L).y();

			int lx = (int)floor(std::min(x1, x2));
			int ly = (int)floor(std::min(y1, y2));
			int hx = (int)ceil(std::max(x1, x2));
			int hy = (int)ceil(std::max(y1, y2));

			if (lx < bbox->minx) bbox->minx = lx;
			if (ly < bbox->miny) bbox->miny = ly;
			if (hx > bbox->maxx) bbox->maxx = hx;
			if (hy > bbox->maxy) bbox->maxy = hy;

		}
	}

# if DEBUG_BLOCKMAP
	gLog.debugPrintf("Blockmap lines centered at (%d,%d)\n", block_mid_x, block_mid_y);
# endif
}

//
// compute blockmap origin & size (the block_x/y/w/h variables)
// based on the set of loaded linedefs.
//
void LevelData::Block::InitMap(const Document &doc)
{
	bbox_t map_bbox;

	// find limits of linedefs, and store as map limits
	FindMapLimits(&map_bbox, doc);

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
void LevelData::PutBlockmap()
{
	if (! cur_info->do_blockmap || doc.numLinedefs() == 0)
	{
		// just create an empty blockmap lump
		CreateLevelLump("BLOCKMAP");
		return;
	}

	block.overflowed = false;

	// initial phase: create internal blockmap containing the index of
	// all lines in each block.

	block.CreateMap(doc);

	// -AJA- second phase: compress the blockmap.  We do this by sorting
	//       the blocks, which is a typical way to detect duplicates in
	//       a large list.  This also detects BLOCKMAP overflow.

	block.CompressMap();

	// final phase: write it out in the correct format

	if (block.overflowed)
	{
		// leave an empty blockmap lump
		CreateLevelLump("BLOCKMAP");

		Warning("Blockmap overflowed (lump will be empty)\n");
	}
	else
	{
		WriteBlockmap();

		PrintDetail("Completed blockmap, size %dx%d (compression: %d%%)\n",
				block.block_w, block.block_h, block.compression);
	}

	block.FreeMap();
}


//------------------------------------------------------------------------
// REJECT : Generate the reject table
//------------------------------------------------------------------------

//
// Allocate the matrix, init sectors into individual groups.
//
void LevelData::Reject::Init(const Document &doc)
{
	rej_total_size = (doc.numSectors() * doc.numSectors() + 7) / 8;

	rej_matrix = new uint8_t[rej_total_size];
	memset(rej_matrix, 0, rej_total_size);

	rej_sector_groups.resize(doc.numSectors());

	for (int i=0 ; i < doc.numSectors() ; i++)
	{
		rej_sector_groups[i] = i;
	}
}


void LevelData::Reject::Free()
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
void LevelData::Reject::GroupSectors(const Document &doc)
{
	for(const auto &L : doc.linedefs)
	{
		if (L->right < 0 || L->left < 0)
			continue;

		int sec1 = doc.getRight(*L)->sector;
		int sec2 = doc.getLeft(*L) ->sector;

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


void LevelData::Reject::ProcessSectors(const Document &doc)
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


void LevelData::Reject_WriteLump() const
{
	Lump_c &lump = CreateLevelLump("REJECT");

	lump.Write(rej.rej_matrix, rej.rej_total_size);
}


//
// build the reject table and write it into the REJECT lump
//
// For now we only do very basic reject processing, limited to
// determining all isolated groups of sectors (islands that are
// surrounded by void space).
//
void LevelData::PutReject()
{
	if (! cur_info->do_reject || doc.numSectors() == 0)
	{
		// just create an empty reject lump
		CreateLevelLump("REJECT");
		return;
	}

	rej.Init(doc);
	rej.GroupSectors(doc);
	rej.ProcessSectors(doc);

# if DEBUG_REJECT
	Reject_DebugGroups();
# endif

	Reject_WriteLump();
	rej.Free();

	PrintDetail("Added simple reject lump\n");
}


//------------------------------------------------------------------------
// LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------


// Note: ZDoom format support based on code (C) 2002,2003 Randy Heit


#define ALLOC_BLKNUM  1024


/* ----- allocation routines ---------------------------- */

vertex_t *LevelData::NewVertex()
{
	vertex_t *V = (vertex_t *) UtilCalloc(sizeof(vertex_t));
	vertices.push_back(V);
	return V;
}

seg_t *LevelData::NewSeg()
{
	seg_t *S = (seg_t *) UtilCalloc(sizeof(seg_t));
	segs.push_back(S);
	return S;
}

subsec_t *LevelData::NewSubsec()
{
	subsec_t *S = (subsec_t *) UtilCalloc(sizeof(subsec_t));
	subsecs.push_back(S);
	return S;
}

node_t *LevelData::NewNode()
{
	node_t *N = (node_t *) UtilCalloc(sizeof(node_t));
	nodes.push_back(N);
	return N;
}

walltip_t *LevelData::NewWallTip()
{
	walltip_t *WT = (walltip_t *) UtilCalloc(sizeof(walltip_t));
	walltips.push_back(WT);
	return WT;
}


/* ----- free routines ---------------------------- */

void LevelData::FreeVertices()
{
	for (unsigned int i = 0 ; i < vertices.size() ; i++)
		UtilFree((void *) vertices[i]);

	vertices.clear();
}

void LevelData::FreeSegs()
{
	for (unsigned int i = 0 ; i < segs.size() ; i++)
		UtilFree((void *) segs[i]);

	segs.clear();
}

void LevelData::FreeSubsecs()
{
	for (unsigned int i = 0 ; i < subsecs.size() ; i++)
		UtilFree((void *) subsecs[i]);

	subsecs.clear();
}

void LevelData::FreeNodes()
{
	for (unsigned int i = 0 ; i < nodes.size() ; i++)
		UtilFree((void *) nodes[i]);

	nodes.clear();
}

void LevelData::FreeWallTips()
{
	for (unsigned int i = 0 ; i < walltips.size() ; i++)
		UtilFree((void *) walltips[i]);

	walltips.clear();
}


/* ----- reading routines ------------------------------ */

void LevelData::GetVertices()
{
	for (int i = 0 ; i < doc.numVertices() ; i++)
	{
		vertex_t *vert = NewVertex();

		vert->x = doc.vertices[i]->x();
		vert->y = doc.vertices[i]->y();

		vert->index = i;
	}

	num_old_vert = (int)vertices.size();
}


#if 0
static inline SideDef *SafeLookupSidedef(uint16_t num)
{
	if (num == 0xFFFF)
		return NULL;

	if ((int)num >= doc.numSidedefs() && (int16_t)(num) < 0)
		return NULL;

	return SideDefs[num];
}
#endif


static inline int VanillaSegDist(const seg_t *seg, const Document &doc)
{
	const auto L = doc.linedefs[seg->linedef];

	double lx = seg->side ? doc.getEnd(*L).x() : doc.getStart(*L).x();
	double ly = seg->side ? doc.getEnd(*L).y() : doc.getStart(*L).y();

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

static const uint8_t *lev_v2_magic = (uint8_t *) "gNd2";
static const uint8_t *lev_v5_magic = (uint8_t *) "gNd5";


void LevelData::MarkOverflow(int flags)
{
	// flags are ignored

	overflows++;
}


void LevelData::PutVertices(const char *name, int do_gl)
{
	int count, i;

	Lump_c &lump = CreateLevelLump(name);

	for (i=0, count=0 ; i < (int)vertices.size() ; i++)
	{
		raw_vertex_t raw;

		vertex_t *vert = vertices[i];

		if ((do_gl ? 1 : 0) != (vert->is_new ? 1 : 0))
		{
			continue;
		}

		raw.x = LE_S16(iround(vert->x));
		raw.y = LE_S16(iround(vert->y));

		lump.Write(&raw, sizeof(raw));

		count++;
	}

	if (count != (do_gl ? num_new_vert : num_old_vert))
		BugError("PutVertices miscounted (%d != %d)\n", count,
				do_gl ? num_new_vert : num_old_vert);

	if (! do_gl && count > 65534)
	{
		Failure("Number of vertices has overflowed.\n");
		MarkOverflow(LIMIT_VERTEXES);
	}
}


void LevelData::PutGLVertices(int do_v5) const
{
	int count, i;

	Lump_c &lump = CreateLevelLump("GL_VERT");

	if (do_v5)
		lump.Write(lev_v5_magic, 4);
	else
		lump.Write(lev_v2_magic, 4);

	for (i=0, count=0 ; i < (int)vertices.size() ; i++)
	{
		raw_v2_vertex_t raw;

		vertex_t *vert = vertices[i];

		if (! vert->is_new)
			continue;

		raw.x = LE_S32((int)(vert->x * 65536.0));
		raw.y = LE_S32((int)(vert->y * 65536.0));

		lump.Write(&raw, sizeof(raw));

		count++;
	}

	if (count != num_new_vert)
		BugError("PutGLVertices miscounted (%d != %d)\n", count, num_new_vert);
}


static inline uint16_t VertexIndex16Bit(const vertex_t *v)
{
	if (v->is_new)
		return (uint16_t) (v->index | 0x8000U);

	return (uint16_t) v->index;
}


static inline uint32_t VertexIndex_V5(const vertex_t *v)
{
	if (v->is_new)
		return (uint32_t) (v->index | 0x80000000U);

	return (uint32_t) v->index;
}

void LevelData::PutSegs()
{
	int i, count;

	Lump_c &lump = CreateLevelLump("SEGS");

	for (i=0, count=0 ; i < (int)segs.size() ; i++)
	{
		raw_seg_t raw;

		seg_t *seg = segs[i];

		raw.start   = LE_U16(VertexIndex16Bit(seg->start));
		raw.end     = LE_U16(VertexIndex16Bit(seg->end));
		raw.angle   = LE_U16(VanillaSegAngle(seg));
		raw.linedef = LE_U16(seg->linedef);
		raw.flip    = LE_U16(seg->side);
		raw.dist    = LE_U16(VanillaSegDist(seg, doc));

		lump.Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT SEG: %04X  Vert %04X->%04X  Line %04X %s  "
				"Angle %04X  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index,
				LE_U16(raw.start), LE_U16(raw.end), LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.angle),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != (int)segs.size())
		BugError("PutSegs miscounted (%d != %d)\n", count, (int)segs.size());

	if (count > 65534)
	{
		Failure("Number of segs has overflowed.\n");
		MarkOverflow(LIMIT_SEGS);
	}
}


void LevelData::PutGLSegs() const
{
	int i, count;

	Lump_c &lump = CreateLevelLump("GL_SEGS");

	for (i=0, count=0 ; i < (int)segs.size() ; i++)
	{
		raw_gl_seg_t raw;

		seg_t *seg = segs[i];

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

		lump.Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT GL SEG: %04X  Line %04X %s  Partner %04X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U16(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != (int)segs.size())
		BugError("PutGLSegs miscounted (%d != %d)\n", count, (int)segs.size());

	if (count > 65534)
		BugError("PutGLSegs with %d (> 65534) segs\n", count);
}


void LevelData::PutGLSegs_V5() const
{
	int i, count;

	Lump_c &lump = CreateLevelLump("GL_SEGS");

	for (i=0, count=0 ; i < (int)segs.size() ; i++)
	{
		raw_v5_seg_t raw;

		seg_t *seg = segs[i];

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

		lump.Write(&raw, sizeof(raw));

		count++;

#   if DEBUG_BSP
		gLog.debugPrintf("PUT V3 SEG: %06X  Line %04X %s  Partner %06X  "
				"(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, LE_U16(raw.linedef),
				seg->side ? "L" : "R", LE_U32(raw.partner),
				seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
	}

	if (count != (int)segs.size())
		BugError("PutGLSegs miscounted (%d != %d)\n", count, (int)segs.size());
}


void LevelData::PutSubsecs(const char *name, int do_gl)
{
	int i;

	Lump_c & lump = CreateLevelLump(name);

	for (i=0 ; i < (int)subsecs.size() ; i++)
	{
		raw_subsec_t raw;

		subsec_t *sub = subsecs[i];

		raw.first = LE_U16(sub->seg_list->index);
		raw.num   = LE_U16(sub->seg_count);

		lump.Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		gLog.debugPrintf("PUT SUBSEC %04X  First %04X  Num %04X\n",
				sub->index, LE_U16(raw.first), LE_U16(raw.num));
#   endif
	}

	if ((int)segs.size() > 32767)
	{
		Failure("Number of %s has overflowed.\n", do_gl ? "GL subsectors" : "subsectors");
		MarkOverflow(do_gl ? LIMIT_GL_SSECT : LIMIT_SSECTORS);
	}
}


void LevelData::PutGLSubsecs_V5() const
{
	int i;

	Lump_c &lump = CreateLevelLump("GL_SSECT");

	for (i=0 ; i < (int)subsecs.size() ; i++)
	{
		raw_v5_subsec_t raw;

		subsec_t *sub = subsecs[i];

		raw.first = LE_U32(sub->seg_list->index);
		raw.num   = LE_U32(sub->seg_count);

		lump.Write(&raw, sizeof(raw));

#   if DEBUG_BSP
		gLog.debugPrintf("PUT V3 SUBSEC %06X  First %06X  Num %06X\n",
					sub->index, LE_U32(raw.first), LE_U32(raw.num));
#   endif
	}
}

void LevelData::PutOneNode(node_t *node, Lump_c *lump)
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


void LevelData::PutOneNode_V5(node_t *node, Lump_c *lump)
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


void LevelData::PutNodes(const char *name, int do_v5, node_t *root)
{
	Lump_c &lump = CreateLevelLump(name);

	node_cur_index = 0;

	if (root)
	{
		if (do_v5)
			PutOneNode_V5(root, &lump);
		else
			PutOneNode(root, &lump);
	}

	if (node_cur_index != (int)nodes.size())
		BugError("PutNodes miscounted (%d != %d)\n",
				node_cur_index, (int)nodes.size());

	if (!do_v5 && node_cur_index > 32767)
	{
		Failure("Number of nodes has overflowed.\n");
		MarkOverflow(LIMIT_NODES);
	}
}


void LevelData::CheckLimits(bool& force_v5, bool& force_xnod)
{
	if (doc.numSectors() > 65534)
	{
		Failure("Map has too many sectors.\n");
		MarkOverflow(LIMIT_SECTORS);
	}

	if (doc.numSidedefs() > 65534)
	{
		Failure("Map has too many sidedefs.\n");
		MarkOverflow(LIMIT_SIDEDEFS);
	}

	if (doc.numLinedefs() > 65534)
	{
		Failure("Map has too many linedefs.\n");
		MarkOverflow(LIMIT_LINEDEFS);
	}

	if (cur_info->gl_nodes && !cur_info->force_v5)
	{
		if (num_old_vert > 32767 ||
			num_new_vert > 32767 ||
			(int)segs.size() > 65534 ||
			(int)nodes.size() > 32767)
		{
			Warning("Forcing V5 of GL-Nodes due to overflows.\n");
			force_v5 = true;
		}
	}

	if (! cur_info->force_xnod)
	{
		if (num_old_vert > 32767 ||
			num_new_vert > 32767 ||
			(int)segs.size() > 32767 ||
			(int)nodes.size() > 32767)
		{
			Warning("Forcing XNOD format nodes due to overflows.\n");
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

void LevelData::SortSegs()
{
	// do a sanity check
	for (int i = 0 ; i < (int)segs.size() ; i++)
		if (segs[i]->index < 0)
			BugError("Seg %d never reached a subsector!\n", i);

	// sort segs into ascending index
	std::sort(segs.begin(), segs.end(), seg_index_CMP_pred());

	// remove unwanted segs
	while (segs.size() > 0 && segs.back()->index == SEG_IS_GARBAGE)
	{
		UtilFree((void *) segs.back());
		segs.pop_back();
	}
}


/* ----- ZDoom format writing --------------------------- */


class ZLibContext
{
public:
	class Compression
	{
	public:
		class Exception : public std::runtime_error
		{
		public:
			Exception(int result, const SString& message) : std::runtime_error(message.get() + ": " + std::to_string(result))
			{
			}
		};

		Compression();
		~Compression();
		Compression(const Compression& other) = delete;
		Compression& operator = (const Compression& other) = delete;

		int deflate(int flush)
		{
			return ::deflate(&stream, flush);
		}

		void setNextOut(Bytef* out, uInt outSize) noexcept
		{
			stream.next_out = out;
			stream.avail_out = outSize;
		}

		uInt getAvailOut() const noexcept
		{
			return stream.avail_out;
		}

		void setNextIn(Bytef* in, uInt inSize) noexcept
		{
			stream.next_in = in;
			stream.avail_in = inSize;
		}

		uInt getAvailIn() const noexcept
		{
			return stream.avail_in;
		}

	private:
		z_stream stream{};
	};

	ZLibContext(bool compress, std::vector<byte> &data);
	
	void appendLump(const void *data, int length) noexcept(false);
	void finishLump() noexcept(false);
	
private:

	std::vector<byte> &out_data;
	tl::optional<Compression> compression;
	Bytef out_buffer[1024] = {};
};

ZLibContext::Compression::Compression()
{
	int result = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
	if (result != Z_OK)
		throw Exception(result, "Trouble setting up zlib compression");
}

ZLibContext::Compression::~Compression()
{
	int result = deflateEnd(&stream);
	if (result != Z_OK)
		gLog.printf("Error ending zlib compression: %d\n", result);
}

ZLibContext::ZLibContext(bool compress, std::vector<byte> &data) : out_data(data)
{
	if(!compress)
		return;
	
	compression.emplace();
	compression->setNextOut(out_buffer, sizeof(out_buffer));
}

void ZLibContext::appendLump(const void *data, int length) noexcept(false)
{
	if (! compression)
	{
		auto bdata = static_cast<const byte *>(data);
		out_data.insert(out_data.end(), bdata, bdata + length);
		return;
	}

	compression->setNextIn(const_cast<Bytef*>(static_cast<const Bytef*>(data)), length);

	while (compression->getAvailIn() > 0)
	{
		int err = compression->deflate(Z_NO_FLUSH);
		if (err != Z_OK)
		{
			throw Compression::Exception(err, SString::printf("Trouble compressing %d bytes (zlib)", length));
		}

		if (compression->getAvailOut() == 0)
		{
			out_data.insert(out_data.end(), out_buffer, out_buffer + sizeof(out_buffer));

			compression->setNextOut(out_buffer, sizeof(out_buffer));
		}
	}
}

void ZLibContext::finishLump() noexcept(false)
{
	if (! compression)
	{
		return;
	}

	int left_over;

	// ASSERT(zout_stream.avail_out > 0)

	compression->setNextIn(Z_NULL, 0);

	for (;;)
	{
		int err = compression->deflate(Z_FINISH);

		if (err == Z_STREAM_END)
			break;

		if (err != Z_OK)
		{
			throw Compression::Exception(err, "Trouble finishing compression (zlib)");
		}

		if (compression->getAvailOut() == 0)
		{
			out_data.insert(out_data.end(), out_buffer, out_buffer + sizeof(out_buffer));

			compression->setNextOut(out_buffer, sizeof(out_buffer));
		}
	}

	left_over = sizeof(out_buffer) - compression->getAvailOut();

	if (left_over > 0)
	{
		out_data.insert(out_data.end(), out_buffer, out_buffer + left_over);
	}
}

static const uint8_t *lev_XNOD_magic = (uint8_t *) "XNOD";
static const uint8_t *lev_XGL3_magic = (uint8_t *) "XGL3";
static const uint8_t *lev_ZNOD_magic = (uint8_t *) "ZNOD";

void LevelData::PutZVertices(ZLibContext &zcontext) const noexcept(false)
{
	int count, i;

	uint32_t orgverts = LE_U32(num_old_vert);
	uint32_t newverts = LE_U32(num_new_vert);

	zcontext.appendLump(&orgverts, 4);
	zcontext.appendLump(&newverts, 4);

	for (i=0, count=0 ; i < (int)vertices.size() ; i++)
	{
		raw_v2_vertex_t raw;

		const vertex_t *vert = vertices[i];

		if (! vert->is_new)
			continue;

		raw.x = LE_S32(iround(vert->x * 65536.0));
		raw.y = LE_S32(iround(vert->y * 65536.0));

		zcontext.appendLump(&raw, sizeof(raw));

		count++;
	}

	if (count != num_new_vert)
		BugError("PutZVertices miscounted (%d != %d)", count, num_new_vert);
}


void LevelData::PutZSubsecs(ZLibContext &zcontext) const noexcept(false)
{
	int i;
	int count;
	uint32_t raw_num = LE_U32((int)subsecs.size());

	int cur_seg_index = 0;

	zcontext.appendLump(&raw_num, 4);

	for (i=0 ; i < (int)subsecs.size() ; i++)
	{
		subsec_t *sub = subsecs[i];
		seg_t *seg;

		raw_num = LE_U32(sub->seg_count);

		zcontext.appendLump(&raw_num, 4);

		// sanity check the seg index values
		count = 0;
		for (seg = sub->seg_list ; seg ; seg = seg->next, cur_seg_index++)
		{
			if (cur_seg_index != seg->index)
			{
				BugError("PutZSubsecs: seg index mismatch in sub %d (%d != %d)", i, cur_seg_index,
						 seg->index);
			}

			count++;
		}

		if (count != sub->seg_count)
			BugError("PutZSubsecs: miscounted segs in sub %d (%d != %d)", i, count, sub->seg_count);
	}

	if (cur_seg_index != (int)segs.size())
		BugError("PutZSubsecs miscounted segs (%d != %d)", cur_seg_index, (int)segs.size());
}


void LevelData::PutZSegs(ZLibContext &zcontext) const noexcept(false)
{
	int i, count;
	uint32_t raw_num = LE_U32((int)segs.size());

	zcontext.appendLump(&raw_num, 4);

	for (i=0, count=0 ; i < (int)segs.size() ; i++)
	{
		seg_t *seg = segs[i];

		if (count != seg->index)
			BugError("PutZSegs: seg index mismatch (%d != %d)", count, seg->index);

		{
			uint32_t v1 = LE_U32(VertexIndex_XNOD(seg->start));
			uint32_t v2 = LE_U32(VertexIndex_XNOD(seg->end));

			uint16_t line = LE_U16(seg->linedef);
			uint8_t  side = static_cast<uint8_t>(seg->side);

			zcontext.appendLump(&v1,   4);
			zcontext.appendLump(&v2,   4);
			zcontext.appendLump(&line, 2);
			zcontext.appendLump(&side, 1);
		}

		count++;
	}

	if (count != (int)segs.size())
		BugError("PutZSegs miscounted (%d != %d)\n", count, (int)segs.size());
}


void LevelData::PutXGL3Segs(ZLibContext &zcontext) const noexcept(false)
{
	int i, count;
	uint32_t raw_num = LE_U32((int)segs.size());

	zcontext.appendLump(&raw_num, 4);

	for (i=0, count=0 ; i < (int)segs.size() ; i++)
	{
		seg_t *seg = segs[i];

		if (count != seg->index)
		{
			BugError("PutXGL3Segs: seg index mismatch (%d != %d)\n", count, seg->index);
		}

		{
			uint32_t v1   = LE_U32(VertexIndex_XNOD(seg->start));
			uint32_t partner = LE_U32(seg->partner ? seg->partner->index : -1);
			uint32_t line = LE_U32(seg->linedef);
			uint8_t  side = static_cast<uint8_t>(seg->side);

# if DEBUG_BSP
			fprintf(stderr, "SEG[%d] v1=%d partner=%d line=%d side=%d\n", i, v1, partner, line, side);
# endif

			zcontext.appendLump(&v1,      4);
			zcontext.appendLump(&partner, 4);
			zcontext.appendLump(&line,    4);
			zcontext.appendLump(&side,    1);
		}

		count++;
	}

	if (count != (int)segs.size())
	{
		BugError("PutXGL3Segs miscounted (%d != %d)\n", count, (int)segs.size());
	}
}


void LevelData::PutOneZNode(ZLibContext &zcontext, node_t *node, bool do_xgl3) noexcept(false)
{
	raw_v5_node_t raw;

	if (node->r.node)
		PutOneZNode(zcontext, node->r.node, do_xgl3);

	if (node->l.node)
		PutOneZNode(zcontext, node->l.node, do_xgl3);

	node->index = node_cur_index++;

	if (do_xgl3)
	{
		uint32_t x  = LE_S32(iround(node->x  * 65536.0));
		uint32_t y  = LE_S32(iround(node->y  * 65536.0));
		uint32_t dx = LE_S32(iround(node->dx * 65536.0));
		uint32_t dy = LE_S32(iround(node->dy * 65536.0));

		zcontext.appendLump(&x,  4);
		zcontext.appendLump(&y,  4);
		zcontext.appendLump(&dx, 4);
		zcontext.appendLump(&dy, 4);
	}
	else
	{
		raw.x  = LE_S16(iround(node->x));
		raw.y  = LE_S16(iround(node->y));
		raw.dx = LE_S16(iround(node->dx));
		raw.dy = LE_S16(iround(node->dy));

		zcontext.appendLump(&raw.x,  2);
		zcontext.appendLump(&raw.y,  2);
		zcontext.appendLump(&raw.dx, 2);
		zcontext.appendLump(&raw.dy, 2);
	}

	raw.b1.minx = LE_S16(node->r.bounds.minx);
	raw.b1.miny = LE_S16(node->r.bounds.miny);
	raw.b1.maxx = LE_S16(node->r.bounds.maxx);
	raw.b1.maxy = LE_S16(node->r.bounds.maxy);

	raw.b2.minx = LE_S16(node->l.bounds.minx);
	raw.b2.miny = LE_S16(node->l.bounds.miny);
	raw.b2.maxx = LE_S16(node->l.bounds.maxx);
	raw.b2.maxy = LE_S16(node->l.bounds.maxy);

	zcontext.appendLump(&raw.b1, sizeof(raw.b1));
	zcontext.appendLump(&raw.b2, sizeof(raw.b2));

	if (node->r.node)
		raw.right = LE_U32(node->r.node->index);
	else if (node->r.subsec)
		raw.right = LE_U32(node->r.subsec->index | 0x80000000U);
	else
	{
		BugError("Bad right child in V5 node %d\n", node->index);
	}

	if (node->l.node)
		raw.left = LE_U32(node->l.node->index);
	else if (node->l.subsec)
		raw.left = LE_U32(node->l.subsec->index | 0x80000000U);
	else
		BugError("Bad left child in V5 node %d\n", node->index);

	zcontext.appendLump(&raw.right, 4);
	zcontext.appendLump(&raw.left,  4);

# if DEBUG_BSP
	gLog.debugPrintf("PUT Z NODE %08X  Left %08X  Right %08X  "
			"(%d,%d) -> (%d,%d)\n", node->index, LE_U32(raw.left),
			LE_U32(raw.right), node->x, node->y,
			node->x + node->dx, node->y + node->dy);
# endif
}


void LevelData::PutZNodes(ZLibContext &zcontext, node_t *root, bool do_xgl3) noexcept(false)
{
	uint32_t raw_num = LE_U32((int)nodes.size());

	zcontext.appendLump(&raw_num, 4);

	node_cur_index = 0;

	if (root)
		PutOneZNode(zcontext, root, do_xgl3);

	if (node_cur_index != (int)nodes.size())
	{
		BugError("PutZNodes miscounted (%d != %d)\n", node_cur_index, (int)nodes.size());
	}
}

void LevelData::putZItems(std::vector<byte>& lumpData, node_t* root_node, bool do_xgl3)
{
	auto putTheStuff = [this, root_node, do_xgl3](ZLibContext &zlibContext)
		{
			PutZVertices(zlibContext);
			PutZSubsecs(zlibContext);
			if (do_xgl3)
				PutXGL3Segs(zlibContext);
			else
				PutZSegs(zlibContext);
			PutZNodes(zlibContext, root_node, do_xgl3);
		};

	try
	{
		ZLibContext zlibContext(cur_info->force_compress, lumpData);
		putTheStuff(zlibContext);
		zlibContext.finishLump();
	}
	catch (const ZLibContext::Compression::Exception& e)
	{
		PrintMsg("Cannot compress nodes: %s\n", e.what());
		lumpData.clear();

		ZLibContext zlibContext(false, lumpData);
		putTheStuff(zlibContext);
		zlibContext.finishLump();
	}
}

void LevelData::SaveZDFormat(node_t *root_node)
{

	// the ZLibXXX functions do no compression for XNOD format
	
	std::vector<byte> lumpData;
	putZItems(lumpData, root_node, false);
	
	// Commit
	
	// leave SEGS and SSECTORS empty
	CreateLevelLump("SEGS");
	CreateLevelLump("SSECTORS");

	Lump_c &lump = CreateLevelLump("NODES");

	if (cur_info->force_compress)
		lump.Write(lev_ZNOD_magic, 4);
	else
		lump.Write(lev_XNOD_magic, 4);
	lump.Write(lumpData.data(), (int)lumpData.size());
}


void LevelData::SaveXGL3Format(node_t *root_node)
{
	// WISH : compute a max_size

	Lump_c &lump = CreateLevelLump("ZNODES");

	lump.Write(lev_XGL3_magic, 4);

	// disable compression
	cur_info->force_compress = false;

	std::vector<byte> lumpData;
	putZItems(lumpData, root_node, true);
	
	lump.Write(lumpData.data(), (int)lumpData.size());
}


/* ----- whole-level routines --------------------------- */

void LevelData::LoadLevel()
{
	const Lump_c *LEV = wad.GetLump(current_start);

	current_name = LEV->Name();
	overflows = 0;

	PrintMsg("Building nodes on %s\n", current_name.c_str());

	num_new_vert = 0;
	num_real_lines = 0;

	GetVertices();

	for(auto &L : doc.linedefs)
	{
		if (L->right >= 0 || L->left >= 0)
			num_real_lines++;

		// init some fake flags
		L->flags &= ~(MLF_IS_PRECIOUS | MLF_IS_OVERLAP);

		if (L->tag >= 900 && L->tag < 1000)
			L->flags |= MLF_IS_PRECIOUS;
	}

	PrintDetail("Loaded %d vertices, %d sectors, %d sides, %d lines, %d things\n",
			doc.numVertices(), doc.numSectors(), doc.numSidedefs(), doc.numLinedefs(), doc.numThings());

	DetectOverlappingVertices();
	DetectOverlappingLines(doc);

	CalculateWallTips();

	if (format != MapFormat::doom)
	{
		// -JL- Find sectors containing polyobjs
		DetectPolyobjSectors();
	}
}


void LevelData::FreeLevel(void)
{
	FreeVertices();
	FreeSegs();
	FreeSubsecs();
	FreeNodes();
	FreeWallTips();
}

uint32_t LevelData::CalcGLChecksum() const
{
	uint32_t crc;

	Adler32_Begin(&crc);

	const Lump_c *lump = FindLevelLump("VERTEXES");

	if (lump && lump->Length() > 0)
	{
		const uint8_t *data = lump->getData().data();
		Adler32_AddBlock(&crc, data, lump->Length());
	}

	lump = FindLevelLump("LINEDEFS");

	if (lump && lump->Length() > 0)
	{
		const uint8_t *data = lump->getData().data();
		Adler32_AddBlock(&crc, data, lump->Length());
	}

	Adler32_Finish(&crc);

	return crc;
}


void LevelData::UpdateGLMarker(Lump_c *marker) const
{
	// we *must* compute the checksum BEFORE (re)creating the lump
	// [ otherwise we write data into the wrong part of the file ]
	uint32_t crc = CalcGLChecksum();

	// when original name is long, need to specify it here
	if (current_name.length() > 5)
	{
		marker->Printf("LEVEL=%s\n", current_name.c_str());
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


void LevelData::AddMissingLump(const char *name, const char *after)
{
	if (wad.LevelLookupLump(current_idx, name) >= 0)
		return;

	int exist = wad.LevelLookupLump(current_idx, after);

	// if this happens, the level structure is very broken
	if (exist < 0)
	{
		Warning("Missing %s lump -- level structure is broken\n", after);

		exist = wad.LevelLastLump(current_idx);
	}

	wad.InsertPoint(exist + 1);

	wad.AddLump(name);
}

build_result_e LevelData::SaveLevel(node_t *root_node)
{
	// Note: root_node may be NULL

	// remove any existing GL-Nodes
	wad.RemoveGLNodes(current_idx);

	// ensure all necessary level lumps are present
	AddMissingLump("SEGS",     "VERTEXES");
	AddMissingLump("SSECTORS", "SEGS");
	AddMissingLump("NODES",    "SSECTORS");
	AddMissingLump("REJECT",   "SECTORS");
	AddMissingLump("BLOCKMAP", "REJECT");

	// user preferences
	bool force_v5   = cur_info->force_v5;
	bool force_xnod = cur_info->force_xnod;

	// check for overflows...
	// this sets the force_xxx vars if certain limits are breached
	CheckLimits(force_v5, force_xnod);


	/* --- GL Nodes --- */

	Lump_c * gl_marker = NULL;

	if (cur_info->gl_nodes && num_real_lines > 0)
	{
		SortSegs();

		// create empty marker now, flesh it out later
		gl_marker = &CreateGLMarker();

		PutGLVertices(force_v5);

		if (force_v5)
			PutGLSegs_V5();
		else
			PutGLSegs();

		if (force_v5)
			PutGLSubsecs_V5();
		else
			PutSubsecs("GL_SSECT", true);

		PutNodes("GL_NODES", force_v5, root_node);

		// -JL- Add empty PVS lump
		CreateLevelLump("GL_PVS");
	}


	/* --- Normal nodes --- */

	// remove all the mini-segs from subsectors
	NormaliseBspTree();

	if (force_xnod && num_real_lines > 0)
	{
		SortSegs();

		SaveZDFormat(root_node);
	}
	else
	{
		// reduce vertex precision for classic DOOM nodes.
		// some segs can become "degenerate" after this, and these
		// are removed from subsectors.
		RoundOffBspTree();

		// this also removes minisegs and degenerate segs
		SortSegs();

		PutVertices("VERTEXES", false);

		PutSegs();
		PutSubsecs("SSECTORS", false);
		PutNodes("NODES", false, root_node);
	}

	PutBlockmap();
	PutReject();

	// keyword support (v5.0 of the specs).
	// must be done *after* doing normal nodes (for proper checksum).
	if (gl_marker)
	{
		UpdateGLMarker(gl_marker);
	}

	if (overflows > 0)
	{
		cur_info->total_failed_maps++;
		PrintMsg("FAILED with %d overflowed lumps\n", overflows);

		return BUILD_LumpOverflow;
	}

	return BUILD_OK;
}


build_result_e LevelData::SaveUDMF(node_t *root_node)
{
	// remove any existing ZNODES lump
	wad.RemoveZNodes(current_idx);

	try
	{
		if (num_real_lines >= 0)
		{
			SortSegs();

			SaveXGL3Format(root_node);
		}
	}
	catch (const std::runtime_error& e)
	{
		gLog.printf("Failed building UDMF nodes: %s\n", e.what());
		throw;
	}

	if (overflows > 0)
	{
		cur_info->total_failed_maps++;
		PrintMsg("FAILED with %d overflowed lumps\n", overflows);

		return BUILD_LumpOverflow;
	}

	return BUILD_OK;
}

/* ---------------------------------------------------------------- */


Lump_c * LevelData::FindLevelLump(const char *name) const noexcept
{
	int idx = wad.LevelLookupLump(current_idx, name);

	if (idx < 0)
		return NULL;

	return wad.GetLump(idx);
}


Lump_c & LevelData::CreateLevelLump(const char *name) const
{
	// look for existing one
	Lump_c *lump = FindLevelLump(name);

	if(!lump)
	{
		int last_idx = wad.LevelLastLump(current_idx);

		// in UDMF maps, insert before the ENDMAP lump, otherwise insert
		// after the last known lump of the level.
		if (format != MapFormat::udmf)
			last_idx++;

		wad.InsertPoint(last_idx);

		lump = &wad.AddLump(name);
	}

    lump->clearData();
	return *lump;
}


Lump_c & LevelData::CreateGLMarker() const
{
	SString name_buf;

	if (current_name.length() <= 5)
	{
		name_buf = "GL_" + current_name;
	}
	else
	{
		// names longer than 5 chars use "GL_LEVEL" as marker name
		name_buf = "GL_LEVEL";
	}

	int last_idx = wad.LevelLastLump(current_idx);

	wad.InsertPoint(last_idx + 1);

	return wad.AddLump(name_buf);
}


//------------------------------------------------------------------------
// MAIN STUFF
//------------------------------------------------------------------------

build_result_e LevelData::BuildLevel(nodebuildinfo_t *info, int lev_idx)
{
	cur_info = info;

	node_t *root_node  = NULL;
	subsec_t *root_sub = NULL;
	bbox_t root_bbox;

	if (cur_info->cancelled)
		return BUILD_Cancelled;

	current_idx   = lev_idx;
	current_start = wad.LevelHeader(lev_idx);

	LoadLevel();

	block.InitMap(doc);


	build_result_e ret = BUILD_OK;

	if (num_real_lines > 0)
	{
		// create initial segs
		seg_t *list = CreateSegs();

		// recursively create nodes
		ret = BuildNodes(list, &root_bbox, &root_node, &root_sub, 0);
	}

	if (ret == BUILD_OK)
	{
		PrintDetail("Built %d NODES, %d SSECTORS, %d SEGS, %d VERTEXES\n",
					(int)nodes.size(), (int)subsecs.size(), (int)segs.size(), num_old_vert + num_new_vert);

		if (root_node)
		{
			PrintDetail("Heights of left and right subtrees = (%d,%d)\n",
					ComputeBspHeight(root_node->r.node),
					ComputeBspHeight(root_node->l.node));
		}

		ClockwiseBspTree();

		try
		{
			if (format == MapFormat::udmf)
				ret = SaveUDMF(root_node);
			else
				ret = SaveLevel(root_node);
		}
		catch(const std::runtime_error &e)
		{
			PrintMsg("Failed saving after node-build: %s\n", e.what());
			ret = BUILD_BadFile;
		}
	}
	else
	{
		/* build was Cancelled by the user */
	}

	FreeLevel();
	FreeQuickAllocCuts();

	// clear some fake line flags
	for(auto &linedef : doc.linedefs)
		linedef->flags &= ~(MLF_IS_PRECIOUS | MLF_IS_OVERLAP);

	return ret;
}

}  // namespace ajbsp


build_result_e AJBSP_BuildLevel(nodebuildinfo_t *info, int lev_idx, Instance &inst, const Document &doc, const LoadingData& loading, Wad_file& wad)
{
	ajbsp::LevelData lev_data(loading.levelFormat, wad, doc, inst.conf, [&inst](const SString &message){
		inst.GB_PrintMsg("%s", message.c_str());
	});
	return lev_data.BuildLevel(info, lev_idx);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
