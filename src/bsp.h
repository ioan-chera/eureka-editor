//------------------------------------------------------------------------
// GLBSP.H : Interface to Node Builder
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

#ifndef __GLBSP_GLBSP_H__
#define __GLBSP_GLBSP_H__


/** Eureka change: namespacing */

namespace glbsp
{

#define GLBSP_VER  "2.27"
#define GLBSP_VER_HEX  0x227


// certain GCC attributes can be useful
#undef GCCATTR
#ifdef __GNUC__
#define GCCATTR(xyz)  __attribute__ (xyz)
#else
#define GCCATTR(xyz)  /* nothing */
#endif


/** OBLIGE change: assume C++ **/

// #ifdef __cplusplus
// extern "C" {
// #endif // __cplusplus


/* ----- basic types --------------------------- */

typedef signed char  sint8_g;
typedef signed short sint16_g;
typedef signed int   sint32_g;
   
typedef unsigned char  uint8_g;
typedef unsigned short uint16_g;
typedef unsigned int   uint32_g;

typedef double float_g;
typedef double angle_g;  // degrees, 0 is E, 90 is N

// boolean type
typedef int boolean_g;

#ifndef FALSE
#define FALSE  0
#endif
#ifndef TRUE
#define TRUE   1
#endif


/* ----- complex types --------------------------- */

// Node Build Information Structure 
//
// Memory note: when changing the string values here (and in
// nodebuildcomms_t) they should be freed using GlbspFree() and
// allocated with GlbspStrDup().  The application has the final
// responsibility to free the strings in here.
// 
typedef struct nodebuildinfo_s
{
  const char *input_file;
  const char *output_file;

  // pointer to a NULL terminated array of strings containing extra
  // input filenames.  Normally this field is NULL.  When there are
  // extra filenames, 'output_file' will be NULL -- also the build
  // mode will be GWA.
  const char **extra_files;

  int factor;

  boolean_g no_reject;
  boolean_g no_progress;
  boolean_g quiet;
  boolean_g mini_warnings;
  boolean_g force_hexen;
  boolean_g pack_sides;
  boolean_g fast;

  int spec_version;  // 1, 2, 3 or 5

  boolean_g load_all;
  boolean_g no_normal;
  boolean_g force_normal;
  boolean_g gwa_mode;  
  boolean_g prune_sect;
  boolean_g no_prune;
  boolean_g merge_vert;
  boolean_g skip_self_ref;
  boolean_g window_fx;

  int block_limit;

  // private stuff -- values computed in GlbspParseArgs or
  // GlbspCheckInfo that need to be passed to GlbspBuildNodes.

  boolean_g missing_output;
  boolean_g same_filenames;
}
nodebuildinfo_t;

// This is for two-way communication (esp. with the GUI).
// Should be flagged 'volatile' since multiple threads (real or
// otherwise, e.g. signals) may read or change the values.
//
typedef struct nodebuildcomms_s
{
  // if the node builder failed, this will contain the error
  const char *message;

  // the GUI can set this to tell the node builder to stop
  boolean_g cancelled;

  // from here on, various bits of internal state
  int total_small_warn, total_big_warn;
  int build_pos, file_pos;
}
nodebuildcomms_t;


// Display Prototypes
typedef enum
{
  DIS_INVALID,        // Nonsense value is always useful
  DIS_BUILDPROGRESS,  // Build Box, has 2 bars
  DIS_FILEPROGRESS,   // File Box, has 1 bar
  NUMOFGUITYPES
}
displaytype_e;

// Callback functions
typedef struct nodebuildfuncs_s
{
  // Fatal errors are called as a last resort when something serious
  // goes wrong, e.g. out of memory.  This routine should show the
  // error to the user and abort the program.
  // 
  void (* fatal_error)(const char *str, ...) GCCATTR((format (printf, 1, 2)));

  // The print_msg routine is used to display the various messages
  // that occur, e.g. "Building GL nodes on MAP01" and that kind of
  // thing.
  // 
  void (* print_msg)(const char *str, ...) GCCATTR((format (printf, 1, 2)));

  // This routine is called frequently whilst building the nodes, and
  // can be used to keep a GUI responsive to user input.  Many
  // toolkits have a "do iteration" or "check events" type of function
  // that this can call.  Avoid anything that sleeps though, or it'll
  // slow down the build process unnecessarily.
  //
  void (* ticker)(void);

  // These display routines is used for tasks that can show a progress
  // bar, namely: building nodes, loading the wad, and saving the wad.
  // The command line version could show a percentage value, or even
  // draw a bar using characters.
 
  // Display_open is called at the beginning, and 'type' holds the
  // type of progress (and determines how many bars to display).
  // Returns TRUE if all went well, or FALSE if it failed (in which
  // case the other routines should do nothing when called).
  // 
  boolean_g (* display_open)(displaytype_e type);

  // For GUI versions this can be used to set the title of the
  // progress window.  OK to ignore it (e.g. command line version).
  //
  void (* display_setTitle)(const char *str);

  // The next three routines control the appearance of each progress
  // bar.  Display_setBarText is called to change the message above
  // the bar.  Display_setBarLimit sets the integer limit of the
  // progress (the target value), and display_setBar sets the current
  // value (which will count up from 0 to the limit, inclusive).
  // 
  void (* display_setBar)(int barnum, int count);
  void (* display_setBarLimit)(int barnum, int limit);
  void (* display_setBarText)(int barnum, const char *str);

  // The display_close routine is called when the task is finished,
  // and should remove the progress indicator/window from the screen.
  //
  void (* display_close)(void);
}
nodebuildfuncs_t;

// Default build info and comms
extern const nodebuildinfo_t default_buildinfo;
extern const nodebuildcomms_t default_buildcomms;


/* -------- engine prototypes ----------------------- */

typedef enum
{
  // everything went peachy keen
  GLBSP_E_OK = 0,

  // an unknown error occurred (this is the catch-all value)
  GLBSP_E_Unknown,

  // the arguments were bad/inconsistent.
  GLBSP_E_BadArgs,

  // the info was bad/inconsistent, but has been fixed
  GLBSP_E_BadInfoFixed,

  // file errors
  GLBSP_E_ReadError,
  GLBSP_E_WriteError,

  // building was cancelled
  GLBSP_E_Cancelled
}
glbsp_ret_e;

// parses the arguments, modifying the 'info' structure accordingly.
// Returns GLBSP_E_OK if all went well, otherwise another error code.
// Upon error, comms->message may be set to an string describing the
// error.  Typical errors are unrecognised options and invalid option
// values.  Calling this routine is not compulsory.  Note that the set
// of arguments does not include the program's name.
//
glbsp_ret_e ParseArgs(nodebuildinfo_t *info,
    volatile nodebuildcomms_t *comms,
    const char ** argv, int argc);

// checks the node building parameters in 'info'.  If they are valid,
// returns GLBSP_E_OK, otherwise an error code is returned.  This
// routine should always be called shortly before GlbspBuildNodes().
// Note that when 'output_file' is NULL, this routine will silently
// update it to the correct GWA filename (and set the gwa_mode flag).
//
// If the GLBSP_E_BadInfoFixed error code is returned, the parameter
// causing the problem has been changed.  You could either treat it as
// a fatal error, or alternatively keep calling the routine in a loop
// until something different than GLBSP_E_BadInfoFixed is returned,
// showing the user the returned messages (e.g. as warnings or in
// pop-up dialog boxes).
//
// If there are multiple input files (extra_files is non-NULL), this
// routine should be called once for each input file, setting the
// 'output_file' field to NULL each time.
//
glbsp_ret_e CheckInfo(nodebuildinfo_t *info,
    volatile nodebuildcomms_t *comms);

// main routine, this will build the nodes (GL and/or normal) for the
// given input wad file out to the given output file.  Returns
// GLBSP_E_OK if everything went well, otherwise another error code.
// Typical errors are fubar parameters (like input_file == NULL),
// problems reading/writing files, or cancellation by another thread
// (esp. the GUI) using the comms->cancelled flag.  Upon errors, the
// comms->message field usually contains a string describing it.
//
glbsp_ret_e BuildNodes(const nodebuildinfo_t *info,
    const nodebuildfuncs_t *funcs, 
    volatile nodebuildcomms_t *comms);

// string memory routines.  These should be used for all strings
// shared between the main glBSP code and the UI code (including code
// using glBSP as a plug-in).  They accept NULL pointers.
//
const char *GlbspStrDup(const char *str);
void GlbspFree(const char *str);


}  // namespace glbsp

#endif /* __GLBSP_GLBSP_H__ */
//------------------------------------------------------------------------
// STRUCT : Doom structures, raw on-disk layout
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

#ifndef __GLBSP_STRUCTS_H__
#define __GLBSP_STRUCTS_H__

#include "system.h"


namespace glbsp
{

/* ----- The wad structures ---------------------- */

// wad header

typedef struct raw_wad_header_s
{
  char type[4];

  uint32_g num_entries;
  uint32_g dir_start;
}
raw_wad_header_t;


// directory entry

typedef struct raw_wad_entry_s
{
  uint32_g start;
  uint32_g length;

  char name[8];
}
raw_wad_entry_t;


// blockmap

typedef struct raw_blockmap_header_s
{
  sint16_g x_origin, y_origin;
  sint16_g x_blocks, y_blocks;
}
raw_blockmap_header_t;


/* ----- The level structures ---------------------- */

typedef struct raw_vertex_s
{
  sint16_g x, y;
}
raw_vertex_t;

typedef struct raw_v2_vertex_s
{
  sint32_g x, y;
}
raw_v2_vertex_t;


typedef struct raw_linedef_s
{
  uint16_g start;     // from this vertex...
  uint16_g end;       // ... to this vertex
  uint16_g flags;     // linedef flags (impassible, etc)
  uint16_g type;      // linedef type (0 for none, 97 for teleporter, etc)
  sint16_g tag;       // this linedef activates the sector with same tag
  uint16_g sidedef1;  // right sidedef
  uint16_g sidedef2;  // left sidedef (only if this line adjoins 2 sectors)
}
raw_linedef_t;

typedef struct raw_hexen_linedef_s
{
  uint16_g start;        // from this vertex...
  uint16_g end;          // ... to this vertex
  uint16_g flags;        // linedef flags (impassible, etc)
  uint8_g  type;         // linedef type
  uint8_g  specials[5];  // hexen specials
  uint16_g sidedef1;     // right sidedef
  uint16_g sidedef2;     // left sidedef
}
raw_hexen_linedef_t;

#define LINEFLAG_TWO_SIDED  4

#define HEXTYPE_POLY_START     1
#define HEXTYPE_POLY_EXPLICIT  5


typedef struct raw_sidedef_s
{
  sint16_g x_offset;  // X offset for texture
  sint16_g y_offset;  // Y offset for texture

  char upper_tex[8];  // texture name for the part above
  char lower_tex[8];  // texture name for the part below
  char mid_tex[8];    // texture name for the regular part

  uint16_g sector;    // adjacent sector
}
raw_sidedef_t;


typedef struct raw_sector_s
{
  sint16_g floor_h;   // floor height
  sint16_g ceil_h;    // ceiling height

  char floor_tex[8];  // floor texture
  char ceil_tex[8];   // ceiling texture

  uint16_g light;     // light level (0-255)
  uint16_g special;   // special behaviour (0 = normal, 9 = secret, ...)
  sint16_g tag;       // sector activated by a linedef with same tag
}
raw_sector_t;


typedef struct raw_thing_s
{
  sint16_g x, y;      // position of thing
  sint16_g angle;     // angle thing faces (degrees)
  uint16_g type;      // type of thing
  uint16_g options;   // when appears, deaf, etc..
}
raw_thing_t;


// -JL- Hexen thing definition
typedef struct raw_hexen_thing_s
{
  sint16_g tid;       // thing tag id (for scripts/specials)
  sint16_g x, y;      // position
  sint16_g height;    // start height above floor
  sint16_g angle;     // angle thing faces
  uint16_g type;      // type of thing
  uint16_g options;   // when appears, deaf, dormant, etc..

  uint8_g special;    // special type
  uint8_g arg[5];     // special arguments
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
  uint16_g start;     // from this vertex...
  uint16_g end;       // ... to this vertex
  uint16_g angle;     // angle (0 = east, 16384 = north, ...)
  uint16_g linedef;   // linedef that this seg goes along
  uint16_g flip;      // true if not the same direction as linedef
  uint16_g dist;      // distance from starting point
}
raw_seg_t;


typedef struct raw_gl_seg_s
{
  uint16_g start;      // from this vertex...
  uint16_g end;        // ... to this vertex
  uint16_g linedef;    // linedef that this seg goes along, or -1
  uint16_g side;       // 0 if on right of linedef, 1 if on left
  uint16_g partner;    // partner seg number, or -1
}
raw_gl_seg_t;


typedef struct raw_v3_seg_s
{
  uint32_g start;      // from this vertex...
  uint32_g end;        // ... to this vertex
  uint16_g linedef;    // linedef that this seg goes along, or -1
  uint16_g side;       // 0 if on right of linedef, 1 if on left
  uint32_g partner;    // partner seg number, or -1
}
raw_v3_seg_t;


typedef struct raw_bbox_s
{
  sint16_g maxy, miny;
  sint16_g minx, maxx;
}
raw_bbox_t;


typedef struct raw_node_s
{
  sint16_g x, y;         // starting point
  sint16_g dx, dy;       // offset to ending point
  raw_bbox_t b1, b2;     // bounding rectangles
  uint16_g right, left;  // children: Node or SSector (if high bit is set)
}
raw_node_t;


typedef struct raw_subsec_s
{
  uint16_g num;     // number of Segs in this Sub-Sector
  uint16_g first;   // first Seg
}
raw_subsec_t;


typedef struct raw_v3_subsec_s
{
  uint32_g num;     // number of Segs in this Sub-Sector
  uint32_g first;   // first Seg
}
raw_v3_subsec_t;


typedef struct raw_v5_node_s
{
  sint16_g x, y;         // starting point
  sint16_g dx, dy;       // offset to ending point
  raw_bbox_t b1, b2;     // bounding rectangles
  uint32_g right, left;  // children: Node or SSector (if high bit is set)
}
raw_v5_node_t;


}  // namespace glbsp

#endif /* __GLBSP_STRUCTS_H__ */
//------------------------------------------------------------------------
// SYSTEM : Bridging code
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

#ifndef __GLBSP_SYSTEM_H__
#define __GLBSP_SYSTEM_H__

#include "glbsp.h"


namespace glbsp
{

// use this for inlining.  Usually defined in the makefile.
#ifndef INLINE_G
#define INLINE_G  /* nothing */
#endif


// internal storage of node building parameters

extern const nodebuildinfo_t *cur_info;
extern const nodebuildfuncs_t *cur_funcs;
extern volatile nodebuildcomms_t *cur_comms;


/* ----- function prototypes ---------------------------- */

// fatal error messages (these don't return)
void FatalError(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void InternalError(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// display normal messages & warnings to the screen
void PrintMsg(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintVerbose(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintWarn(const char *str, ...) GCCATTR((format (printf, 1, 2)));
void PrintMiniWarn(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// set message for certain errors
void SetErrorMsg(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// endian handling
void InitEndian(void);
uint16_g Endian_U16(uint16_g);
uint32_g Endian_U32(uint32_g);

// these are only used for debugging
void InitDebug(void);
void TermDebug(void);
void PrintDebug(const char *str, ...) GCCATTR((format (printf, 1, 2)));

// macros for the display stuff
#define DisplayOpen        (* cur_funcs->display_open)
#define DisplaySetTitle    (* cur_funcs->display_setTitle)
#define DisplaySetBar      (* cur_funcs->display_setBar)
#define DisplaySetBarLimit (* cur_funcs->display_setBarLimit)
#define DisplaySetBarText  (* cur_funcs->display_setBarText)
#define DisplayClose       (* cur_funcs->display_close)

#define DisplayTicker      (* cur_funcs->ticker)

}  // namespace glbsp

#endif /* __GLBSP_SYSTEM_H__ */
//------------------------------------------------------------------------
// UTILITY : general purpose functions
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

#ifndef __GLBSP_UTIL_H__
#define __GLBSP_UTIL_H__

namespace glbsp
{

/* ----- useful macros ---------------------------- */

#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

#ifndef MAX
#define MAX(x,y)  ((x) > (y) ? (x) : (y))
#endif

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

#ifndef ABS
#define ABS(x)  ((x) >= 0 ? (x) : -(x))
#endif

#ifndef I_ROUND
#define I_ROUND(x)  ((int) (((x) < 0.0f) ? ((x) - 0.5f) : ((x) + 0.5f)))
#endif

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
angle_g UtilComputeAngle(float_g dx, float_g dy);
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

// check if the file exists.
int UtilFileExists(const char *filename);

// checksum functions
void Adler32_Begin(uint32_g *crc);
void Adler32_AddBlock(uint32_g *crc, const uint8_g *data, int length);
void Adler32_Finish(uint32_g *crc);

}  // namespace glbsp

#endif /* __GLBSP_UTIL_H__ */
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

#ifndef __GLBSP_BLOCKMAP_H__
#define __GLBSP_BLOCKMAP_H__

#include "structs.h"
#include "level.h"

namespace glbsp
{

#define DEFAULT_BLOCK_LIMIT  16000

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

}  // namespace glbsp

#endif /* __GLBSP_BLOCKMAP_H__ */
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

#ifndef __GLBSP_REJECT_H__
#define __GLBSP_REJECT_H__

#include "structs.h"
#include "level.h"

namespace glbsp
{

// build the reject table and write it into the REJECT lump
void PutReject(void);

}  // namespace glbsp

#endif /* __GLBSP_REJECT_H__ */
//------------------------------------------------------------------------
// LEVEL : Level structures & read/write functions.
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

#ifndef __GLBSP_LEVEL_H__
#define __GLBSP_LEVEL_H__

#include "structs.h"
#include "wad.h"


namespace glbsp
{

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
  float_g x, y;

  // vertex index.  Always valid after loading and pruning of unused
  // vertices has occurred.  For GL vertices, bit 30 will be set.
  int index;

  // reference count.  When building normal node info, unused vertices
  // will be pruned.
  int ref_count;

  // usually NULL, unless this vertex occupies the same location as a
  // previous vertex.  Only used during the pruning phase.
  struct vertex_s *equiv;

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
  float_g psx, psy;
  float_g pex, pey;
  float_g pdx, pdy;

  float_g p_length;
  float_g p_angle;
  float_g p_para;
  float_g p_perp;

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
  float_g mid_x;
  float_g mid_y;

  // this is normally FALSE, only set for the "no nodes hack"
  // [ see comments in the BuildNodes() function. ]
  int is_dummy;
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

// check whether the current level already has normal nodes
int CheckForNormalNodes(void);

// load all level data for the current level
void LoadLevel(void);

// free all level data
void FreeLevel(void);

// save the newly computed NODE info etc..
void SaveLevel(node_t *root_node);

}  // namespace glbsp

#endif /* __GLBSP_LEVEL_H__ */
//------------------------------------------------------------------------
// ANALYZE : Analyzing level structures
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

#ifndef __GLBSP_ANALYZE_H__
#define __GLBSP_ANALYZE_H__

#include "structs.h"
#include "level.h"

namespace glbsp
{

// detection routines
void DetectDuplicateVertices(void);
void DetectDuplicateSidedefs(void);
void DetectPolyobjSectors(void);
void DetectOverlappingLines(void);
void DetectWindowEffects(void);

// pruning routines
void PruneLinedefs(void);
void PruneVertices(void);
void PruneSidedefs(void);
void PruneSectors(void);

// computes the wall tips for all of the vertices
void CalculateWallTips(void);

// return a new vertex (with correct wall_tip info) for the split that
// happens along the given seg at the given location.
//
vertex_t *NewVertexFromSplitSeg(seg_t *seg, float_g x, float_g y);

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
sector_t * VertexCheckOpen(vertex_t *vert, float_g dx, float_g dy);

}  // namespace glbsp

#endif /* __GLBSP_ANALYZE_H__ */
//------------------------------------------------------------------------
// SEG : Choose the best Seg to use for a node line.
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

#ifndef __GLBSP_SEG_H__
#define __GLBSP_SEG_H__

#include "structs.h"


namespace glbsp
{

#define DEFAULT_FACTOR  11

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
  float_g along_dist;

  // TRUE if this intersection was on a self-referencing linedef
  boolean_g self_ref;

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

}  // namespace glbsp

#endif /* __GLBSP_SEG_H__ */
//------------------------------------------------------------------------
// NODE : Recursively create nodes and return the pointers.
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

#ifndef __GLBSP_NODE_H__
#define __GLBSP_NODE_H__

#include "structs.h"

namespace glbsp
{

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
// returns GLBSP_E_OK, or GLBSP_E_Cancelled if user stopped it.
//
glbsp_ret_e BuildNodes(superblock_t *seg_list,
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

}  // namespace glbsp

#endif /* __GLBSP_NODE_H__ */
