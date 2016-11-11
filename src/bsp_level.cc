//------------------------------------------------------------------------
// BLOCKMAP : Generate the blockmap
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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


namespace glbsp
{

#define DEBUG_BLOCKMAP  0


static int block_x, block_y;
static int block_w, block_h;
static int block_count;

static int block_mid_x = 0;
static int block_mid_y = 0;

static uint16_g ** block_lines;

static uint16_g *block_ptrs;
static uint16_g *block_dups;

static int block_compression;
static int block_overflowed;

#define DUMMY_DUP  0xFFFF


//
// GetBlockmapBounds
//
void GetBlockmapBounds(int *x, int *y, int *w, int *h)
{
  *x = block_x; *y = block_y;
  *w = block_w; *h = block_h;
}

//
// CheckLinedefInsideBox
//
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
        return FALSE;
        
      x1 = x1 + (int) ((x2-x1) * (double)(ymax-y1) / (double)(y2-y1));
      y1 = ymax;
      
      count = 2;
      continue;
    }

    if (y1 < ymin)
    {
      if (y2 < ymin)
        return FALSE;
      
      x1 = x1 + (int) ((x2-x1) * (double)(ymin-y1) / (double)(y2-y1));
      y1 = ymin;
      
      count = 2;
      continue;
    }

    if (x1 > xmax)
    {
      if (x2 > xmax)
        return FALSE;
        
      y1 = y1 + (int) ((y2-y1) * (double)(xmax-x1) / (double)(x2-x1));
      x1 = xmax;

      count = 2;
      continue;
    }

    if (x1 < xmin)
    {
      if (x2 < xmin)
        return FALSE;
        
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
  return TRUE;
}


/* ----- create blockmap ------------------------------------ */

#define BK_NUM    0
#define BK_MAX    1
#define BK_XOR    2
#define BK_FIRST  3

#define BK_QUANTUM  32

static void BlockAdd(int blk_num, int line_index)
{
  uint16_g *cur = block_lines[blk_num];

# if DEBUG_BLOCKMAP
  PrintDebug("Block %d has line %d\n", blk_num, line_index);
# endif

  if (blk_num < 0 || blk_num >= block_count)
    InternalError("BlockAdd: bad block number %d", blk_num);
    
  if (! cur)
  {
    // create empty block
    block_lines[blk_num] = cur = (uint16_g *)UtilCalloc(BK_QUANTUM * sizeof(uint16_g));
    cur[BK_NUM] = 0;
    cur[BK_MAX] = BK_QUANTUM;
    cur[BK_XOR] = 0x1234;
  }

  if (BK_FIRST + cur[BK_NUM] == cur[BK_MAX])
  {
    // no more room, so allocate some more...
    cur[BK_MAX] += BK_QUANTUM;

    block_lines[blk_num] = cur = (uint16_g *)UtilRealloc(cur, cur[BK_MAX] * sizeof(uint16_g));
  }

  // compute new checksum
  cur[BK_XOR] = ((cur[BK_XOR] << 4) | (cur[BK_XOR] >> 12)) ^ line_index;

  cur[BK_FIRST + cur[BK_NUM]] = UINT16(line_index);
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
  PrintDebug("BlockAddLine: %d (%d,%d) -> (%d,%d)\n", line_index, 
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
    for (bx=bx1; bx <= bx2; bx++)
    {
      int blk_num = by1 * block_w + bx;
      BlockAdd(blk_num, line_index);
    }
    return;
  }

  // handle simple case #2: completely vertical
  if (bx1 == bx2)
  {
    for (by=by1; by <= by2; by++)
    {
      int blk_num = by * block_w + bx1;
      BlockAdd(blk_num, line_index);
    }
    return;
  }

  // handle the rest (diagonals)

  for (by=by1; by <= by2; by++)
  for (bx=bx1; bx <= bx2; bx++)
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

  block_lines = (uint16_g **) UtilCalloc(block_count * sizeof(uint16_g *));

  DisplayTicker();

  for (i=0; i < num_linedefs; i++)
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
  int blk_num1 = ((const uint16_g *) p1)[0];
  int blk_num2 = ((const uint16_g *) p2)[0];

  const uint16_g *A = block_lines[blk_num1];
  const uint16_g *B = block_lines[blk_num2];

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
 
  return memcmp(A+BK_FIRST, B+BK_FIRST, A[BK_NUM] * sizeof(uint16_g));
}

static void CompressBlockmap(void)
{
  int i;
  int cur_offset;
  int dup_count=0;

  int orig_size, new_size;

  block_ptrs = (uint16_g *)UtilCalloc(block_count * sizeof(uint16_g));
  block_dups = (uint16_g *)UtilCalloc(block_count * sizeof(uint16_g));

  DisplayTicker();

  // sort duplicate-detecting array.  After the sort, all duplicates
  // will be next to each other.  The duplicate array gives the order
  // of the blocklists in the BLOCKMAP lump.
  
  for (i=0; i < block_count; i++)
    block_dups[i] = i;

  qsort(block_dups, block_count, sizeof(uint16_g), BlockCompare);

  // scan duplicate array and build up offset array

  cur_offset = 4 + block_count + 2;

  orig_size = 4 + block_count;
  new_size  = cur_offset;

  DisplayTicker();

  for (i=0; i < block_count; i++)
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
    MarkSoftFailure(LIMIT_BLOCKMAP);
    block_overflowed = TRUE;
    return;
  }

# if DEBUG_BLOCKMAP
  PrintDebug("Blockmap: Last ptr = %d  duplicates = %d\n", 
      cur_offset, dup_count);
# endif

  block_compression = (orig_size - new_size) * 100 / orig_size;

  // there's a tiny chance of new_size > orig_size
  if (block_compression < 0)
    block_compression = 0;
}


static void WriteBlockmap(void)
{
  int i;
  
  raw_blockmap_header_t header;

  lump_t *lump = CreateLevelLump("BLOCKMAP");

  uint16_g null_block[2] = { 0x0000, 0xFFFF };
  uint16_g m_zero = 0x0000;
  uint16_g m_neg1 = 0xFFFF;
  
  // leave empty if the blockmap overflowed
  if (block_overflowed)
    return;

  // fill in header
  header.x_origin = UINT16(block_x);
  header.y_origin = UINT16(block_y);
  header.x_blocks = UINT16(block_w);
  header.y_blocks = UINT16(block_h);
  
  AppendLevelLump(lump, &header, sizeof(header));

  // handle pointers
  for (i=0; i < block_count; i++)
  {
    uint16_g ptr = UINT16(block_ptrs[i]);

    if (ptr == 0)
      InternalError("WriteBlockmap: offset %d not set.", i);

    AppendLevelLump(lump, &ptr, sizeof(uint16_g));
  }

  // add the null block which _all_ empty blocks will use
  AppendLevelLump(lump, null_block, sizeof(null_block));

  // handle each block list
  for (i=0; i < block_count; i++)
  {
    int blk_num = block_dups[i];
    uint16_g *blk;

    // ignore duplicate or empty blocks
    if (blk_num == DUMMY_DUP)
      continue;

    blk = block_lines[blk_num];

    if (blk == NULL)
      InternalError("WriteBlockmap: block %d is NULL !", i);

    AppendLevelLump(lump, &m_zero, sizeof(uint16_g));
    AppendLevelLump(lump, blk + BK_FIRST, blk[BK_NUM] * sizeof(uint16_g));
    AppendLevelLump(lump, &m_neg1, sizeof(uint16_g));
  }
}


static void FreeBlockmap(void)
{
  int i;

  for (i=0; i < block_count; i++)
  {
    if (block_lines[i])
      UtilFree(block_lines[i]);
  }

  UtilFree(block_lines);
  UtilFree(block_ptrs);
  UtilFree(block_dups);
}


/* ----- top level funcs ------------------------------------ */

static void FindBlockmapLimits(bbox_t *bbox)
{
  int i;

  int mid_x = 0;
  int mid_y = 0;

  bbox->minx = bbox->miny = SHRT_MAX;
  bbox->maxx = bbox->maxy = SHRT_MIN;

  for (i=0; i < num_linedefs; i++)
  {
    linedef_t *L = LookupLinedef(i);

    if (! L->zero_len)
    {
      float_g x1 = L->start->x;
      float_g y1 = L->start->y;
      float_g x2 = L->end->x;
      float_g y2 = L->end->y;

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
  PrintDebug("Blockmap lines centered at (%d,%d)\n", block_mid_x, block_mid_y);
# endif
}

//
// TruncateBlockmap
//
static void TruncateBlockmap(void)
{
  while (block_w * block_h > cur_info->block_limit)
  {
    block_w -= block_w / 8;
    block_h -= block_h / 8;
  }

  block_count = block_w * block_h;

  PrintMiniWarn("Blockmap TOO LARGE!  Truncated to %dx%d blocks\n",
      block_w, block_h);

  MarkSoftFailure(LIMIT_BMAP_TRUNC);

  /* center the truncated blockmap */
  block_x = block_mid_x - block_w * 64;
  block_y = block_mid_y - block_h * 64;

# if DEBUG_BLOCKMAP
  PrintDebug("New blockmap origin: (%d,%d)\n", block_x, block_y);
# endif
}

//
// InitBlockmap
//
void InitBlockmap(void)
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

//
// PutBlockmap
//
void PutBlockmap(void)
{
  block_overflowed = FALSE;

  // truncate blockmap if too large.  We're limiting the number of
  // blocks to around 16000 (user changeable), this leaves about 48K
  // of shorts for the actual line lists.
 
  if (block_count > cur_info->block_limit)
  {
    MarkSoftFailure(LIMIT_BLOCKMAP);
    block_overflowed = TRUE;
  }

  // initial phase: create internal blockmap containing the index of
  // all lines in each block.
  
  CreateBlockmap();

  // -AJA- second phase: compress the blockmap.  We do this by sorting
  //       the blocks, which is a typical way to detect duplicates in
  //       a large list.

  CompressBlockmap();
 
  // final phase: write it out in the correct format

  WriteBlockmap();

  if (block_overflowed)
    PrintVerbose("Blockmap overflowed (lump will be empty)\n");
  else
    PrintVerbose("Completed blockmap, size %dx%d (compression: %d%%)\n",
        block_w, block_h, block_compression);

  FreeBlockmap();
}


}  // namespace glbsp
//------------------------------------------------------------------------
// REJECT : Generate the reject table
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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


namespace glbsp
{

#define DEBUG_REJECT  0


//
// InitReject
//
// Puts each sector into individual groups.
// 
static void InitReject(void)
{
  int i;

  for (i=0; i < num_sectors; i++)
  {
    sector_t *sec = LookupSector(i);

    sec->rej_group = i;
    sec->rej_next = sec->rej_prev = sec;
  }
}

//
// GroupSectors
//
// Algorithm: Initially all sectors are in individual groups.  Now we
// scan the linedef list.  For each 2-sectored line, merge the two
// sector groups into one.  That's it !
//
static void GroupSectors(void)
{
  int i;

  for (i=0; i < num_linedefs; i++)
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

    for (tmp=sec2->rej_next; tmp != sec2; tmp=tmp->rej_next)
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
static void CountGroups(void)
{
  // Note: this routine is destructive to the group numbers
  
  int i;

  for (i=0; i < num_sectors; i++)
  {
    sector_t *sec = LookupSector(i);
    sector_t *tmp;

    int group = sec->rej_group;
    int num = 0;

    if (group < 0)
      continue;

    sec->rej_group = -1;
    num++;

    for (tmp=sec->rej_next; tmp != sec; tmp=tmp->rej_next)
    {
      tmp->rej_group = -1;
      num++;
    }

    PrintDebug("Group %d  Sectors %d\n", group, num);
  }
}
#endif

//
// CreateReject
//
static void CreateReject(uint8_g *matrix)
{
  int view, target;

  for (view=0; view < num_sectors; view++)
  for (target=0; target < view; target++)
  {
    sector_t *view_sec = LookupSector(view);
    sector_t *targ_sec = LookupSector(target);

    int p1, p2;

    if (view_sec->rej_group == targ_sec->rej_group)
      continue;

    // for symmetry, do two bits at a time

    p1 = view * num_sectors + target;
    p2 = target * num_sectors + view;
    
    matrix[p1 >> 3] |= (1 << (p1 & 7));
    matrix[p2 >> 3] |= (1 << (p2 & 7));
  }
}

//
// PutReject
//
// For now we only do very basic reject processing, limited to
// determining all isolated groups of sectors (islands that are
// surrounded by void space).
//
void PutReject(void)
{
  int reject_size;
  uint8_g *matrix;
  lump_t *lump;

  DisplayTicker();

  InitReject();
  GroupSectors();
  
  reject_size = (num_sectors * num_sectors + 7) / 8;
  matrix = (uint8_g *)UtilCalloc(reject_size);

  CreateReject(matrix);

# if DEBUG_REJECT
  CountGroups();
# endif

  lump = CreateLevelLump("REJECT");

  AppendLevelLump(lump, matrix, reject_size);

  PrintVerbose("Added simple reject lump\n");

  UtilFree(matrix);
}


}  // namespace glbsp
//------------------------------------------------------------------------
// LEVEL : Level structure read/write functions.
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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
//  ZDBSP format support based on code (C) 2002,2003 Randy Heit
// 
//------------------------------------------------------------------------


namespace glbsp
{

#define DEBUG_LOAD      0
#define DEBUG_BSP       0

#define ALLOC_BLKNUM  1024


// per-level variables

boolean_g lev_doing_normal;
boolean_g lev_doing_hexen;

static boolean_g lev_force_v3;
static boolean_g lev_force_v5;


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
  for (i=0; i < NUMVAR; i++)  \
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


//
// CheckForNormalNodes
//
int CheckForNormalNodes(void)
{
  lump_t *lump;
  
  /* Note: an empty NODES lump can be valid */
  if (FindLevelLump("NODES") == NULL)
    return FALSE;
 
  lump = FindLevelLump("SEGS");
  
  if (! lump || lump->length == 0 || CheckLevelLumpZero(lump))
    return FALSE;

  lump = FindLevelLump("SSECTORS");
  
  if (! lump || lump->length == 0 || CheckLevelLumpZero(lump))
    return FALSE;

  return TRUE;
}

//
// GetVertices
//
void GetVertices(void)
{
  int i, count=-1;
  raw_vertex_t *raw;
  lump_t *lump = FindLevelLump("VERTEXES");

  if (lump)
    count = lump->length / sizeof(raw_vertex_t);

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetVertices: num = %d\n", count);
# endif

  if (!lump || count == 0)
    FatalError("Couldn't find any Vertices");

  raw = (raw_vertex_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    vertex_t *vert = NewVertex();

    vert->x = (float_g) SINT16(raw->x);
    vert->y = (float_g) SINT16(raw->y);

    vert->index = i;
  }

  num_normal_vert = num_vertices;
  num_gl_vert = 0;
  num_complete_seg = 0;
}

//
// GetSectors
//
void GetSectors(void)
{
  int i, count=-1;
  raw_sector_t *raw;
  lump_t *lump = FindLevelLump("SECTORS");

  if (lump)
    count = lump->length / sizeof(raw_sector_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Sectors");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetSectors: num = %d\n", count);
# endif

  raw = (raw_sector_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    sector_t *sector = NewSector();

    sector->floor_h = SINT16(raw->floor_h);
    sector->ceil_h  = SINT16(raw->ceil_h);

    memcpy(sector->floor_tex, raw->floor_tex, sizeof(sector->floor_tex));
    memcpy(sector->ceil_tex,  raw->ceil_tex,  sizeof(sector->ceil_tex));

    sector->light = UINT16(raw->light);
    sector->special = UINT16(raw->special);
    sector->tag = SINT16(raw->tag);

    sector->coalesce = (sector->tag >= 900 && sector->tag < 1000) ?
        TRUE : FALSE;

    /* sector indices never change */
    sector->index = i;

    sector->warned_facing = -1;

    /* Note: rej_* fields are handled completely in reject.c */
  }
}

//
// GetThings
//
void GetThings(void)
{
  int i, count=-1;
  raw_thing_t *raw;
  lump_t *lump = FindLevelLump("THINGS");

  if (lump)
    count = lump->length / sizeof(raw_thing_t);

  if (!lump || count == 0)
  {
    // Note: no error if no things exist, even though technically a map
    // will be unplayable without the player starts.
    PrintWarn("Couldn't find any Things!\n");
    return;
  }

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetThings: num = %d\n", count);
# endif

  raw = (raw_thing_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    thing_t *thing = NewThing();

    thing->x = SINT16(raw->x);
    thing->y = SINT16(raw->y);

    thing->type = UINT16(raw->type);
    thing->options = UINT16(raw->options);

    thing->index = i;
  }
}

//
// GetThingsHexen
//
void GetThingsHexen(void)
{
  int i, count=-1;
  raw_hexen_thing_t *raw;
  lump_t *lump = FindLevelLump("THINGS");

  if (lump)
    count = lump->length / sizeof(raw_hexen_thing_t);

  if (!lump || count == 0)
  {
    // Note: no error if no things exist, even though technically a map
    // will be unplayable without the player starts.
    PrintWarn("Couldn't find any Things!\n");
    return;
  }

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetThingsHexen: num = %d\n", count);
# endif

  raw = (raw_hexen_thing_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    thing_t *thing = NewThing();

    thing->x = SINT16(raw->x);
    thing->y = SINT16(raw->y);

    thing->type = UINT16(raw->type);
    thing->options = UINT16(raw->options);

    thing->index = i;
  }
}

//
// GetSidedefs
//
void GetSidedefs(void)
{
  int i, count=-1;
  raw_sidedef_t *raw;
  lump_t *lump = FindLevelLump("SIDEDEFS");

  if (lump)
    count = lump->length / sizeof(raw_sidedef_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Sidedefs");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetSidedefs: num = %d\n", count);
# endif

  raw = (raw_sidedef_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    sidedef_t *side = NewSidedef();

    side->sector = (SINT16(raw->sector) == -1) ? NULL :
        LookupSector(UINT16(raw->sector));

    if (side->sector)
      side->sector->ref_count++;

    side->x_offset = SINT16(raw->x_offset);
    side->y_offset = SINT16(raw->y_offset);

    memcpy(side->upper_tex, raw->upper_tex, sizeof(side->upper_tex));
    memcpy(side->lower_tex, raw->lower_tex, sizeof(side->lower_tex));
    memcpy(side->mid_tex,   raw->mid_tex,   sizeof(side->mid_tex));

    /* sidedef indices never change */
    side->index = i;
  }
}

static inline sidedef_t *SafeLookupSidedef(uint16_g num)
{
  if (num == 0xFFFF)
    return NULL;

  if ((int)num >= num_sidedefs && (sint16_g)(num) < 0)
    return NULL;

  return LookupSidedef(num);
}

//
// GetLinedefs
//
void GetLinedefs(void)
{
  int i, count=-1;
  raw_linedef_t *raw;
  lump_t *lump = FindLevelLump("LINEDEFS");

  if (lump)
    count = lump->length / sizeof(raw_linedef_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Linedefs");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetLinedefs: num = %d\n", count);
# endif

  raw = (raw_linedef_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    linedef_t *line;

    vertex_t *start = LookupVertex(UINT16(raw->start));
    vertex_t *end   = LookupVertex(UINT16(raw->end));

    start->ref_count++;
    end->ref_count++;

    line = NewLinedef();

    line->start = start;
    line->end   = end;

    /* check for zero-length line */
    line->zero_len = (fabs(start->x - end->x) < DIST_EPSILON) && 
        (fabs(start->y - end->y) < DIST_EPSILON);

    line->flags = UINT16(raw->flags);
    line->type = UINT16(raw->type);
    line->tag  = SINT16(raw->tag);

    line->two_sided = (line->flags & LINEFLAG_TWO_SIDED) ? TRUE : FALSE;
    line->is_precious = (line->tag >= 900 && line->tag < 1000) ? 
        TRUE : FALSE;

    line->right = SafeLookupSidedef(UINT16(raw->sidedef1));
    line->left  = SafeLookupSidedef(UINT16(raw->sidedef2));

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

//
// GetLinedefsHexen
//
void GetLinedefsHexen(void)
{
  int i, j, count=-1;
  raw_hexen_linedef_t *raw;
  lump_t *lump = FindLevelLump("LINEDEFS");

  if (lump)
    count = lump->length / sizeof(raw_hexen_linedef_t);

  if (!lump || count == 0)
    FatalError("Couldn't find any Linedefs");

  DisplayTicker();

# if DEBUG_LOAD
  PrintDebug("GetLinedefsHexen: num = %d\n", count);
# endif

  raw = (raw_hexen_linedef_t *) lump->data;

  for (i=0; i < count; i++, raw++)
  {
    linedef_t *line;

    vertex_t *start = LookupVertex(UINT16(raw->start));
    vertex_t *end   = LookupVertex(UINT16(raw->end));

    start->ref_count++;
    end->ref_count++;

    line = NewLinedef();

    line->start = start;
    line->end   = end;

    // check for zero-length line
    line->zero_len = (fabs(start->x - end->x) < DIST_EPSILON) && 
        (fabs(start->y - end->y) < DIST_EPSILON);

    line->flags = UINT16(raw->flags);
    line->type = UINT8(raw->type);
    line->tag  = 0;

    /* read specials */
    for (j=0; j < 5; j++)
      line->specials[j] = UINT8(raw->specials[j]);

    // -JL- Added missing twosided flag handling that caused a broken reject
    line->two_sided = (line->flags & LINEFLAG_TWO_SIDED) ? TRUE : FALSE;

    line->right = SafeLookupSidedef(UINT16(raw->sidedef1));
    line->left  = SafeLookupSidedef(UINT16(raw->sidedef2));

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
  float_g sx = seg->side ? seg->linedef->end->x : seg->linedef->start->x;
  float_g sy = seg->side ? seg->linedef->end->y : seg->linedef->start->y;

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
    InternalError("Seg %p never reached a subsector !", A);

  if (B->index < 0)
    InternalError("Seg %p never reached a subsector !", B);

  return (A->index - B->index);
}


/* ----- writing routines ------------------------------ */

static const uint8_g *lev_v2_magic = (uint8_g *) "gNd2";
static const uint8_g *lev_v3_magic = (uint8_g *) "gNd3";
static const uint8_g *lev_v5_magic = (uint8_g *) "gNd5";

void PutVertices(const char *name, int do_gl)
{
  int count, i;
  lump_t *lump;

  DisplayTicker();

  if (do_gl)
    lump = CreateGLLump(name);
  else
    lump = CreateLevelLump(name);

  for (i=0, count=0; i < num_vertices; i++)
  {
    raw_vertex_t raw;
    vertex_t *vert = lev_vertices[i];

    if ((do_gl ? 1 : 0) != ((vert->index & IS_GL_VERTEX) ? 1 : 0))
    {
      continue;
    }

    raw.x = SINT16(I_ROUND(vert->x));
    raw.y = SINT16(I_ROUND(vert->y));

    AppendLevelLump(lump, &raw, sizeof(raw));

    count++;
  }

  if (count != (do_gl ? num_gl_vert : num_normal_vert))
    InternalError("PutVertices miscounted (%d != %d)", count,
      do_gl ? num_gl_vert : num_normal_vert);

  if (lev_doing_normal && ! do_gl && count > 65534)
    MarkHardFailure(LIMIT_VERTEXES);
  else if (count > 32767)
    MarkSoftFailure(do_gl ? LIMIT_GL_VERT : LIMIT_VERTEXES);
}

void PutV2Vertices(int do_v5)
{
  int count, i;
  lump_t *lump;

  DisplayTicker();

  lump = CreateGLLump("GL_VERT");

  if (do_v5)
      AppendLevelLump(lump, lev_v5_magic, 4);
  else
      AppendLevelLump(lump, lev_v2_magic, 4);

  for (i=0, count=0; i < num_vertices; i++)
  {
    raw_v2_vertex_t raw;
    vertex_t *vert = lev_vertices[i];

    if (! (vert->index & IS_GL_VERTEX))
      continue;

    raw.x = SINT32((int)(vert->x * 65536.0));
    raw.y = SINT32((int)(vert->y * 65536.0));

    AppendLevelLump(lump, &raw, sizeof(raw));

    count++;
  }

  if (count != num_gl_vert)
    InternalError("PutV2Vertices miscounted (%d != %d)", count,
      num_gl_vert);

  if (count > 32767)
    MarkSoftFailure(LIMIT_GL_VERT);
}

void PutSectors(void)
{
  int i;
  lump_t *lump = CreateLevelLump("SECTORS");

  DisplayTicker();

  for (i=0; i < num_sectors; i++)
  {
    raw_sector_t raw;
    sector_t *sector = lev_sectors[i];

    raw.floor_h = SINT16(sector->floor_h);
    raw.ceil_h  = SINT16(sector->ceil_h);

    memcpy(raw.floor_tex, sector->floor_tex, sizeof(raw.floor_tex));
    memcpy(raw.ceil_tex,  sector->ceil_tex,  sizeof(raw.ceil_tex));

    raw.light = UINT16(sector->light);
    raw.special = UINT16(sector->special);
    raw.tag   = SINT16(sector->tag);

    AppendLevelLump(lump, &raw, sizeof(raw));
  }

  if (num_sectors > 65534)
    MarkHardFailure(LIMIT_SECTORS);
  else if (num_sectors > 32767)
    MarkSoftFailure(LIMIT_SECTORS);
}

void PutSidedefs(void)
{
  int i;
  lump_t *lump = CreateLevelLump("SIDEDEFS");

  DisplayTicker();

  for (i=0; i < num_sidedefs; i++)
  {
    raw_sidedef_t raw;
    sidedef_t *side = lev_sidedefs[i];

    raw.sector = (side->sector == NULL) ? SINT16(-1) :
        UINT16(side->sector->index);

    raw.x_offset = SINT16(side->x_offset);
    raw.y_offset = SINT16(side->y_offset);

    memcpy(raw.upper_tex, side->upper_tex, sizeof(raw.upper_tex));
    memcpy(raw.lower_tex, side->lower_tex, sizeof(raw.lower_tex));
    memcpy(raw.mid_tex,   side->mid_tex,   sizeof(raw.mid_tex));
 
    AppendLevelLump(lump, &raw, sizeof(raw));
  }

  if (num_sidedefs > 65534)
    MarkHardFailure(LIMIT_SIDEDEFS);
  else if (num_sidedefs > 32767)
    MarkSoftFailure(LIMIT_SIDEDEFS);
}

void PutLinedefs(void)
{
  int i;
  lump_t *lump = CreateLevelLump("LINEDEFS");

  DisplayTicker();

  for (i=0; i < num_linedefs; i++)
  {
    raw_linedef_t raw;
    linedef_t *line = lev_linedefs[i];

    raw.start = UINT16(line->start->index);
    raw.end   = UINT16(line->end->index);

    raw.flags = UINT16(line->flags);
    raw.type  = UINT16(line->type);
    raw.tag   = SINT16(line->tag);

    raw.sidedef1 = line->right ? UINT16(line->right->index) : 0xFFFF;
    raw.sidedef2 = line->left  ? UINT16(line->left->index)  : 0xFFFF;

    AppendLevelLump(lump, &raw, sizeof(raw));
  }

  if (num_linedefs > 65534)
    MarkHardFailure(LIMIT_LINEDEFS);
  else if (num_linedefs > 32767)
    MarkSoftFailure(LIMIT_LINEDEFS);
}

void PutLinedefsHexen(void)
{
  int i, j;
  lump_t *lump = CreateLevelLump("LINEDEFS");

  DisplayTicker();

  for (i=0; i < num_linedefs; i++)
  {
    raw_hexen_linedef_t raw;
    linedef_t *line = lev_linedefs[i];

    raw.start = UINT16(line->start->index);
    raw.end   = UINT16(line->end->index);

    raw.flags = UINT16(line->flags);
    raw.type  = UINT8(line->type);

    // write specials
    for (j=0; j < 5; j++)
      raw.specials[j] = UINT8(line->specials[j]);

    raw.sidedef1 = line->right ? UINT16(line->right->index) : 0xFFFF;
    raw.sidedef2 = line->left  ? UINT16(line->left->index)  : 0xFFFF;

    AppendLevelLump(lump, &raw, sizeof(raw));
  }

  if (num_linedefs > 65534)
    MarkHardFailure(LIMIT_LINEDEFS);
  else if (num_linedefs > 32767)
    MarkSoftFailure(LIMIT_LINEDEFS);
}

static inline uint16_g VertexIndex16Bit(const vertex_t *v)
{
  if (v->index & IS_GL_VERTEX)
    return (uint16_g) ((v->index & ~IS_GL_VERTEX) | 0x8000U);

  return (uint16_g) v->index;
}

static inline uint32_g VertexIndex32BitV5(const vertex_t *v)
{
  if (v->index & IS_GL_VERTEX)
    return (uint32_g) ((v->index & ~IS_GL_VERTEX) | 0x80000000U);

  return (uint32_g) v->index;
}

void PutSegs(void)
{
  int i, count;
  lump_t *lump = CreateLevelLump("SEGS");

  DisplayTicker();

  // sort segs into ascending index
  qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

  for (i=0, count=0; i < num_segs; i++)
  {
    raw_seg_t raw;
    seg_t *seg = segs[i];

    // ignore minisegs and degenerate segs
    if (! seg->linedef || seg->degenerate)
      continue;

    raw.start   = UINT16(VertexIndex16Bit(seg->start));
    raw.end     = UINT16(VertexIndex16Bit(seg->end));
    raw.angle   = UINT16(TransformAngle(seg->p_angle));
    raw.linedef = UINT16(seg->linedef->index);
    raw.flip    = UINT16(seg->side);
    raw.dist    = UINT16(TransformSegDist(seg));

    AppendLevelLump(lump, &raw, sizeof(raw));

    count++;

#   if DEBUG_BSP
    PrintDebug("PUT SEG: %04X  Vert %04X->%04X  Line %04X %s  "
        "Angle %04X  (%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index,
        UINT16(raw.start), UINT16(raw.end), UINT16(raw.linedef), 
        seg->side ? "L" : "R", UINT16(raw.angle), 
        seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
  }

  if (count != num_complete_seg)
    InternalError("PutSegs miscounted (%d != %d)", count,
      num_complete_seg);

  if (count > 65534)
    MarkHardFailure(LIMIT_SEGS);
  else if (count > 32767)
    MarkSoftFailure(LIMIT_SEGS);
}

void PutGLSegs(void)
{
  int i, count;
  lump_t *lump = CreateGLLump("GL_SEGS");

  DisplayTicker();

  // sort segs into ascending index
  qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

  for (i=0, count=0; i < num_segs; i++)
  {
    raw_gl_seg_t raw;
    seg_t *seg = segs[i];

    // ignore degenerate segs
    if (seg->degenerate)
      continue;

    raw.start = UINT16(VertexIndex16Bit(seg->start));
    raw.end   = UINT16(VertexIndex16Bit(seg->end));
    raw.side  = UINT16(seg->side);

    if (seg->linedef)
      raw.linedef = UINT16(seg->linedef->index);
    else
      raw.linedef = UINT16(0xFFFF);

    if (seg->partner)
      raw.partner = UINT16(seg->partner->index);
    else
      raw.partner = UINT16(0xFFFF);

    AppendLevelLump(lump, &raw, sizeof(raw));

    count++;

#   if DEBUG_BSP
    PrintDebug("PUT GL SEG: %04X  Line %04X %s  Partner %04X  "
      "(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, UINT16(raw.linedef), 
      seg->side ? "L" : "R", UINT16(raw.partner),
      seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
  }

  if (count != num_complete_seg)
    InternalError("PutGLSegs miscounted (%d != %d)", count,
      num_complete_seg);

  if (count > 65534)
    InternalError("PutGLSegs with %d (> 65534) segs", count);
  else if (count > 32767)
    MarkSoftFailure(LIMIT_GL_SEGS);
}

void PutV3Segs(int do_v5)
{
  int i, count;
  lump_t *lump = CreateGLLump("GL_SEGS");

  if (! do_v5)
      AppendLevelLump(lump, lev_v3_magic, 4);

  DisplayTicker();

  // sort segs into ascending index
  qsort(segs, num_segs, sizeof(seg_t *), SegCompare);

  for (i=0, count=0; i < num_segs; i++)
  {
    raw_v3_seg_t raw;
    seg_t *seg = segs[i];

    // ignore degenerate segs
    if (seg->degenerate)
      continue;

    if (do_v5)
    {
      raw.start = UINT32(VertexIndex32BitV5(seg->start));
      raw.end   = UINT32(VertexIndex32BitV5(seg->end));
    }
    else
    {
      raw.start = UINT32(seg->start->index);
      raw.end   = UINT32(seg->end->index);
    }

    raw.side  = UINT16(seg->side);

    if (seg->linedef)
      raw.linedef = UINT16(seg->linedef->index);
    else
      raw.linedef = UINT16(0xFFFF);

    if (seg->partner)
      raw.partner = UINT32(seg->partner->index);
    else
      raw.partner = UINT32(0xFFFFFFFF);

    AppendLevelLump(lump, &raw, sizeof(raw));

    count++;

#   if DEBUG_BSP
    PrintDebug("PUT V3 SEG: %06X  Line %04X %s  Partner %06X  "
      "(%1.1f,%1.1f) -> (%1.1f,%1.1f)\n", seg->index, UINT16(raw.linedef), 
      seg->side ? "L" : "R", UINT32(raw.partner),
      seg->start->x, seg->start->y, seg->end->x, seg->end->y);
#   endif
  }

  if (count != num_complete_seg)
    InternalError("PutGLSegs miscounted (%d != %d)", count,
      num_complete_seg);
}

void PutSubsecs(const char *name, int do_gl)
{
  int i;
  lump_t *lump;

  DisplayTicker();

  if (do_gl)
    lump = CreateGLLump(name);
  else
    lump = CreateLevelLump(name);

  for (i=0; i < num_subsecs; i++)
  {
    raw_subsec_t raw;
    subsec_t *sub = subsecs[i];

    raw.first = UINT16(sub->seg_list->index);
    raw.num   = UINT16(sub->seg_count);

    AppendLevelLump(lump, &raw, sizeof(raw));

#   if DEBUG_BSP
    PrintDebug("PUT SUBSEC %04X  First %04X  Num %04X\n",
      sub->index, UINT16(raw.first), UINT16(raw.num));
#   endif
  }

  if (num_subsecs > 32767)
    MarkHardFailure(do_gl ? LIMIT_GL_SSECT : LIMIT_SSECTORS);
}

void PutV3Subsecs(int do_v5)
{
  int i;
  lump_t *lump;

  DisplayTicker();

  lump = CreateGLLump("GL_SSECT");

  if (! do_v5)
      AppendLevelLump(lump, lev_v3_magic, 4);

  for (i=0; i < num_subsecs; i++)
  {
    raw_v3_subsec_t raw;
    subsec_t *sub = subsecs[i];

    raw.first = UINT32(sub->seg_list->index);
    raw.num   = UINT32(sub->seg_count);

    AppendLevelLump(lump, &raw, sizeof(raw));

#   if DEBUG_BSP
    PrintDebug("PUT V3 SUBSEC %06X  First %06X  Num %06X\n",
      sub->index, UINT32(raw.first), UINT32(raw.num));
#   endif
  }

  if (!do_v5 && num_subsecs > 32767)
    MarkHardFailure(LIMIT_GL_SSECT);
}

static int node_cur_index;

static void PutOneNode(node_t *node, lump_t *lump)
{
  raw_node_t raw;

  if (node->r.node)
    PutOneNode(node->r.node, lump);

  if (node->l.node)
    PutOneNode(node->l.node, lump);

  node->index = node_cur_index++;

  raw.x  = SINT16(node->x);
  raw.y  = SINT16(node->y);
  raw.dx = SINT16(node->dx / (node->too_long ? 2 : 1));
  raw.dy = SINT16(node->dy / (node->too_long ? 2 : 1));

  raw.b1.minx = SINT16(node->r.bounds.minx);
  raw.b1.miny = SINT16(node->r.bounds.miny);
  raw.b1.maxx = SINT16(node->r.bounds.maxx);
  raw.b1.maxy = SINT16(node->r.bounds.maxy);

  raw.b2.minx = SINT16(node->l.bounds.minx);
  raw.b2.miny = SINT16(node->l.bounds.miny);
  raw.b2.maxx = SINT16(node->l.bounds.maxx);
  raw.b2.maxy = SINT16(node->l.bounds.maxy);

  if (node->r.node)
    raw.right = UINT16(node->r.node->index);
  else if (node->r.subsec)
    raw.right = UINT16(node->r.subsec->index | 0x8000);
  else
    InternalError("Bad right child in node %d", node->index);

  if (node->l.node)
    raw.left = UINT16(node->l.node->index);
  else if (node->l.subsec)
    raw.left = UINT16(node->l.subsec->index | 0x8000);
  else
    InternalError("Bad left child in node %d", node->index);

  AppendLevelLump(lump, &raw, sizeof(raw));

# if DEBUG_BSP
  PrintDebug("PUT NODE %04X  Left %04X  Right %04X  "
    "(%d,%d) -> (%d,%d)\n", node->index, UINT16(raw.left),
    UINT16(raw.right), node->x, node->y,
    node->x + node->dx, node->y + node->dy);
# endif
}

static void PutOneV5Node(node_t *node, lump_t *lump)
{
  raw_v5_node_t raw;

  if (node->r.node)
    PutOneV5Node(node->r.node, lump);

  if (node->l.node)
    PutOneV5Node(node->l.node, lump);

  node->index = node_cur_index++;

  raw.x  = SINT16(node->x);
  raw.y  = SINT16(node->y);
  raw.dx = SINT16(node->dx / (node->too_long ? 2 : 1));
  raw.dy = SINT16(node->dy / (node->too_long ? 2 : 1));

  raw.b1.minx = SINT16(node->r.bounds.minx);
  raw.b1.miny = SINT16(node->r.bounds.miny);
  raw.b1.maxx = SINT16(node->r.bounds.maxx);
  raw.b1.maxy = SINT16(node->r.bounds.maxy);

  raw.b2.minx = SINT16(node->l.bounds.minx);
  raw.b2.miny = SINT16(node->l.bounds.miny);
  raw.b2.maxx = SINT16(node->l.bounds.maxx);
  raw.b2.maxy = SINT16(node->l.bounds.maxy);

  if (node->r.node)
    raw.right = UINT32(node->r.node->index);
  else if (node->r.subsec)
    raw.right = UINT32(node->r.subsec->index | 0x80000000U);
  else
    InternalError("Bad right child in V5 node %d", node->index);

  if (node->l.node)
    raw.left = UINT32(node->l.node->index);
  else if (node->l.subsec)
    raw.left = UINT32(node->l.subsec->index | 0x80000000U);
  else
    InternalError("Bad left child in V5 node %d", node->index);

  AppendLevelLump(lump, &raw, sizeof(raw));

# if DEBUG_BSP
  PrintDebug("PUT V5 NODE %08X  Left %08X  Right %08X  "
    "(%d,%d) -> (%d,%d)\n", node->index, UINT32(raw.left),
    UINT32(raw.right), node->x, node->y,
    node->x + node->dx, node->y + node->dy);
# endif
}

void PutNodes(const char *name, int do_gl, int do_v5, node_t *root)
{
  lump_t *lump;

  DisplayTicker();

  if (do_gl)
    lump = CreateGLLump(name);
  else
    lump = CreateLevelLump(name);

  node_cur_index = 0;

  if (root)
  {
    if (do_v5)
      PutOneV5Node(root, lump);
    else
      PutOneNode(root, lump);
  }

  if (node_cur_index != num_nodes)
    InternalError("PutNodes miscounted (%d != %d)",
      node_cur_index, num_nodes);

  if (!do_v5 && node_cur_index > 32767)
    MarkHardFailure(LIMIT_NODES);
}


/* ----- ZDBSP format writing --------------------------- */

static const uint8_g *lev_ZD_magic = (uint8_g *) "ZNOD";

void PutZVertices(void)
{
  int count, i;

  uint32_g orgverts = UINT32(num_normal_vert);
  uint32_g newverts = UINT32(num_gl_vert);

  ZLibAppendLump(&orgverts, 4);
  ZLibAppendLump(&newverts, 4);

  DisplayTicker();

  for (i=0, count=0; i < num_vertices; i++)
  {
    raw_v2_vertex_t raw;
    vertex_t *vert = lev_vertices[i];

    if (! (vert->index & IS_GL_VERTEX))
      continue;

    raw.x = SINT32((int)(vert->x * 65536.0));
    raw.y = SINT32((int)(vert->y * 65536.0));

    ZLibAppendLump(&raw, sizeof(raw));

    count++;
  }

  if (count != num_gl_vert)
    InternalError("PutZVertices miscounted (%d != %d)",
        count, num_gl_vert);
}

void PutZSubsecs(void)
{
  int i;
  int count;
  uint32_g raw_num = UINT32(num_subsecs);

  int cur_seg_index = 0;

  ZLibAppendLump(&raw_num, 4);
  DisplayTicker();

  for (i=0; i < num_subsecs; i++)
  {
    subsec_t *sub = subsecs[i];
    seg_t *seg;

    raw_num = UINT32(sub->seg_count);

    ZLibAppendLump(&raw_num, 4);

    // sanity check the seg index values
    count = 0;
    for (seg = sub->seg_list; seg; seg = seg->next, cur_seg_index++)
    {
      // ignore minisegs and degenerate segs
      if (! seg->linedef || seg->degenerate)
        continue;

      if (cur_seg_index != seg->index)
        InternalError("PutZSubsecs: seg index mismatch in sub %d (%d != %d)\n",
            i, cur_seg_index, seg->index);
      
      count++;
    }

    if (count != sub->seg_count)
      InternalError("PutZSubsecs: miscounted segs in sub %d (%d != %d)\n",
          i, count, sub->seg_count);
  }

  if (cur_seg_index != num_complete_seg)
    InternalError("PutZSubsecs miscounted segs (%d != %d)",
        cur_seg_index, num_complete_seg);
}

void PutZSegs(void)
{
  int i, count;
  uint32_g raw_num = UINT32(num_complete_seg);

  ZLibAppendLump(&raw_num, 4);
  DisplayTicker();

  for (i=0, count=0; i < num_segs; i++)
  {
    seg_t *seg = segs[i];

    // ignore minisegs and degenerate segs
    if (! seg->linedef || seg->degenerate)
      continue;

    if (count != seg->index)
      InternalError("PutZSegs: seg index mismatch (%d != %d)\n",
          count, seg->index);

    {
      uint32_g v1 = UINT32(VertexIndex32BitV5(seg->start));
      uint32_g v2 = UINT32(VertexIndex32BitV5(seg->end));

      uint16_g line = UINT16(seg->linedef->index);
      uint8_g  side = seg->side;

      ZLibAppendLump(&v1,   4);
      ZLibAppendLump(&v2,   4);
      ZLibAppendLump(&line, 2);
      ZLibAppendLump(&side, 1);
    }

    count++;
  }

  if (count != num_complete_seg)
    InternalError("PutZSegs miscounted (%d != %d)",
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

  raw.x  = SINT16(node->x);
  raw.y  = SINT16(node->y);
  raw.dx = SINT16(node->dx / (node->too_long ? 2 : 1));
  raw.dy = SINT16(node->dy / (node->too_long ? 2 : 1));

  ZLibAppendLump(&raw.x,  2);
  ZLibAppendLump(&raw.y,  2);
  ZLibAppendLump(&raw.dx, 2);
  ZLibAppendLump(&raw.dy, 2);

  raw.b1.minx = SINT16(node->r.bounds.minx);
  raw.b1.miny = SINT16(node->r.bounds.miny);
  raw.b1.maxx = SINT16(node->r.bounds.maxx);
  raw.b1.maxy = SINT16(node->r.bounds.maxy);

  raw.b2.minx = SINT16(node->l.bounds.minx);
  raw.b2.miny = SINT16(node->l.bounds.miny);
  raw.b2.maxx = SINT16(node->l.bounds.maxx);
  raw.b2.maxy = SINT16(node->l.bounds.maxy);

  ZLibAppendLump(&raw.b1, sizeof(raw.b1));
  ZLibAppendLump(&raw.b2, sizeof(raw.b2));

  if (node->r.node)
    raw.right = UINT32(node->r.node->index);
  else if (node->r.subsec)
    raw.right = UINT32(node->r.subsec->index | 0x80000000U);
  else
    InternalError("Bad right child in V5 node %d", node->index);

  if (node->l.node)
    raw.left = UINT32(node->l.node->index);
  else if (node->l.subsec)
    raw.left = UINT32(node->l.subsec->index | 0x80000000U);
  else
    InternalError("Bad left child in V5 node %d", node->index);

  ZLibAppendLump(&raw.right, 4);
  ZLibAppendLump(&raw.left,  4);

# if DEBUG_BSP
  PrintDebug("PUT Z NODE %08X  Left %08X  Right %08X  "
    "(%d,%d) -> (%d,%d)\n", node->index, UINT32(raw.left),
    UINT32(raw.right), node->x, node->y,
    node->x + node->dx, node->y + node->dy);
# endif
}

void PutZNodes(node_t *root)
{
  uint32_g raw_num = UINT32(num_nodes);

  ZLibAppendLump(&raw_num, 4);
  DisplayTicker();

  node_cur_index = 0;

  if (root)
    PutOneZNode(root);

  if (node_cur_index != num_nodes)
    InternalError("PutZNodes miscounted (%d != %d)",
      node_cur_index, num_nodes);
}

void SaveZDFormat(node_t *root_node)
{
  lump_t *lump;

  // leave SEGS and SSECTORS empty
  CreateLevelLump("SEGS");
  CreateLevelLump("SSECTORS");

  lump = CreateLevelLump("NODES");
 
  AppendLevelLump(lump, lev_ZD_magic, 4);

  ZLibBeginLump(lump);

  PutZVertices();
  PutZSubsecs();
  PutZSegs();
  PutZNodes(root_node);

  ZLibFinishLump();
}


/* ----- whole-level routines --------------------------- */

//
// LoadLevel
//
void LoadLevel(void)
{
  char *message;

  const char *level_name = GetLevelName();

  boolean_g normal_exists = CheckForNormalNodes();

  lev_doing_normal = !cur_info->gwa_mode && (cur_info->force_normal || 
    (!cur_info->no_normal && !normal_exists));

  // -JL- Identify Hexen mode by presence of BEHAVIOR lump
  lev_doing_hexen = (FindLevelLump("BEHAVIOR") != NULL);

  if (lev_doing_normal)
    message = UtilFormat("Building normal and GL nodes on %s%s",
        level_name, lev_doing_hexen ? " (Hexen)" : "");
  else
    message = UtilFormat("Building GL nodes on %s%s",
        level_name, lev_doing_hexen ? " (Hexen)" : "");
 
  lev_doing_hexen |= cur_info->force_hexen;

  DisplaySetBarText(1, message);

  PrintVerbose("\n\n");
  PrintMsg("%s\n", message);
  PrintVerbose("\n");

  UtilFree(message);

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

  if (lev_doing_normal)
  {
    // NOTE: order here is critical

    if (cur_info->pack_sides)
      DetectDuplicateSidedefs();

    if (cur_info->merge_vert)
      DetectDuplicateVertices();

    if (!cur_info->no_prune)
      PruneLinedefs();

    // always prune vertices (ignore -noprune), otherwise all the
    // unused vertices from seg splits would keep accumulating.
    PruneVertices();

    if (!cur_info->no_prune)
      PruneSidedefs();

    if (cur_info->prune_sect)
      PruneSectors();
  }
 
  CalculateWallTips();

  if (lev_doing_hexen)
  {
    // -JL- Find sectors containing polyobjs
    DetectPolyobjSectors();
  }

  DetectOverlappingLines();

  if (cur_info->window_fx)
    DetectWindowEffects();
}

//
// FreeLevel
//
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

//
// PutGLOptions
//
void PutGLOptions(void)
{
  char option_buf[128];

  sprintf(option_buf, "-v%d -factor %d", cur_info->spec_version, cur_info->factor);

  if (cur_info->fast         ) strcat(option_buf, " -f");
  if (cur_info->force_normal ) strcat(option_buf, " -n");
  if (cur_info->merge_vert   ) strcat(option_buf, " -m");
  if (cur_info->pack_sides   ) strcat(option_buf, " -p");
  if (cur_info->prune_sect   ) strcat(option_buf, " -u");
  if (cur_info->skip_self_ref) strcat(option_buf, " -s");
  if (cur_info->window_fx    ) strcat(option_buf, " -y");

  if (cur_info->no_normal) strcat(option_buf, " -xn");
  if (cur_info->no_reject) strcat(option_buf, " -xr");
  if (cur_info->no_prune ) strcat(option_buf, " -xu");

  AddGLTextLine("OPTIONS", option_buf);
}

//
// PutGLChecksum
//
void PutGLChecksum(void)
{
  uint32_g crc;
  lump_t *lump;
  char num_buf[64];

  Adler32_Begin(&crc);

  lump = FindLevelLump("VERTEXES");

  if (lump && lump->length > 0)
    Adler32_AddBlock(&crc, (uint8_g*) lump->data, lump->length);

  lump = FindLevelLump("LINEDEFS");

  if (lump && lump->length > 0)
    Adler32_AddBlock(&crc, (uint8_g*) lump->data, lump->length);

  Adler32_Finish(&crc);

  sprintf(num_buf, "0x%08x", crc);

  AddGLTextLine("CHECKSUM", num_buf);
}

//
// SaveLevel
//
void SaveLevel(node_t *root_node)
{
  lev_force_v3 = (cur_info->spec_version == 3) ? TRUE : FALSE;
  lev_force_v5 = (cur_info->spec_version == 5) ? TRUE : FALSE;
  
  // Note: RoundOffBspTree will convert the GL vertices in segs to
  // their normal counterparts (pointer change: use normal_dup).

  if (cur_info->spec_version == 1)
    RoundOffBspTree(root_node);

  // GL Nodes
  {
    if (num_normal_vert > 32767 || num_gl_vert > 32767)
    {
      if (cur_info->spec_version < 3)
      {
        lev_force_v5 = TRUE;
        MarkV5Switch(LIMIT_VERTEXES | LIMIT_GL_SEGS);
      }
    }

    if (num_segs > 65534)
    {
      if (cur_info->spec_version < 3)
      {
        lev_force_v5 = TRUE;
        MarkV5Switch(LIMIT_GL_SSECT | LIMIT_GL_SEGS);
      }
    }

    if (num_nodes > 32767)
    {
      if (cur_info->spec_version < 5)
      {
        lev_force_v5 = TRUE;
        MarkV5Switch(LIMIT_GL_NODES);
      }
    }

    if (cur_info->spec_version == 1)
      PutVertices("GL_VERT", TRUE);
    else
      PutV2Vertices(lev_force_v5);

    if (lev_force_v3 || lev_force_v5)
      PutV3Segs(lev_force_v5);
    else
      PutGLSegs();

    if (lev_force_v3 || lev_force_v5)
      PutV3Subsecs(lev_force_v5);
    else
      PutSubsecs("GL_SSECT", TRUE);

    PutNodes("GL_NODES", TRUE, lev_force_v5, root_node);

    // -JL- Add empty PVS lump
    CreateGLLump("GL_PVS");
  }

  if (lev_doing_normal)
  {
    if (cur_info->spec_version != 1)
      RoundOffBspTree(root_node);
 
    NormaliseBspTree(root_node);

    PutVertices("VERTEXES", FALSE);
    PutSectors();
    PutSidedefs();

    if (lev_doing_hexen)
      PutLinedefsHexen();
    else
      PutLinedefs();
 
    if (lev_force_v5)
    {
      // don't report a problem when -v5 was explicitly given
      if (cur_info->spec_version < 5)
        MarkZDSwitch();

      SaveZDFormat(root_node);
    }
    else
    {
      PutSegs();
      PutSubsecs("SSECTORS", FALSE);
      PutNodes("NODES", FALSE, FALSE, root_node);
    }

    // -JL- Don't touch blockmap and reject if not doing normal nodes
    PutBlockmap();

    if (!cur_info->no_reject || !FindLevelLump("REJECT"))
      PutReject();
  }

  // keyword support (v5.0 of the specs)
  AddGLTextLine("BUILDER", "glBSP " GLBSP_VER);
  PutGLOptions();
  {
    char *time_str = UtilTimeString();

    if (time_str)
    {
      AddGLTextLine("TIME", time_str);
      UtilFree(time_str);
    }
  }

  // this must be done _after_ the normal nodes have been built,
  // so that we use the new VERTEXES lump in the checksum.
  PutGLChecksum();
}


}  // namespace glbsp
//------------------------------------------------------------------------
// MAIN : Main program for glBSP
//------------------------------------------------------------------------
//
//  GL-Friendly Node Builder (C) 2000-2007 Andrew Apted
//
//  Based on 'BSP 2.3' by Colin Reed, Lee Killough and others.
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
 

namespace glbsp
{

const nodebuildinfo_t *cur_info = NULL;
const nodebuildfuncs_t *cur_funcs = NULL;
volatile nodebuildcomms_t *cur_comms = NULL;


const nodebuildinfo_t default_buildinfo =
{
  NULL,    // input_file
  NULL,    // output_file
  NULL,    // extra_files

  DEFAULT_FACTOR,  // factor

  FALSE,   // no_reject
  FALSE,   // no_progress
  FALSE,   // quiet
  FALSE,   // mini_warnings
  FALSE,   // force_hexen
  FALSE,   // pack_sides
  FALSE,   // fast

  2,   // spec_version

  FALSE,   // load_all
  FALSE,   // no_normal
  FALSE,   // force_normal
  FALSE,   // gwa_mode
  FALSE,   // prune_sect
  FALSE,   // no_prune
  FALSE,   // merge_vert
  FALSE,   // skip_self_ref
  FALSE,   // window_fx

  DEFAULT_BLOCK_LIMIT,   // block_limit

  FALSE,   // missing_output
  FALSE    // same_filenames
};

const nodebuildcomms_t default_buildcomms =
{
  NULL,    // message
  FALSE,   // cancelled

  0, 0,    // total warnings
  0, 0     // build and file positions
};


/* ----- option parsing ----------------------------- */

#define EXTRA_BLOCK  10  /* includes terminating NULL */

static void AddExtraFile(nodebuildinfo_t *info, const char *str)
{
  int count = 0;
  int space;

  if (! info->extra_files)
  {
    info->extra_files = (const char **)
        UtilCalloc(EXTRA_BLOCK * sizeof(const char *));

    info->extra_files[0] = str;
    info->extra_files[1] = NULL;

    return;
  }

  while (info->extra_files[count])
    count++;

  space = EXTRA_BLOCK - 1 - (count % EXTRA_BLOCK);

  if (space == 0)
  {
    info->extra_files = (const char **) UtilRealloc((void *)info->extra_files,
        (count + 1 + EXTRA_BLOCK) * sizeof(const char *));
  }

  info->extra_files[count]   = str;
  info->extra_files[count+1] = NULL;
}

#define HANDLE_BOOLEAN(name, field)  \
    if (UtilStrCaseCmp(opt_str, name) == 0)  \
    {  \
      info->field = TRUE;  \
      argv++; argc--;  \
      continue;  \
    }

#define HANDLE_BOOLEAN2(abbrev, name, field)  \
    HANDLE_BOOLEAN(abbrev, field)  \
    HANDLE_BOOLEAN(name, field)

glbsp_ret_e ParseArgs(nodebuildinfo_t *info, 
    volatile nodebuildcomms_t *comms,
    const char ** argv, int argc)
{
  const char *opt_str;
  int num_files = 0;
  int got_output = FALSE;

  cur_comms = comms;
  SetErrorMsg("(Unknown Problem)");

  while (argc > 0)
  {
    if (argv[0][0] != '-')
    {
      // --- ORDINARY FILENAME ---

      if (got_output)
      {
        SetErrorMsg("Input filenames must precede the -o option");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      if (UtilCheckExtension(argv[0], "gwa"))
      {
        SetErrorMsg("Input file cannot be GWA (contains nothing to build)");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      if (num_files >= 1)
      {
        AddExtraFile(info, GlbspStrDup(argv[0]));
      }
      else
      {
        GlbspFree(info->input_file);
        info->input_file = GlbspStrDup(argv[0]);
      }

      num_files++;

      argv++; argc--;
      continue;
    }

    // --- AN OPTION ---

    opt_str = &argv[0][1];

    // handle GNU style options beginning with '--'
    if (opt_str[0] == '-')
      opt_str++;

    if (UtilStrCaseCmp(opt_str, "o") == 0)
    {
      if (got_output)
      {
        SetErrorMsg("The -o option cannot be used more than once");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      if (num_files >= 2)
      {
        SetErrorMsg("Cannot use -o with multiple input files.");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      if (argc < 2 || argv[1][0] == '-')
      {
        SetErrorMsg("Missing filename for the -o option");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      GlbspFree(info->output_file);
      info->output_file = GlbspStrDup(argv[1]);

      got_output = TRUE;

      argv += 2; argc -= 2;
      continue;
    }

    if (UtilStrCaseCmp(opt_str, "factor") == 0 ||
        UtilStrCaseCmp(opt_str, "c") == 0)
    {
      if (argc < 2)
      {
        SetErrorMsg("Missing factor value");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      info->factor = (int) strtol(argv[1], NULL, 10);

      argv += 2; argc -= 2;
      continue;
    }

    if (tolower(opt_str[0]) == 'v' && isdigit(opt_str[1]))
    {
      info->spec_version = (opt_str[1] - '0');

      argv++; argc--;
      continue;
    }

    if (UtilStrCaseCmp(opt_str, "maxblock") == 0 ||
        UtilStrCaseCmp(opt_str, "b") == 0)
    {
      if (argc < 2)
      {
        SetErrorMsg("Missing maxblock value");
        cur_comms = NULL;
        return GLBSP_E_BadArgs;
      }

      info->block_limit = (int) strtol(argv[1], NULL, 10);

      argv += 2; argc -= 2;
      continue;
    }

    HANDLE_BOOLEAN2("q",  "quiet",      quiet)
    HANDLE_BOOLEAN2("f",  "fast",       fast)
    HANDLE_BOOLEAN2("w",  "warn",       mini_warnings)
    HANDLE_BOOLEAN2("p",  "pack",       pack_sides)
    HANDLE_BOOLEAN2("n",  "normal",     force_normal)
    HANDLE_BOOLEAN2("xr", "noreject",   no_reject)
    HANDLE_BOOLEAN2("xp", "noprog",     no_progress)

    HANDLE_BOOLEAN2("m",  "mergevert",   merge_vert)
    HANDLE_BOOLEAN2("u",  "prunesec",    prune_sect)
    HANDLE_BOOLEAN2("y",  "windowfx",    window_fx)
    HANDLE_BOOLEAN2("s",  "skipselfref", skip_self_ref)
    HANDLE_BOOLEAN2("xu", "noprune",     no_prune)
    HANDLE_BOOLEAN2("xn", "nonormal",    no_normal)

    // to err is human...
    HANDLE_BOOLEAN("noprogress",  no_progress)
    HANDLE_BOOLEAN("packsides",   pack_sides)
    HANDLE_BOOLEAN("prunesect",   prune_sect)

    // ignore these options for backwards compatibility
    if (UtilStrCaseCmp(opt_str, "fresh") == 0 ||
        UtilStrCaseCmp(opt_str, "keepdummy") == 0 ||
        UtilStrCaseCmp(opt_str, "keepsec") == 0 ||
        UtilStrCaseCmp(opt_str, "keepsect") == 0)
    {
      argv++; argc--;
      continue;
    }

    // backwards compatibility
    HANDLE_BOOLEAN("forcegwa",    gwa_mode)
    HANDLE_BOOLEAN("forcenormal", force_normal)
    HANDLE_BOOLEAN("loadall",     load_all)

    // The -hexen option is only kept for backwards compatibility
    HANDLE_BOOLEAN("hexen", force_hexen)

    SetErrorMsg("Unknown option: %s", argv[0]);

    cur_comms = NULL;
    return GLBSP_E_BadArgs;
  }

  cur_comms = NULL;
  return GLBSP_E_OK;
}

glbsp_ret_e CheckInfo(nodebuildinfo_t *info,
    volatile nodebuildcomms_t *comms)
{
  cur_comms = comms;
  SetErrorMsg("(Unknown Problem)");

  info->same_filenames = FALSE;
  info->missing_output = FALSE;

  if (!info->input_file || info->input_file[0] == 0)
  {
    SetErrorMsg("Missing input filename !");
    return GLBSP_E_BadArgs;
  }

  if (UtilCheckExtension(info->input_file, "gwa"))
  {
    SetErrorMsg("Input file cannot be GWA (contains nothing to build)");
    return GLBSP_E_BadArgs;
  }

  if (!info->output_file || info->output_file[0] == 0)
  {
    GlbspFree(info->output_file);
    info->output_file = GlbspStrDup(UtilReplaceExtension(
          info->input_file, "gwa"));

    info->gwa_mode = TRUE;
    info->missing_output = TRUE;
  }
  else  /* has output filename */
  {
    if (UtilCheckExtension(info->output_file, "gwa"))
      info->gwa_mode = TRUE;
  }

  if (UtilStrCaseCmp(info->input_file, info->output_file) == 0)
  {
    info->load_all = TRUE;
    info->same_filenames = TRUE;
  }

  if (info->no_prune && info->pack_sides)
  {
    info->pack_sides = FALSE;
    SetErrorMsg("-noprune and -packsides cannot be used together");
    return GLBSP_E_BadInfoFixed;
  }

  if (info->gwa_mode && info->force_normal)
  {
    info->force_normal = FALSE;
    SetErrorMsg("-forcenormal used, but GWA files don't have normal nodes");
    return GLBSP_E_BadInfoFixed;
  }
 
  if (info->no_normal && info->force_normal)
  {
    info->force_normal = FALSE;
    SetErrorMsg("-forcenormal and -nonormal cannot be used together");
    return GLBSP_E_BadInfoFixed;
  }
 
  if (info->factor <= 0 || info->factor > 32)
  {
    info->factor = DEFAULT_FACTOR;
    SetErrorMsg("Bad factor value !");
    return GLBSP_E_BadInfoFixed;
  }

  if (info->spec_version <= 0 || info->spec_version > 5)
  {
    info->spec_version = 2;
    SetErrorMsg("Bad GL-Nodes version number !");
    return GLBSP_E_BadInfoFixed;
  }
  else if (info->spec_version == 4)
  {
    info->spec_version = 5;
    SetErrorMsg("V4 GL-Nodes is not supported");
    return GLBSP_E_BadInfoFixed;
  }

  if (info->block_limit < 1000 || info->block_limit > 64000)
  {
    info->block_limit = DEFAULT_BLOCK_LIMIT;
    SetErrorMsg("Bad blocklimit value !");
    return GLBSP_E_BadInfoFixed;
  }

  return GLBSP_E_OK;
}


/* ----- memory functions --------------------------- */

const char *GlbspStrDup(const char *str)
{
  if (! str)
    return NULL;

  return UtilStrDup(str);
}

void GlbspFree(const char *str)
{
  if (! str)
    return;

  UtilFree((char *) str);
}


/* ----- build nodes for a single level --------------------------- */

static glbsp_ret_e HandleLevel(void)
{
  superblock_t *seg_list;
  bbox_t seg_bbox;

  node_t *root_node;
  subsec_t *root_sub;

  glbsp_ret_e ret;

  if (cur_comms->cancelled)
    return GLBSP_E_Cancelled;

  DisplaySetBarLimit(1, 1000);
  DisplaySetBar(1, 0);

  cur_comms->build_pos = 0;

  LoadLevel();

  InitBlockmap();

  // create initial segs
  seg_list = CreateSegs();

  FindLimits(seg_list, &seg_bbox);

  // recursively create nodes
  ret = BuildNodes(seg_list, &root_node, &root_sub, 0, &seg_bbox);
  FreeSuper(seg_list);

  if (ret == GLBSP_E_OK)
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


/* ----- main routine -------------------------------------- */

glbsp_ret_e BuildNodes(const nodebuildinfo_t *info,
    const nodebuildfuncs_t *funcs, volatile nodebuildcomms_t *comms)
{
  char *file_msg;

  glbsp_ret_e ret = GLBSP_E_OK;

  cur_info  = info;
  cur_funcs = funcs;
  cur_comms = comms;

  cur_comms->total_big_warn = 0;
  cur_comms->total_small_warn = 0;

  // clear cancelled flag
  comms->cancelled = FALSE;

  // sanity check
  if (!cur_info->input_file  || cur_info->input_file[0] == 0 ||
      !cur_info->output_file || cur_info->output_file[0] == 0)
  {
    SetErrorMsg("INTERNAL ERROR: Missing in/out filename !");
    return GLBSP_E_BadArgs;
  }

  InitDebug();
  InitEndian();
 
  if (info->missing_output)
    PrintMsg("* No output file specified. Using: %s\n\n", info->output_file);

  if (info->same_filenames)
    PrintMsg("* Output file is same as input file. Using -loadall\n\n");

  // opens and reads directory from the input wad
  ret = ReadWadFile(cur_info->input_file);

  if (ret != GLBSP_E_OK)
  {
    TermDebug();
    return ret;
  }

  if (CountLevels() <= 0)
  {
    CloseWads();
    TermDebug();

    SetErrorMsg("No levels found in wad !");
    return GLBSP_E_Unknown;
  }
   
  PrintMsg("\n");
  PrintVerbose("Creating nodes using tunable factor of %d\n", info->factor);

  DisplayOpen(DIS_BUILDPROGRESS);
  DisplaySetTitle("glBSP Build Progress");

  file_msg = UtilFormat("File: %s", cur_info->input_file);
 
  DisplaySetBarText(2, file_msg);
  DisplaySetBarLimit(2, CountLevels() * 10);
  DisplaySetBar(2, 0);

  UtilFree(file_msg);
  
  cur_comms->file_pos = 0;
  
  // loop over each level in the wad
  while (FindNextLevel())
  {
    ret = HandleLevel();

    if (ret != GLBSP_E_OK)
      break;

    cur_comms->file_pos += 10;
    DisplaySetBar(2, cur_comms->file_pos);
  }

  DisplayClose();

  // writes all the lumps to the output wad
  if (ret == GLBSP_E_OK)
  {
    ret = WriteWadFile(cur_info->output_file);

    // when modifying the original wad, any GWA companion must be deleted
    if (ret == GLBSP_E_OK && cur_info->same_filenames)
      DeleteGwaFile(cur_info->output_file);

    PrintMsg("\n");
    PrintMsg("Total serious warnings: %d\n", cur_comms->total_big_warn);
    PrintMsg("Total minor warnings: %d\n", cur_comms->total_small_warn);

    ReportFailedLevels();
  }

  // close wads and free memory
  CloseWads();

  TermDebug();

  cur_info  = NULL;
  cur_comms = NULL;
  cur_funcs = NULL;

  return ret;
}


}  // namespace glbsp
