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

#ifndef __EUREKA_BSP_H__
#define __EUREKA_BSP_H__

#include "lib_util.h"
#include "m_strings.h"
#include "sys_type.h"
#include "Thing.h"

#include <functional>
#include <vector>

struct ConfigData;
class Instance;
class Lump_c;
struct Sector;
enum class Side;
struct Document;
class Wad_file;
struct LoadingData;

// Node Build Information Structure
//
// Memory note: when changing the string values here (and in
// nodebuildcomms_t) they should be freed using StringFree() and
// allocated with StringDup().  The application has the final
// responsibility to free the strings in here.
//
#define DEFAULT_FACTOR  11

struct nodebuildinfo_t
{
	int factor = DEFAULT_FACTOR;

	bool gl_nodes = true;

	// when these two are false, they create an empty lump
	bool do_blockmap = true;
	bool do_reject = true;

	bool fast = false;
	bool warnings = false;

	bool force_v5 = false;
	bool force_xnod = false;
	bool force_compress = false;

	// the GUI can set this to tell the node builder to stop
	bool cancelled = false;

	// from here on, various bits of internal state
	int total_failed_maps = 0;
	int total_warnings = 0;
};


enum build_result_e
{
	// everything went peachy keen
	BUILD_OK = 0,

	// building was cancelled
	BUILD_Cancelled,

	// the WAD file was corrupt / empty / bad filename
	BUILD_BadFile,

	// when saving the map, one or more lumps overflowed
	BUILD_LumpOverflow
};


build_result_e AJBSP_BuildLevel(nodebuildinfo_t *info, int lev_idx, Instance &inst, const Document &doc, const LoadingData& loading, Wad_file &wad);


//======================================================================
//
//    INTERNAL STUFF FROM HERE ON
//
//======================================================================

namespace ajbsp
{




/* ----- basic types --------------------------- */

typedef double angle_g;  // degrees, 0 is E, 90 is N


// prefer not to split
#define MLF_IS_PRECIOUS  0x40000000

// this is flag set when a linedef directly overlaps an earlier
// one (a rarely-used trick to create higher mid-masked textures).
// No segs should be created for these overlapping linedefs.
#define MLF_IS_OVERLAP   0x20000000


//------------------------------------------------------------------------
// UTILITY : general purpose functions
//------------------------------------------------------------------------

void PrintDetail(const char *fmt, ...);


// allocate and clear some memory.  guaranteed not to fail.
void *UtilCalloc(int size);

// re-allocate some memory.  guaranteed not to fail.
void *UtilRealloc(void *old, int size);

// free some memory or a string.
void UtilFree(void *data);

// return an allocated string for the current data and time,
// or NULL if an error occurred.
SString UtilTimeString(void);

// compute angle of line from (0,0) to (dx,dy)
angle_g UtilComputeAngle(double dx, double dy);

// checksum functions
void Adler32_Begin(uint32_t *crc);
void Adler32_AddBlock(uint32_t *crc, const uint8_t *data, int length);
void Adler32_Finish(uint32_t *crc);


//------------------------------------------------------------------------
// BLOCKMAP : Generate the blockmap
//------------------------------------------------------------------------

// utility routines...
void GetBlockmapBounds(int *x, int *y, int *w, int *h);

int CheckLinedefInsideBox(int xmin, int ymin, int xmax, int ymax,
    int x1, int y1, int x2, int y2);


//------------------------------------------------------------------------
// LEVEL : Level structures & read/write functions.
//------------------------------------------------------------------------

struct node_t;

class quadtree_c;


// a wall-tip is where a wall meets a vertex
struct walltip_t
{
	// link in list.  List is kept in ANTI-clockwise order.
	walltip_t *next;
	walltip_t *prev;

	// angle that line makes at vertex (degrees).
	angle_g angle;

	// whether each side of wall is OPEN or CLOSED.
	// left is the side of increasing angles, whereas
	// right is the side of decreasing angles.
	bool open_left;
	bool open_right;
};


struct vertex_t
{
	// coordinates
	double x, y;

	// vertex index.  Always valid after loading and pruning of unused
	// vertices has occurred.
	int index;

	// vertex is newly created (from a seg split)
	bool is_new;

	// usually NULL, unless this vertex occupies the same location as a
	// previous vertex.  when there are multiple vertices at one spot,
	// the second and third (etc) all refer to the first.
	struct vertex_t *overlap;

	// list of wall-tips
	walltip_t *tip_set;
};

struct seg_t
{
	// link for list
	struct seg_t *next;

	vertex_t *start;   // from this vertex...
	vertex_t *end;     // ... to this vertex

	// linedef that this seg goes along, or -1 if miniseg
	int linedef;

	// 0 for right, 1 for left
	int side;

	// seg on other side, or NULL if one-sided.  This relationship is
	// always one-to-one -- if one of the segs is split, the partner seg
	// must also be split.
	struct seg_t *partner;

	// seg index.  Only valid once the seg has been added to a
	// subsector.  A negative value means it is invalid -- there
	// shouldn't be any of these once the BSP tree has been built.
	int index;

	// when 1, this seg has become zero length (integer rounding of the
	// start and end vertices produces the same location).  It should be
	// ignored when writing the SEGS or V1 GL_SEGS lumps.  [Note: there
	// won't be any of these when writing the V2 GL_SEGS lump].
	bool is_degenerate;

	// the quad-tree node that contains this seg, or NULL if the seg
	// is now in a subsector.
	quadtree_c *quad;

	// precomputed data for faster calculations
	double psx, psy;
	double pex, pey;
	double pdx, pdy;

	double p_length;
	double p_para;
	double p_perp;

	// linedef that this seg initially comes from.  For "real" segs,
	// this is just the same as the 'linedef' field above.  For
	// "minisegs", this is the linedef of the partition line.
	int source_line;

	// this only used by ClockwiseOrder()
	angle_g cmp_angle;

public:
	// compute the seg private info (psx/y, pex/y, pdx/y, etc).
	void Recompute();

	// returns SIDE_LEFT, SIDE_RIGHT or 0 for being "on" the line.
	Side PointOnLineSide(double x, double y) const;

	// compute the parallel and perpendicular distances from a partition
	// line to a point.
	//
	inline double ParallelDist(double x, double y) const
	{
		return (x * pdx + y * pdy + p_para) / p_length;
	}

	inline double PerpDist(double x, double y) const
	{
		return (x * pdy - y * pdx + p_perp) / p_length;
	}
};


// a seg with this index is removed by SortSegs().
// it must be a very high value.
#define SEG_IS_GARBAGE  (1 << 29)

class LevelData;
struct subsec_t
{
	// list of segs
	seg_t *seg_list;

	// count of segs
	int seg_count;

	// subsector index.  Always valid, set when the subsector is
	// initially created.
	int index;

	// approximate middle point
	double mid_x;
	double mid_y;

public:
	void DetermineMiddle();
	void ClockwiseOrder(const Document &doc);
	void RenumberSegs();

	void RoundOff(LevelData &lev_data);
	void Normalise();

	void SanityCheckClosed();
	void SanityCheckHasRealSeg();
};


struct bbox_t
{
	int minx, miny;
	int maxx, maxy;
};


struct child_t
{
	// child node or subsector (one must be NULL)
	node_t *node;
	subsec_t *subsec;

	// child bounding box
	bbox_t bounds;
};


struct node_t
{
	// these coordinates are high precision to support UDMF.
	// in non-UDMF maps, they will actually be integral since a
	// partition line *always* comes from a normal linedef.

	double x, y;     // starting point
	double dx, dy;   // offset to ending point

	// right & left children
	child_t r;
	child_t l;

	// node index.  Only valid once the NODES or GL_NODES lump has been
	// created.
	int index;

public:
	void SetPartition(LevelData &lev_data, const seg_t *part);
};


class quadtree_c
{
public:
	// coordinates on map for this block, from lower-left corner to
	// upper-right corner.  Fully inclusive, i.e (x,y) is inside this
	// block when x1 < x < x2 and y1 < y < y2.
	int x1, y1;
	int x2, y2;

	// sub-trees.  NULL for leaf nodes.
	// [0] has the lower coordinates, and [1] has the higher coordinates.
	// Division of a square always occurs horizontally (e.g. 512x512 -> 256x512).
	quadtree_c *subs[2];

	// count of real/mini segs contained in this node AND ALL CHILDREN.
	int real_num;
	int mini_num;

	// list of segs contained in this node itself.
	seg_t *list;

public:
	quadtree_c(int _x1, int _y1, int _x2, int _y2);
	~quadtree_c();

	void AddSeg(seg_t *seg);
	void AddList(seg_t *list);

	inline bool Empty() const
	{
		return (real_num + mini_num) == 0;
	}

	void ConvertToList(seg_t **list);

	// check relationship between this box and the partition line.
	// returns SIDE_LEFT or SIDE_RIGHT if box is definitively on a
	// particular side, or 0 if the line intersects/touches the box.
	//
	Side OnLineSide(const seg_t *part) const;
};


/* ----- Level data arrays ----------------------- */

class ZLibContext;
struct intersection_t;
struct eval_info_t;
class LevelData
{
	
public:
	typedef std::function<void(const SString &)> ReportFunc;
	
	LevelData(MapFormat format, Wad_file &wad, const Document &doc, const ConfigData &config, const ReportFunc &reportLog) : format(format), wad(wad), doc(doc), config(config), reportLog(reportLog)
	{
	}
	LevelData(const LevelData& other) = delete;
	LevelData& operator = (const LevelData& other) = delete;

	MapFormat GetFormat() const
	{
		return format;
	}

	const Document& GetDoc() const
	{
		return doc;
	}

	// return a new end vertex to compensate for a seg that would end up
	// being zero-length (after integer rounding).  Doesn't compute the
	// wall-tip info (thus this routine should only be used _after_ node
	// building).
	//
	vertex_t *NewVertexDegenerate(vertex_t *start, vertex_t *end);
	
	// MAIN STUFF
	build_result_e BuildLevel(nodebuildinfo_t *info, int lev_idx);
	
	void Warning(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(2, 3);
	
private:
	struct Block
	{
		void CreateMap(const Document &doc);
		void CompressMap();
		void InitMap(const Document &doc);
		void FreeMap();
		
		int block_x = 0;
		int block_y = 0;
		int block_w = 0;
		int block_h = 0;
		int block_count = 0;
		uint16_t ** block_lines = nullptr;
		uint16_t *block_ptrs = nullptr;
		uint16_t *block_dups = nullptr;
		int compression = 0;
		int overflowed = 0;
		
	private:
		void Add(int blk_num, int line_index);
		void AddLine(int line_index, const Document &doc);
		int Compare(const void *p1, const void *p2) const;
	};
	
	struct Reject
	{
		void Init(const Document &doc);
		void Free();
		void GroupSectors(const Document &doc);
		void ProcessSectors(const Document &doc);
		
		uint8_t *rej_matrix = nullptr;
		int   rej_total_size = 0;	// in bytes
		std::vector<int> rej_sector_groups;
	};
	
	/* ----- create blockmap ------------------------------------ */
	
	void WriteBlockmap() const;
	
	void PutBlockmap();
	
	// REJECT : Generate the reject table
	void Reject_WriteLump() const;
	void PutReject();
	
	// allocation routines
	vertex_t *NewVertex();
	seg_t     *NewSeg();
	subsec_t  *NewSubsec();
	node_t    *NewNode();
	walltip_t *NewWallTip();
	
	/* ----- free routines ---------------------------- */
	void FreeVertices();
	void FreeSegs();
	void FreeSubsecs();
	void FreeNodes();
	void FreeWallTips();
	
	/* ----- reading routines ------------------------------ */
	void GetVertices();
	
	/* ----- writing routines ------------------------------ */
	void MarkOverflow(int flags);
	void PutVertices(const char *name, int do_gl);
	void PutGLVertices(int do_v5) const;
	inline uint32_t VertexIndex_XNOD(const vertex_t *v) const noexcept
	{
		if (v->is_new)
			return (uint32_t) (num_old_vert + v->index);

		return (uint32_t) v->index;
	}
	void PutSegs();
	void PutGLSegs() const;
	void PutGLSegs_V5() const;
	void PutSubsecs(const char *name, int do_gl);
	void PutGLSubsecs_V5() const;
	void PutNodes(const char *name, int do_v5, node_t *root);
	void PutOneNode(node_t *node, Lump_c *lump);
	void PutOneNode_V5(node_t *node, Lump_c *lump);
	void CheckLimits(bool& force_v5, bool& force_xnod);
	void SortSegs();
	
	/* ----- ZDoom format writing --------------------------- */
	void putZItems(std::vector<byte>& lumpData, node_t* root_node, bool do_xgl3);
	void PutZVertices(ZLibContext &zcontext) const noexcept(false);
	void PutZSubsecs(ZLibContext &zcontext) const noexcept(false);
	void PutZSegs(ZLibContext &zcontext) const noexcept(false);
	void PutXGL3Segs(ZLibContext &zcontext) const noexcept(false);
	void PutOneZNode(ZLibContext &zcontext, node_t *node, bool do_xgl3) noexcept(false);
	void PutZNodes(ZLibContext &zcontext, node_t *root, bool do_xgl3) noexcept(false);
	void SaveZDFormat(node_t *root_node);
	void SaveXGL3Format(node_t *root_node);
	
	/* ----- whole-level routines --------------------------- */
	void LoadLevel();
	void FreeLevel();
	uint32_t CalcGLChecksum() const;
	inline SString CalcOptionsString() const
	{
		return SString::printf("--cost %d%s", cur_info->factor, cur_info->fast ? " --fast" : "");
	}
	void UpdateGLMarker(Lump_c *marker) const;
	void AddMissingLump(const char *name, const char *after);
	build_result_e SaveLevel(node_t *root_node);
	build_result_e SaveUDMF(node_t *root_node);
	
	/* ---------------------------------------------------------------- */
	Lump_c * FindLevelLump(const char *name) const noexcept;
	Lump_c & CreateLevelLump(const char *name) const;
	Lump_c & CreateGLMarker() const;
	
	// NODES
	seg_t * SplitSeg(seg_t *old_seg, double x, double y);
	void DivideOneSeg(seg_t *seg, seg_t *part,
					  seg_t **left_list, seg_t **right_list,
					  intersection_t ** cut_list);
	void SeparateSegs(quadtree_c *tree, seg_t *part,
					  seg_t **left_list, seg_t **right_list,
					  intersection_t ** cut_list);
	void AddMinisegs(intersection_t *cut_list, seg_t *part,
					 seg_t **left_list, seg_t **right_list);
	// takes the seg list and determines if it is convex.  When it is, the
	// segs are converted to a subsector, and '*S' is the new subsector
	// (and '*N' is set to NULL).  Otherwise the seg list is divided into
	// two halves, a node is created by calling this routine recursively,
	// and '*N' is the new node (and '*S' is set to NULL).  Normally
	// returns BUILD_OK, or BUILD_Cancelled if user stopped it.
	//
	build_result_e BuildNodes(seg_t *list, bbox_t *bounds /* output */,
		node_t ** N, subsec_t ** S, int depth);
	seg_t *CreateOneSeg(int line, vertex_t *start, vertex_t *end,
						int sidedef, int what_side /* 0 or 1 */);
	// scan all the linedef of the level and convert each sidedef into a
	// seg (or seg pair).  Returns the list of segs.
	//
	seg_t *CreateSegs();
	subsec_t *CreateSubsector(quadtree_c *tree);
	// put all the segs in each subsector into clockwise order, and renumber
	// the seg indices.
	//
	// [ This cannot be done DURING BuildNodes() since splitting a seg with
	//   a partner will insert another seg into that partner's list, usually
	//   in the wrong place order-wise. ]
	//
	void ClockwiseBspTree();
	// traverse the BSP tree and do whatever is necessary to convert the
	// node information from GL standard to normal standard (for example,
	// removing minisegs).
	//
	void NormaliseBspTree() const;
	void RoundOffVertices();
	// traverse the BSP tree, doing whatever is necessary to round
	// vertices to integer coordinates (for example, removing segs whose
	// rounded coordinates degenerate to the same point).
	//
	void RoundOffBspTree();
	
	void MarkPolyobjPoint(double x, double y);

	// detection routines
	void DetectOverlappingVertices() const;
	void DetectPolyobjSectors();
	
	int EvalPartitionWorker(quadtree_c *tree, seg_t *part,
								   int best_cost, eval_info_t *info);
	int EvalPartition(quadtree_c *tree, seg_t *part, int best_cost);
	seg_t *FindFastSeg(quadtree_c *tree);
	bool PickNodeWorker(quadtree_c *part_list,
						quadtree_c *tree, seg_t ** best, int *best_cost);
	// scan all the segs in the list, and choose the best seg to use as a
	// partition line, returning it.  If no seg can be used, returns NULL.
	// The 'depth' parameter is the current depth in the tree, used for
	// computing the current progress.
	//
	seg_t *PickNode(quadtree_c *tree, int depth);
	
	/* ----- vertex routines ------------------------------- */
	void VertexAddWallTip(vertex_t *vert, double dx, double dy,
						  int open_left, int open_right);
	// computes the wall tips for all of the vertices
	void CalculateWallTips();
	// return a new vertex (with correct wall-tip info) for the split that
	// happens along the given seg at the given location.
	//
	vertex_t *NewVertexFromSplitSeg(seg_t *seg, double x, double y);
	
	void Failure(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(2, 3);
	
	void PrintMsg(EUR_FORMAT_STRING(const char *format), ...) const EUR_PRINTF(2, 3);
	
	Block block = {};
	Reject rej = {};
	
	SString current_name;
	int current_idx = 0;
	int current_start = 0;
	int overflows = 0;
	
	int num_old_vert = 0;
	int num_new_vert = 0;
	int num_real_lines = 0;
	int node_cur_index = 0;
	
	std::vector<vertex_t *>  vertices;
	std::vector<subsec_t *>  subsecs;
	std::vector<seg_t *>     segs;
	std::vector<node_t *>    nodes;
	std::vector<walltip_t *> walltips;
	
	// internal storage of node building parameters
	nodebuildinfo_t * cur_info = NULL;

	const MapFormat format;
	Wad_file& wad;
	const Document& doc;
	const ConfigData &config;
	
	const ReportFunc reportLog;
};


/* ----- function prototypes ----------------------- */

/* limit flags, to show what went wrong */
#define LIMIT_VERTEXES     0x000001
#define LIMIT_SECTORS      0x000002
#define LIMIT_SIDEDEFS     0x000004
#define LIMIT_LINEDEFS     0x000008

#define LIMIT_SEGS         0x000010
#define LIMIT_SSECTORS     0x000020
#define LIMIT_NODES        0x000040

#define LIMIT_GL_VERT      0x000100
#define LIMIT_GL_SEGS      0x000200
#define LIMIT_GL_SSECT     0x000400
#define LIMIT_GL_NODES     0x000800


//------------------------------------------------------------------------
// ANALYZE : Analyzing level structures
//------------------------------------------------------------------------


// detection routines

void DetectOverlappingLines(const Document &doc);

// check whether a line with the given delta coordinates from this
// vertex is open or closed.  If there exists a walltip at same
// angle, it is closed, likewise if line is in void space.
//
bool VertexCheckOpen(vertex_t *vert, double dx, double dy);


//------------------------------------------------------------------------
// SEG : Choose the best Seg to use for a node line.
//------------------------------------------------------------------------


#define IFFY_LEN  4.0


inline void ListAddSeg(seg_t **list_ptr, seg_t *seg)
{
	seg->next = *list_ptr;
	*list_ptr = seg;
}


// an "intersection" remembers the vertex that touches a BSP divider
// line (especially a new vertex that is created at a seg split).

struct intersection_t
{
	// link in list.  The intersection list is kept sorted by
	// along_dist, in ascending order.
	intersection_t *next;
	intersection_t *prev;

	// vertex in question
	vertex_t *vertex;

	// how far along the partition line the vertex is.  Zero is at the
	// partition seg's start point, positive values move in the same
	// direction as the partition's direction, and negative values move
	// in the opposite direction.
	double along_dist;

	// true if this intersection was on a self-referencing linedef
	bool self_ref;

	// status of each side of the vertex (along the partition),
	// true if OPEN and false if CLOSED.
	bool open_before;
	bool open_after;
};


/* -------- functions ---------------------------- */


// compute the boundary of the list of segs
void FindLimits2(seg_t *list, bbox_t *bbox);

// take the given seg 'cur', compare it with the partition line, and
// determine it's fate: moving it into either the left or right lists
// (perhaps both, when splitting it in two).  Handles partners as
// well.  Updates the intersection list if the seg lies on or crosses
// the partition line.
//
void DivideOneSeg(seg_t *cur, seg_t *part,
    quadtree_c *left_quad, quadtree_c *right_quad,
    intersection_t ** cut_list);

// remove all the segs from the list, partitioning them into the left
// or right lists based on the given partition line.  Adds any
// intersections into the intersection list as it goes.
//
void SeparateSegs(quadtree_c *tree, seg_t *part,
    quadtree_c *left_quad, quadtree_c *right_quad,
    intersection_t ** cut_list);

// analyse the intersection list, and add any needed minisegs to the
// given seg lists (one miniseg on each side).  All the intersection
// structures will be freed back into a quick-alloc list.
//
void AddMinisegs(seg_t *part,
    quadtree_c *left_quad, quadtree_c *right_list,
    intersection_t *cut_list);

// free the quick allocation cut list
void FreeQuickAllocCuts(void);


//------------------------------------------------------------------------
// NODE : Recursively create nodes and return the pointers.
//------------------------------------------------------------------------




quadtree_c *TreeFromSegList(seg_t *list);


// compute the height of the bsp tree, starting at 'node'.
int ComputeBspHeight(node_t *node);



// free all the superblocks on the quick-alloc list
void FreeQuickAllocSupers(void);

}  // namespace ajbsp


#endif /* __EUREKA_BSP_H__ */


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
