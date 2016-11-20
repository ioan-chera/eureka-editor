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

#ifndef __EUREKA_BSP_H__
#define __EUREKA_BSP_H__

class Lump_c;

/** Eureka change: namespacing */

namespace ajbsp
{


// certain GCC attributes can be useful
#undef GCCATTR
#ifdef __GNUC__
#define GCCATTR(xyz)  __attribute__ (xyz)
#else
#define GCCATTR(xyz)  /* nothing */
#endif


/* ----- basic types --------------------------- */

typedef double angle_g;  // degrees, 0 is E, 90 is N


/* ----- complex types --------------------------- */

// Node Build Information Structure
//
// Memory note: when changing the string values here (and in
// nodebuildcomms_t) they should be freed using StringFree() and
// allocated with StringDup().  The application has the final
// responsibility to free the strings in here.
//
#define DEFAULT_FACTOR  11

class nodebuildinfo_t
{
public:
	const char *input_file;
	const char *output_file;

	int factor;

	bool gl_nodes;

	// when these two are false, the create an empty lump
	bool do_blockmap;
	bool do_reject;

	bool force_v5;
	bool force_xnod;

	bool fast;
	bool quiet;
	bool mini_warnings;

	// This is for two-way communication (esp. with the GUI).
	// Should be flagged 'volatile' since multiple threads (real or
	// otherwise, e.g. signals) may read or change the values.

	// if the node builder failed, this will contain the error
	const char *message;

	// the GUI can set this to tell the node builder to stop
	bool cancelled;

	// from here on, various bits of internal state
	int total_small_warn, total_big_warn;
	int build_pos, file_pos;


public:
	nodebuildinfo_t() :
		input_file(NULL),
		output_file(NULL),

		factor(DEFAULT_FACTOR),

		gl_nodes(true),
		do_blockmap(true),
		do_reject  (true),

		force_v5(false),
		force_xnod(false),

		fast(false),
		quiet(false),
		mini_warnings(false),

		message(NULL),
		cancelled(false),
		total_small_warn(0),
		total_big_warn(0),
		build_pos(0),
		file_pos(0)
	{ }

	~nodebuildinfo_t()
	{
		// FIXME : free strings
	}
};


// Display Prototypes
typedef enum
{
	DIS_INVALID,        // Nonsense value is always useful
	DIS_BUILDPROGRESS,  // Build Box, has 2 bars
	DIS_FILEPROGRESS,   // File Box, has 1 bar
	NUMOFGUITYPES
}
displaytype_e;


/* -------- engine prototypes ----------------------- */

typedef enum
{
	// everything went peachy keen
	BUILD_OK = 0,

	// building was cancelled
	BUILD_Cancelled,

	// the WAD file was corrupt / empty / bad filename
	BUILD_BadFile,

	// file errors
	BUILD_ReadError,
	BUILD_WriteError
}
build_result_e;


build_result_e BuildNodesForLevel(nodebuildinfo_t *info, short lev_idx);


// main routine, this will build the nodes (GL and/or normal) for the
// given input wad file out to the given output file.  Returns
// BUILD_OK if everything went well, otherwise another error code.
// Typical errors are fubar parameters (like input_file == NULL),
// problems reading/writing files, or cancellation by another thread
// (esp. the GUI) using the comms->cancelled flag.  Upon errors, the
// comms->message field usually contains a string describing it.
//
///---  build_result_e BuildNodes(nodebuildinfo_t *info);


//------------------------------------------------------------------------
// STRUCT : Doom structures, raw on-disk layout
//------------------------------------------------------------------------


/* ----- The wad structures ---------------------- */

// wad header

typedef struct raw_wad_header_s
{
	char type[4];

	u32_t num_entries;
	u32_t dir_start;
}
raw_wad_header_t;


// directory entry

typedef struct raw_wad_entry_s
{
	u32_t start;
	u32_t length;

	char name[8];
}
raw_wad_entry_t;


// blockmap

typedef struct raw_blockmap_header_s
{
	s16_t x_origin, y_origin;
	s16_t x_blocks, y_blocks;
}
raw_blockmap_header_t;


/* ----- The level structures ---------------------- */

typedef struct raw_vertex_s
{
	s16_t x, y;
}
raw_vertex_t;

typedef struct raw_v2_vertex_s
{
	s32_t x, y;
}
raw_v2_vertex_t;


typedef struct raw_linedef_s
{
	u16_t start;     // from this vertex...
	u16_t end;       // ... to this vertex
	u16_t flags;     // linedef flags (impassible, etc)
	u16_t type;      // linedef type (0 for none, 97 for teleporter, etc)
	s16_t tag;       // this linedef activates the sector with same tag
	u16_t sidedef1;  // right sidedef
	u16_t sidedef2;  // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
	u16_t start;        // from this vertex...
	u16_t end;          // ... to this vertex
	u16_t flags;        // linedef flags (impassible, etc)
	u8_t  type;         // linedef type
	u8_t  specials[5];  // hexen specials
	u16_t sidedef1;     // right sidedef
	u16_t sidedef2;     // left sidedef
}
raw_hexen_linedef_t;

#define LINEFLAG_TWO_SIDED  4

#define HEXTYPE_POLY_START     1
#define HEXTYPE_POLY_EXPLICIT  5


typedef struct raw_sidedef_s
{
	s16_t x_offset;  // X offset for texture
	s16_t y_offset;  // Y offset for texture

	char upper_tex[8];  // texture name for the part above
	char lower_tex[8];  // texture name for the part below
	char mid_tex[8];    // texture name for the regular part

	u16_t sector;    // adjacent sector
}
raw_sidedef_t;


typedef struct raw_sector_s
{
	s16_t floor_h;   // floor height
	s16_t ceil_h;    // ceiling height

	char floor_tex[8];  // floor texture
	char ceil_tex[8];   // ceiling texture

	u16_t light;     // light level (0-255)
	u16_t special;   // special behaviour (0 = normal, 9 = secret, ...)
	s16_t tag;       // sector activated by a linedef with same tag
}
raw_sector_t;


typedef struct raw_thing_s
{
	s16_t x, y;      // position of thing
	s16_t angle;     // angle thing faces (degrees)
	u16_t type;      // type of thing
	u16_t options;   // when appears, deaf, etc..
}
raw_thing_t;


// -JL- Hexen thing definition
typedef struct raw_hexen_thing_s
{
	s16_t tid;       // thing tag id (for scripts/specials)
	s16_t x, y;      // position
	s16_t height;    // start height above floor
	s16_t angle;     // angle thing faces
	u16_t type;      // type of thing
	u16_t options;   // when appears, deaf, dormant, etc..

	u8_t special;    // special type
	u8_t arg[5];     // special arguments
}
raw_hexen_thing_t;

// -JL- Hexen polyobj thing types
#define PO_ANCHOR_TYPE      3000
#define PO_SPAWN_TYPE       3001
#define PO_SPAWNCRUSH_TYPE  3002

// -JL- ZDoom polyobj thing types
#define ZDOOM_PO_ANCHOR_TYPE      9300
#define ZDOOM_PO_SPAWN_TYPE       9301
#define ZDOOM_PO_SPAWNCRUSH_TYPE  9302


/* ----- The BSP tree structures ----------------------- */

typedef struct raw_seg_s
{
	u16_t start;     // from this vertex...
	u16_t end;       // ... to this vertex
	u16_t angle;     // angle (0 = east, 16384 = north, ...)
	u16_t linedef;   // linedef that this seg goes along
	u16_t flip;      // true if not the same direction as linedef
	u16_t dist;      // distance from starting point
}
raw_seg_t;


typedef struct raw_gl_seg_s
{
	u16_t start;      // from this vertex...
	u16_t end;        // ... to this vertex
	u16_t linedef;    // linedef that this seg goes along, or -1
	u16_t side;       // 0 if on right of linedef, 1 if on left
	u16_t partner;    // partner seg number, or -1
}
raw_gl_seg_t;


typedef struct raw_v5_seg_s
{
	u32_t start;      // from this vertex...
	u32_t end;        // ... to this vertex
	u16_t linedef;    // linedef that this seg goes along, or -1
	u16_t side;       // 0 if on right of linedef, 1 if on left
	u32_t partner;    // partner seg number, or -1
}
raw_v5_seg_t;


typedef struct raw_bbox_s
{
	s16_t maxy, miny;
	s16_t minx, maxx;
}
raw_bbox_t;


typedef struct raw_node_s
{
	s16_t x, y;         // starting point
	s16_t dx, dy;       // offset to ending point
	raw_bbox_t b1, b2;     // bounding rectangles
	u16_t right, left;  // children: Node or SSector (if high bit is set)
}
raw_node_t;


typedef struct raw_subsec_s
{
	u16_t num;     // number of Segs in this Sub-Sector
	u16_t first;   // first Seg
}
raw_subsec_t;


typedef struct raw_v5_subsec_s
{
	u32_t num;     // number of Segs in this Sub-Sector
	u32_t first;   // first Seg
}
raw_v5_subsec_t;


typedef struct raw_v5_node_s
{
	s16_t x, y;         // starting point
	s16_t dx, dy;       // offset to ending point
	raw_bbox_t b1, b2;     // bounding rectangles
	u32_t right, left;  // children: Node or SSector (if high bit is set)
}
raw_v5_node_t;



//------------------------------------------------------------------------
// SYSTEM : Bridging code
//------------------------------------------------------------------------


// internal storage of node building parameters

extern nodebuildinfo_t * cur_info;


/* ----- function prototypes ---------------------------- */

// display normal messages & warnings to the screen
void PrintMsg(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintVerbose(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintWarn(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintMiniWarn(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// set message for certain errors
void SetErrorMsg(const char *str, ...) GCCATTR((format (printf, 1, 2)));


//------------------------------------------------------------------------
// UTILITY : general purpose functions
//------------------------------------------------------------------------


/* ----- function prototypes ---------------------------- */

// allocate and clear some memory.  guaranteed not to fail.
void *UtilCalloc(int size);

// re-allocate some memory.  guaranteed not to fail.
void *UtilRealloc(void *old, int size);

// duplicate a string.
char *UtilStrDup(const char *str);
char *UtilStrNDup(const char *str, int size);

// format the string and return the allocated memory.
// The memory must be freed with UtilFree.
char *UtilFormat(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// free some memory or a string.
void UtilFree(void *data);

// compare two strings case insensitively.
int UtilStrCaseCmp(const char *A, const char *B);

// return an allocated string for the current data and time,
// or NULL if an error occurred.
char *UtilTimeString(void);

// round a positive value up to the nearest power of two.
int UtilRoundPOW2(int x);

// compute angle & distance from (0,0) to (dx,dy)
angle_g UtilComputeAngle(double dx, double dy);
#define UtilComputeDist(dx,dy)  sqrt((dx) * (dx) + (dy) * (dy))

// compute the parallel and perpendicular distances from a partition
// line to a point.
//
#define UtilParallelDist(part,x,y)  \
    (((x) * (part)->pdx + (y) * (part)->pdy + (part)->p_para)  \
     / (part)->p_length)

#define UtilPerpDist(part,x,y)  \
    (((x) * (part)->pdy - (y) * (part)->pdx + (part)->p_perp)  \
     / (part)->p_length)

// checksum functions
void Adler32_Begin(u32_t *crc);
void Adler32_AddBlock(u32_t *crc, const u8_t *data, int length);
void Adler32_Finish(u32_t *crc);


//------------------------------------------------------------------------
// BLOCKMAP : Generate the blockmap
//------------------------------------------------------------------------

// compute blockmap origin & size (the block_x/y/w/h variables)
// based on the set of loaded linedefs.
//
void InitBlockmap(void);

// build the blockmap and write the data into the BLOCKMAP lump
void PutBlockmap(void);

// utility routines...
void GetBlockmapBounds(int *x, int *y, int *w, int *h);

int CheckLinedefInsideBox(int xmin, int ymin, int xmax, int ymax,
    int x1, int y1, int x2, int y2);


//------------------------------------------------------------------------
// REJECT : Generate the reject table
//------------------------------------------------------------------------

// build the reject table and write it into the REJECT lump
void PutReject(void);


//------------------------------------------------------------------------
// LEVEL : Level structures & read/write functions.
//------------------------------------------------------------------------

struct node_s;
struct sector_s;
struct superblock_s;


// a wall_tip is where a wall meets a vertex
typedef struct wall_tip_s
{
	// link in list.  List is kept in ANTI-clockwise order.
	struct wall_tip_s *next;
	struct wall_tip_s *prev;

	// angle that line makes at vertex (degrees).
	angle_g angle;

	// sectors on each side of wall.  Left is the side of increasing
	// angles, right is the side of decreasing angles.  Either can be
	// NULL for one sided walls.
	struct sector_s *left;
	struct sector_s *right;
}
wall_tip_t;


typedef struct vertex_s
{
	// coordinates
	double x, y;

	// vertex index.  Always valid after loading and pruning of unused
	// vertices has occurred.  For GL vertices, bit 30 will be set.
	int index;

	// reference count.  When building normal node info, unused vertices
	// will be pruned.
	int ref_count;

	// usually NULL, unless this vertex occupies the same location as a
	// previous vertex.
	struct vertex_s *overlap;

	// set of wall_tips
	wall_tip_t *tip_set;

	// contains a duplicate vertex, needed when both normal and V2 GL
	// nodes are being built at the same time (this is the vertex used
	// for the normal segs).  Normally NULL.  Note: the wall tips on
	// this vertex are not created.
	struct vertex_s *normal_dup;
}
vertex_t;

#define IS_GL_VERTEX  (1 << 30)


typedef struct sector_s
{
	// sector index.  Always valid after loading & pruning.
	int index;

	// allow segs from other sectors to coexist in a subsector.
	char coalesce;

	// -JL- non-zero if this sector contains a polyobj.
	int has_polyobj;

	// reference count.  When building normal nodes, unused sectors will
	// be pruned.
	int ref_count;

	// heights
	int floor_h, ceil_h;

	// textures
	char floor_tex[8];
	char ceil_tex[8];

	// attributes
	int light;
	int special;
	int tag;

	// used when building REJECT table.  Each set of sectors that are
	// isolated from other sectors will have a different group number.
	// Thus: on every 2-sided linedef, the sectors on both sides will be
	// in the same group.  The rej_next, rej_prev fields are a link in a
	// RING, containing all sectors of the same group.
	int rej_group;

	struct sector_s *rej_next;
	struct sector_s *rej_prev;

	// suppress superfluous mini warnings
	int warned_facing;
	char warned_unclosed;
}
sector_t;


typedef struct sidedef_s
{
	// adjacent sector.  Can be NULL (invalid sidedef)
	sector_t *sector;

	// offset values
	int x_offset, y_offset;

	// texture names
	char upper_tex[8];
	char lower_tex[8];
	char mid_tex[8];

	// sidedef index.  Always valid after loading & pruning.
	int index;

	// reference count.  When building normal nodes, unused sidedefs will
	// be pruned.
	int ref_count;

	// usually NULL, unless this sidedef is exactly the same as a
	// previous one.  Only used during the pruning phase.
	struct sidedef_s *equiv;

	// this is true if the sidedef is on a special line.  We don't merge
	// these sidedefs together, as they might scroll, or change texture
	// when a switch is pressed.
	int on_special;
}
sidedef_t;


typedef struct linedef_s
{
	// link for list
	struct linedef_s *next;

	vertex_t *start;    // from this vertex...
	vertex_t *end;      // ... to this vertex

	sidedef_t *right;   // right sidedef
	sidedef_t *left;    // left sidede, or NULL if none

	// line is marked two-sided
	char two_sided;

	// prefer not to split
	char is_precious;

	// zero length (line should be totally ignored)
	char zero_len;

	// sector is the same on both sides
	char self_ref;

	// one-sided linedef used for a special effect (windows).
	// The value refers to the opposite sector on the back side.
	sector_t * window_effect;

	int flags;
	int type;
	int tag;

	// Hexen support
	int specials[5];

	// normally NULL, except when this linedef directly overlaps an earlier
	// one (a rarely-used trick to create higher mid-masked textures).
	// No segs should be created for these overlapping linedefs.
	struct linedef_s *overlap;

	// linedef index.  Always valid after loading & pruning of zero
	// length lines has occurred.
	int index;
}
linedef_t;


typedef struct thing_s
{
	int x, y;
	int type;
	int options;

	// other info (angle, and hexen stuff) omitted.  We don't need to
	// write the THING lump, only read it.

	// Always valid (thing indices never change).
	int index;
}
thing_t;


typedef struct seg_s
{
	// link for list
	struct seg_s *next;

	vertex_t *start;   // from this vertex...
	vertex_t *end;     // ... to this vertex

	// linedef that this seg goes along, or NULL if miniseg
	linedef_t *linedef;

	// adjacent sector, or NULL if invalid sidedef or miniseg
	sector_t *sector;

	// 0 for right, 1 for left
	int side;

	// seg on other side, or NULL if one-sided.  This relationship is
	// always one-to-one -- if one of the segs is split, the partner seg
	// must also be split.
	struct seg_s *partner;

	// seg index.  Only valid once the seg has been added to a
	// subsector.  A negative value means it is invalid -- there
	// shouldn't be any of these once the BSP tree has been built.
	int index;

	// when 1, this seg has become zero length (integer rounding of the
	// start and end vertices produces the same location).  It should be
	// ignored when writing the SEGS or V1 GL_SEGS lumps.  [Note: there
	// won't be any of these when writing the V2 GL_SEGS lump].
	int degenerate;

	// the superblock that contains this seg, or NULL if the seg is no
	// longer in any superblock (e.g. now in a subsector).
	struct superblock_s *block;

	// precomputed data for faster calculations
	double psx, psy;
	double pex, pey;
	double pdx, pdy;

	double p_length;
	double p_angle;
	double p_para;
	double p_perp;

	// linedef that this seg initially comes from.  For "real" segs,
	// this is just the same as the 'linedef' field above.  For
	// "minisegs", this is the linedef of the partition line.
	linedef_t *source_line;
}
seg_t;


typedef struct subsec_s
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

	// this is normally false, only set for the "no nodes hack"
	// [ see comments in the BuildNodes() function. ]
	bool is_dummy;
}
subsec_t;


typedef struct bbox_s
{
	int minx, miny;
	int maxx, maxy;
}
bbox_t;


typedef struct child_s
{
	// child node or subsector (one must be NULL)
	struct node_s *node;
	subsec_t *subsec;

	// child bounding box
	bbox_t bounds;
}
child_t;


typedef struct node_s
{
	int x, y;     // starting point
	int dx, dy;   // offset to ending point

	// right & left children
	child_t r;
	child_t l;

	// node index.  Only valid once the NODES or GL_NODES lump has been
	// created.
	int index;

	// the node is too long, and the (dx,dy) values should be halved
	// when writing into the NODES lump.
	int too_long;
}
node_t;


typedef struct superblock_s
{
	// parent of this block, or NULL for a top-level block
	struct superblock_s *parent;

	// coordinates on map for this block, from lower-left corner to
	// upper-right corner.  Pseudo-inclusive, i.e (x,y) is inside block
	// if and only if x1 <= x < x2 and y1 <= y < y2.
	int x1, y1;
	int x2, y2;

	// sub-blocks.  NULL when empty.  [0] has the lower coordinates, and
	// [1] has the higher coordinates.  Division of a square always
	// occurs horizontally (e.g. 512x512 -> 256x512 -> 256x256).
	struct superblock_s *subs[2];

	// number of real segs and minisegs contained by this block
	// (including all sub-blocks below it).
	int real_num;
	int mini_num;

	// list of segs completely contained by this block.
	seg_t *segs;
}
superblock_t;

#define SUPER_IS_LEAF(s)  \
    ((s)->x2-(s)->x1 <= 256 && (s)->y2-(s)->y1 <= 256)


/* ----- Level data arrays ----------------------- */

extern int num_vertices;
extern int num_linedefs;
extern int num_sidedefs;
extern int num_sectors;
extern int num_things;
extern int num_segs;
extern int num_subsecs;
extern int num_nodes;

extern int num_normal_vert;
extern int num_gl_vert;
extern int num_complete_seg;


/* ----- function prototypes ----------------------- */

// allocation routines
vertex_t *NewVertex(void);
linedef_t *NewLinedef(void);
sidedef_t *NewSidedef(void);
sector_t *NewSector(void);
thing_t *NewThing(void);
seg_t *NewSeg(void);
subsec_t *NewSubsec(void);
node_t *NewNode(void);
wall_tip_t *NewWallTip(void);

// lookup routines
vertex_t *LookupVertex(int index);
linedef_t *LookupLinedef(int index);
sidedef_t *LookupSidedef(int index);
sector_t *LookupSector(int index);
thing_t *LookupThing(int index);
seg_t *LookupSeg(int index);
subsec_t *LookupSubsec(int index);
node_t *LookupNode(int index);

Lump_c * CreateGLMarker();
Lump_c * CreateLevelLump(const char *name, int max_size = -1);
Lump_c * FindLevelLump(const char *name);

// Zlib compression support
void ZLibBeginLump(Lump_c *lump);
void ZLibAppendLump(const void *data, int length);
void ZLibFinishLump(void);

// mark the fact that this level failed to build.
void MarkSoftFailure(int soft);
void MarkHardFailure(int hard);
void MarkV5Switch(int v5);
void MarkZDSwitch(void);

// alert the user if any levels failed to build properly.
void ReportFailedLevels(void);

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

#define LIMIT_BAD_SIDE     0x001000
#define LIMIT_BMAP_TRUNC   0x002000
#define LIMIT_BLOCKMAP     0x004000
#define LIMIT_ZDBSP        0x008000


//------------------------------------------------------------------------
// ANALYZE : Analyzing level structures
//------------------------------------------------------------------------


// detection routines
void DetectOverlappingVertices(void);
void DetectOverlappingLines(void);
void DetectPolyobjSectors(void);

// pruning routines
void PruneVerticesAtEnd(void);

// computes the wall tips for all of the vertices
void CalculateWallTips(void);

// return a new vertex (with correct wall_tip info) for the split that
// happens along the given seg at the given location.
//
vertex_t *NewVertexFromSplitSeg(seg_t *seg, double x, double y);

// return a new end vertex to compensate for a seg that would end up
// being zero-length (after integer rounding).  Doesn't compute the
// wall_tip info (thus this routine should only be used _after_ node
// building).
//
vertex_t *NewVertexDegenerate(vertex_t *start, vertex_t *end);

// check whether a line with the given delta coordinates and beginning
// at this vertex is open.  Returns a sector reference if it's open,
// or NULL if closed (void space or directly along a linedef).
//
sector_t * VertexCheckOpen(vertex_t *vert, double dx, double dy);


//------------------------------------------------------------------------
// SEG : Choose the best Seg to use for a node line.
//------------------------------------------------------------------------


#define IFFY_LEN  4.0


// smallest distance between two points before being considered equal
#define DIST_EPSILON  (1.0 / 128.0)

// smallest degrees between two angles before being considered equal
#define ANG_EPSILON  (1.0 / 1024.0)


// an "intersection" remembers the vertex that touches a BSP divider
// line (especially a new vertex that is created at a seg split).

typedef struct intersection_s
{
	// link in list.  The intersection list is kept sorted by
	// along_dist, in ascending order.
	struct intersection_s *next;
	struct intersection_s *prev;

	// vertex in question
	vertex_t *vertex;

	// how far along the partition line the vertex is.  Zero is at the
	// partition seg's start point, positive values move in the same
	// direction as the partition's direction, and negative values move
	// in the opposite direction.
	double along_dist;

	// true if this intersection was on a self-referencing linedef
	bool self_ref;

	// sector on each side of the vertex (along the partition),
	// or NULL when that direction isn't OPEN.
	sector_t *before;
	sector_t *after;
}
intersection_t;


/* -------- functions ---------------------------- */

// scan all the segs in the list, and choose the best seg to use as a
// partition line, returning it.  If no seg can be used, returns NULL.
// The 'depth' parameter is the current depth in the tree, used for
// computing  the current progress.
//
seg_t *PickNode(superblock_t *seg_list, int depth, const bbox_t *bbox);

// compute the boundary of the list of segs
void FindLimits(superblock_t *seg_list, bbox_t *bbox);

// compute the seg private info (psx/y, pex/y, pdx/y, etc).
void RecomputeSeg(seg_t *seg);

// take the given seg 'cur', compare it with the partition line, and
// determine it's fate: moving it into either the left or right lists
// (perhaps both, when splitting it in two).  Handles partners as
// well.  Updates the intersection list if the seg lies on or crosses
// the partition line.
//
void DivideOneSeg(seg_t *cur, seg_t *part,
    superblock_t *left_list, superblock_t *right_list,
    intersection_t ** cut_list);

// remove all the segs from the list, partitioning them into the left
// or right lists based on the given partition line.  Adds any
// intersections onto the intersection list as it goes.
//
void SeparateSegs(superblock_t *seg_list, seg_t *part,
    superblock_t *left_list, superblock_t *right_list,
    intersection_t ** cut_list);

// analyse the intersection list, and add any needed minisegs to the
// given seg lists (one miniseg on each side).  All the intersection
// structures will be freed back into a quick-alloc list.
//
void AddMinisegs(seg_t *part,
    superblock_t *left_list, superblock_t *right_list,
    intersection_t *cut_list);

// free the quick allocation cut list
void FreeQuickAllocCuts(void);


//------------------------------------------------------------------------
// NODE : Recursively create nodes and return the pointers.
//------------------------------------------------------------------------


// check the relationship between the given box and the partition
// line.  Returns -1 if box is on left side, +1 if box is on right
// size, or 0 if the line intersects the box.
//
int BoxOnLineSide(superblock_t *box, seg_t *part);

// add the seg to the given list
void AddSegToSuper(superblock_t *block, seg_t *seg);

// increase the counts within the superblock, to account for the given
// seg being split.
//
void SplitSegInSuper(superblock_t *block, seg_t *seg);

// scan all the linedef of the level and convert each sidedef into a
// seg (or seg pair).  Returns the list of segs.
//
superblock_t *CreateSegs(void);

// free a super block.
void FreeSuper(superblock_t *block);

// takes the seg list and determines if it is convex.  When it is, the
// segs are converted to a subsector, and '*S' is the new subsector
// (and '*N' is set to NULL).  Otherwise the seg list is divided into
// two halves, a node is created by calling this routine recursively,
// and '*N' is the new node (and '*S' is set to NULL).  Normally
// returns BUILD_OK, or BUILD_Cancelled if user stopped it.
//
build_result_e BuildNodes(superblock_t *seg_list,
    node_t ** N, subsec_t ** S, int depth, const bbox_t *bbox);

// compute the height of the bsp tree, starting at 'node'.
int ComputeBspHeight(node_t *node);

// traverse the BSP tree and put all the segs in each subsector into
// clockwise order, and renumber the seg indices.  This cannot be done
// DURING BuildNodes() since splitting a seg with a partner may insert
// another seg into that partner's list -- usually in the wrong place
// order-wise.
//
void ClockwiseBspTree(node_t *root);

// traverse the BSP tree and do whatever is necessary to convert the
// node information from GL standard to normal standard (for example,
// removing minisegs).
//
void NormaliseBspTree(node_t *root);

// traverse the BSP tree, doing whatever is necessary to round
// vertices to integer coordinates (for example, removing segs whose
// rounded coordinates degenerate to the same point).
//
void RoundOffBspTree(node_t *root);

// free all the superblocks on the quick-alloc list
void FreeQuickAllocSupers(void);

}  // namespace ajbsp


/* external funcs */

void GB_PrintMsg(const char *str, ...);

void GB_DisplayTicker(void);

bool GB_DisplayOpen(ajbsp::displaytype_e type);
void GB_DisplaySetTitle(const char *str);
void GB_DisplaySetBarText(int barnum, const char *str);
void GB_DisplaySetBarLimit(int barnum, int limit);
void GB_DisplaySetBar(int barnum, int count);
void GB_DisplayClose(void);


#endif /* __EUREKA_BSP_H__ */


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
