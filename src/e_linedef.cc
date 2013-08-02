//------------------------------------------------------------------------
//  LINEDEFS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2013 Andrew Apted
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

#include "main.h"

#include "e_cutpaste.h"
#include "e_linedef.h"
#include "editloop.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"
#include "w_rawdef.h"


// config items
bool leave_offsets_alone = true;


bool LineDefAlreadyExists(int v1, int v2)
{
	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		if (L->start == v1 && L->end == v2) return true;
		if (L->start == v2 && L->end == v1) return true;
	}

	return false;
}


/* return true if adding a line between v1 and v2 would overlap an
   existing line.  By "overlap" I mean parallel and sitting on top
   (this does NOT test for lines crossing each other).
*/
bool LineDefWouldOverlap(int v1, int x2, int y2)
{
	int x1 = Vertices[v1]->x;
	int y1 = Vertices[v1]->y;

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		LineDef *L = LineDefs[n];

		// zero-length lines should not exist, but don't get stroppy if they do
		if (L->isZeroLength())
			continue;

		double a, b;
		
		a = PerpDist(x1, y1, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);
		b = PerpDist(x2, y2, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);

		if (fabs(a) >= 2.0 || fabs(b) >= 2.0)
			continue;

		a = AlongDist(x1, y1, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);
		b = AlongDist(x2, y2, L->Start()->x, L->Start()->y, L->End()->x, L->End()->y);

		double len = L->CalcLength();

		if (a > b)
			std::swap(a, b);

		if (b < 0.5 || a > len - 0.5)
			continue;

		return true;
	}

	return false;
}


/*
  deletes all the linedefs AND unused vertices AND unused sidedefs
*/
void DeleteLineDefs(selection_c *lines)
{
	selection_c  verts(OBJ_VERTICES);
	selection_c  sides(OBJ_SIDEDEFS);

	UnusedVertices(lines, &verts);
	UnusedSideDefs(lines, &sides);

	DeleteObjects(lines);
	DeleteObjects(&verts);
	DeleteObjects(&sides);
}


/*
   get the absolute height from which the textures are drawn
*/

int GetTextureRefHeight (int sidedef)
{
	int l, sector;
	int otherside = OBJ_NO_NONE;

	/* find the sidedef on the other side of the LineDef, if any */

	for (l = 0 ; l < NumLineDefs ; l++)
	{
		if (LineDefs[l]->right == sidedef)
		{
			otherside = LineDefs[l]->left;
			break;
		}
		if (LineDefs[l]->left == sidedef)
		{
			otherside = LineDefs[l]->right;
			break;
		}
	}
	/* get the Sector number */

	sector = SideDefs[sidedef]->sector;
	/* if the upper texture is displayed,
	   then the reference is taken from the other Sector */
	if (otherside >= 0)
	{
		l = SideDefs[otherside]->sector;
		if (l > 0)
		{

			if (Sectors[l]->ceilh < Sectors[sector]->ceilh
					&& Sectors[l]->ceilh > Sectors[sector]->floorh)
				sector = l;
		}
	}
	/* return the altitude of the ceiling */

	if (sector >= 0)
		return Sectors[sector]->ceilh; /* textures are drawn from the ceiling down */
	else
		return 0; /* yuck! */
}




/*
   Align all textures for the given SideDefs

   Note from RQ:
      This function should be improved!
      But what should be improved first is the way the SideDefs are selected.
      It is stupid to change both sides of a wall when only one side needs
      to be changed.  But with the current selection method, there is no
      way to select only one side of a two-sided wall.
*/

#if (0 == 1)  // FIXME

void AlignTexturesY (SelPtr *sdlist)
{
	int h, refh;

	if (! *sdlist)
		return;

	/* get the reference height from the first sidedef */
	refh = GetTextureRefHeight ((*sdlist)->objnum);

	SideDefs[(*sdlist)->objnum]->y_offset = 0;
	UnSelectObject (sdlist, (*sdlist)->objnum);

	/* adjust Y offset in all other SideDefs */
	while (*sdlist)
	{
		h = GetTextureRefHeight ((*sdlist)->objnum);

		SideDefs[(*sdlist)->objnum]->y_offset = (refh - h) % 128;
		UnSelectObject (sdlist, (*sdlist)->objnum);
	}
	MarkChanges();
}



/*
   Function is to align all highlighted textures in the X-axis

   Note from RJH:
   LineDefs highlighted are read off in reverse order of highlighting.
   The '*sdlist' is in the reverse order of the above mentioned LineDefs
   i.e. the first linedef sidedefs you highlighted will be processed first.

   Note from RQ:
   See also the note for the previous function.

   Note from RJH:
   For the menu for aligning textures 'X' NOW operates upon the fact that
   ALL the SIDEDEFS from the selected LINEDEFS are in the *SDLIST, 2nd
   sidedef is first, 1st sidedef is 2nd). Aligning textures X now does
   SIDEDEF 1's and SIDEDEF 2's.  If the selection process is changed,
   the following needs to be altered radically.
*/

void AlignTexturesX (SelPtr *sdlist)
{
#if 0 // TODO: AlignTexturesX

	/* FIRST texture name used in the highlited objects */
	char texname[WAD_TEX_NAME + 1];
	char errormessage[80];  /* area to hold the error messages produced */
	int  ldef;    /* linedef number */
	int  sd1;   /* current sidedef in *sdlist */
	int  vert1, vert2;  /* vertex 1 and 2 for the linedef under scrutiny */
	int  xoffset;   /* xoffset accumulator */
	int  useroffset;  /* user input offset for first input */
	s16_t  texlength;   /* the length of texture to format to */
	int  length;    /* length of linedef under scrutiny */
	s16_t  dummy;   /* holds useless data */
	int  type_off;    /* do we have an initial offset to use */
	int  type_tex;    /* do we check for same textures */
	int  type_sd;   /* do we align sidedef 1 or sidedef 2 */

	type_sd  = 0;   /* which sidedef to align, 1=SideDef1, 2=SideDef2 */
	type_tex = 0;   /* do we test for similar textures, 0 = no, 1 = yes */
	type_off = 0;   /* do we have an inital offset, 0 = no, 1 = yes */

	vert1   = -1;
	vert2   = -1;   /* 1st time round the while loop the -1 value is needed */
	texlength  = 0;
	xoffset    = 0;
	useroffset = 0;

	switch (vDisplayMenu (250, 110, "Aligning textures (X offset) :",

				" Sidedef 1, Check for identical textures.     ", YK_, 0,
				" Sidedef 1, As above, but with inital offset. ", YK_, 0,
				" Sidedef 1, No texture checking.              ", YK_, 0,
				" Sidedef 1, As above, but with inital offset. ", YK_, 0,

				" Sidedef 2, Check for identical textures.     ", YK_, 0,
				" Sidedef 2, As above, but with inital offset. ", YK_, 0,
				" Sidedef 2, No texture checking.              ", YK_, 0,
				" Sidedef 2, As above, but with inital offset. ", YK_, 0,
				NULL))
	{
		case 1:       /* Sidedef 1 with checking for same textures   */
			type_sd = 1; type_tex = 1; type_off = 0;
			break;

		case 2:       /* Sidedef 1 as above, but with inital offset  */
			type_sd = 1; type_tex = 1; type_off = 1;
			break;

		case 3:       /* Sidedef 1 regardless of same textures       */
			type_sd = 1; type_tex = 0; type_off = 0;
			break;

		case 4:       /* Sidedef 1 as above, but with inital offset  */
			type_sd = 1; type_tex = 0; type_off = 1;
			break;

		case 5:       /* Sidedef 2 with checking for same textures   */
			type_sd = 2; type_tex = 1; type_off = 0;
			break;

		case 6:       /* Sidedef 2 as above, but with initial offset */
			type_sd = 2; type_tex = 1; type_off = 1;
			break;

		case 7:       /* Sidedef 2 regardless of same textures       */
			type_sd = 2; type_tex = 0; type_off = 0;
			break;

		case 8:       /* Sidedef 2 as above, but with initial offset */
			type_sd = 2; type_tex = 0; type_off = 1;
			break;
	}

	ldef = 0;
	if (! *sdlist)
	{
		Notify (-1, -1, "Error in AlignTexturesX: list is empty", 0);
		return;
	}
	sd1 = (*sdlist)->objnum;

	if (type_sd == 1) /* throw out all 2nd SideDefs untill a 1st is found */
	{
		while (*sdlist && LineDefs[ldef]->right!=sd1 && ldef<=NumLineDefs)
		{
			ldef++;
			if (LineDefs[ldef]->left == sd1)
			{
				UnSelectObject (sdlist, (*sdlist)->objnum);
				if (! *sdlist)
					return;
				sd1 = (*sdlist)->objnum;
				ldef = 0;
			}
		}
	}

	if (type_sd == 2) /* throw out all 1st SideDefs untill a 2nd is found */
	{
		while (LineDefs[ldef]->left!=sd1 && ldef<=NumLineDefs)
		{
			ldef++;
			if (LineDefs[ldef]->right == sd1)
			{
				UnSelectObject (sdlist, (*sdlist)->objnum);
				if (! *sdlist)
					return;
				sd1 = (*sdlist)->objnum;
				ldef = 0;
			}
		}
	}



	/* get texture name of the sidedef in the *sdlist) */
	strncpy (texname, SideDefs[(*sdlist)->objnum]->middle, WAD_TEX_NAME);

	/* test if there is a texture there */
	if (texname[0] == '-')
	{
		Beep ();
		sprintf (errormessage, "No texture for sidedef #%d.", (*sdlist)->objnum);
		Notify (-1, -1, errormessage, 0);
		return;
	}

	GetWallTextureSize (&texlength, &dummy, texname); /* clunky, but it works */

	/* get initial offset to use (if required) */
	if (type_off == 1)    /* source taken from InputObjectNumber */
	{
		int  x0;          /* left hand (x) window start     */
		int  y0;          /* top (y) window start           */
		int  key;         /* holds value returned by InputInteger */
		char prompt[80];  /* prompt for inital offset input */



	}

	while (*sdlist)  /* main processing loop */
	{
		ldef = 0;
		sd1 = (*sdlist)->objnum;

		if (type_sd == 1) /* throw out all 2nd SideDefs untill a 1st is found */
		{
			while (LineDefs[ldef]->right!=sd1 && ldef<=NumLineDefs)
			{
				ldef++;
				if (LineDefs[ldef]->left == sd1)
				{
					UnSelectObject (sdlist, (*sdlist)->objnum);
					sd1 = (*sdlist)->objnum;
					ldef = 0;
					if (! *sdlist)
						return;
				}
			}
		}

		if (type_sd == 2) /* throw out all 1st SideDefs untill a 2nd is found */
		{
			while (LineDefs[ldef]->left!=sd1 && ldef<=NumLineDefs)
			{
				ldef++;
				if (LineDefs[ldef]->right == sd1)
				{
					UnSelectObject (sdlist, (*sdlist)->objnum);
					sd1 = (*sdlist)->objnum;
					ldef = 0;
					if (! *sdlist)
						return;
				}
			}
		}

		/* do we test for same textures for the sidedef in question?? */
		if (type_tex == 1)
		{

			if (strncmp (SideDefs[(*sdlist)->objnum]->middle, texname,WAD_TEX_NAME))
			{
				Beep ();
				sprintf (errormessage, "No texture for sidedef #%d.", (*sdlist)->objnum);
				Notify (-1, -1, errormessage, 0);
				return;
			}
		}

		sd1 = (*sdlist)->objnum;
		ldef = 0;



		/* find out which linedef holds that sidedef */
		if (type_sd == 1)
		{
			while (LineDefs[ldef]->right != sd1 && ldef < NumLineDefs)
				ldef++;
		}
		else
		{
			while (LineDefs[ldef]->left != sd1 && ldef < NumLineDefs)
				ldef++;
		}

		vert1 = LineDefs[ldef]->start;
		/* test for linedef highlight continuity */
		if (vert1 != vert2 && vert2 != -1)
		{
			Beep ();
			sprintf (errormessage, "Linedef #%d is not contiguous"
					" with the previous linedef, please reselect.", (*sdlist)->objnum);
			Notify (-1, -1, errormessage, 0);
			return;
		}
		/* is this the first time round here */
		if (vert1 != vert2)
		{
			if (type_off == 1)  /* do we have an initial offset ? */
			{
				SideDefs[sd1]->x_offset = useroffset;
				xoffset = useroffset;
			}
			else
				SideDefs[sd1]->x_offset = 0;
		}
		else     /* put new xoffset into the sidedef */
			SideDefs[sd1]->x_offset = xoffset;

		/* calculate length of linedef */
		vert2 = LineDefs[ldef]->end;

		length = ComputeDist (Vertices[vert2]->x - Vertices[vert1]->x,
				Vertices[vert2]->y - Vertices[vert1]->y);

		xoffset += length;
		/* remove multiples of texlength from xoffset */
		xoffset = xoffset % texlength;
		/* move to next object in selected list */
		UnSelectObject (sdlist, (*sdlist)->objnum);
	}
	MarkChanges();

#endif
}

#endif


// this type represents one side of a linedef
typedef int side_on_a_line_t;

static inline side_on_a_line_t soal_make(int ld, int side)
{
	return (ld << 1) | (side < 0 ? 1 : 0);
}

static inline int soal_ld(side_on_a_line_t zz)
{
	return (zz >> 1);
}

static inline const LineDef * soal_LD_ptr(side_on_a_line_t zz)
{
	return LineDefs[soal_ld(zz)];
}

static inline int soal_side(side_on_a_line_t zz)
{
	return (zz & 1) ? SIDE_LEFT : SIDE_RIGHT;
}

static inline int soal_sd(side_on_a_line_t zz)
{
	return soal_LD_ptr(zz)->WhatSideDef(soal_side(zz));
}

static inline const SideDef * soal_SD_ptr(side_on_a_line_t zz)
{
	int sd = soal_sd(zz);
	return (sd >= 0) ? SideDefs[sd] : NULL;
}


static int PartialTexCmp(const char *A, const char *B)
{
	// only compare the first 4 characters

	char A2[64];
	char B2[64];

	strcpy(A2, A);
	strcpy(B2, B);

	A2[4] = B2[4] = 0;

	return y_stricmp(A2, B2);
}


static bool WantToAlignSideDef(int ld, int side, const char *flags)
{
	const LineDef *L = LineDefs[ld];

	int sd    = (side < 0) ? L->left : L->right;
	int other = (side > 0) ? L->left : L->right;

	if (sd < 0)
		return false;

	if (strchr(flags, 'f')) return (side > 0);
	if (strchr(flags, 'b')) return (side < 0);

	// always do single-sided lines
	if (other < 0)
		return true;

	// FIXME: check for railing texture ??

	// require something visible
	// TODO: have a flag to not require this?
	const Sector *front = SideDefs[sd]->SecRef();
	const Sector *back  = SideDefs[other]->SecRef();

	if (back->floorh > front->floorh) return true;
	if (back->ceilh  < front->ceilh)  return true;

	return false;
}


static int ScoreAdjoiner(side_on_a_line_t zz, side_on_a_line_t adj,
						 const char *flags, bool only_U, bool only_L)
{
	const LineDef *L = soal_LD_ptr(zz);
	const LineDef *N = soal_LD_ptr(adj);

	const SideDef *LS = soal_SD_ptr(zz);
	const SideDef *NS = soal_SD_ptr(adj);

	SYS_ASSERT(LS);

	// no sidedef on adjoining line?
	if (! NS)
		return -2;

	// wrong side?
	if (soal_side(zz) == soal_side(adj))
	{
		if (N->start == L->start) return -1;
		if (N->end   == L->end)   return -1;
	}
	else
	{
		if (N->start == L->end)   return -1;
		if (N->end   == L->start) return -1;
	}


	int score = 1;

	// Main requirement is a matching texture.
	// There are three cases depending on number of sides:
	//
	// (a) single <-> single : easy
	//
	// (b) double <-> double : compare lower/lower and upper/upper
	//                         [only need one to match]
	//
	// (c) single <-> double : compare mid/lower and mid/upper
	//
	bool matched = false;

	for (int part = 0 ; part < 2 ; part++)
	{
		if (part == 0 && only_U) continue;
		if (part == 1 && only_L) continue;

		const char *L_tex = (! L->TwoSided()) ? LS->MidTex() :
							(part & 1)        ? LS->UpperTex() :
							                    LS->LowerTex();

		const char *N_tex = (! N->TwoSided()) ? NS->MidTex() :
							(part & 1)        ? NS->UpperTex() :
							                    NS->LowerTex();

		if (L_tex[0] == '-') continue;
		if (N_tex[0] == '-') continue;

		if (PartialTexCmp(L_tex, N_tex) == 0)
			matched = true;
	}

	if (matched)
		score = score + 20;

	// prefer if adjoiner is "to the left" of this sidedef
	bool on_left = N->TouchesVertex(soal_side(zz) < 0 ? L->end : L->start);

	if (on_left)
		score = score + 10;

	// prefer both lines to have same sided-ness
	if (L->OneSided() == N->OneSided())
		score = score + 5;

	// slight preference for same sector
	if (LS->sector == NS->sector)
		score = score + 1;

	return score;
}


static side_on_a_line_t DetermineAdjoiner(side_on_a_line_t zz, const char *flags)
{
	// returns -1 for none

	int best_adj   = -1;
	int best_score = -1;

	bool only_U = strchr(flags, 'u') ? true : false;
	bool only_L = strchr(flags, 'l') ? true : false;

	const LineDef *L = soal_LD_ptr(zz);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *N = LineDefs[n];

		if (N == L)
			continue;

		if (! (N->TouchesVertex(L->start) || N->TouchesVertex(L->end)))
			continue;

		for (int pass = 0 ; pass < 2 ; pass++)
		{
			int adj_side = pass ? SIDE_LEFT : SIDE_RIGHT;
			int adjoiner = soal_make(n, adj_side);

			int score = ScoreAdjoiner(zz, adjoiner, flags, only_U, only_L);

			if (score > 0 && score > best_score)
			{
				best_adj   = adjoiner;
				best_score = score;
			}
		}
	}

	return best_adj;
}


#if 1  // TEST CRUD
int TestAdjoinerLineDef(int ld)
{
	side_on_a_line_t zz = soal_make(ld, SIDE_RIGHT);

	if (soal_sd(zz) < 0)
		return -1;

	side_on_a_line_t result = DetermineAdjoiner(zz, "");

	if (result >= 0)
		return soal_ld(result);

	return -1;
}
#endif


static void DoAlignSideDef(side_on_a_line_t zz, side_on_a_line_t adj,
                           const char *flags, bool do_X, bool do_Y)
{
	const LineDef *L  = soal_LD_ptr(zz);
	const SideDef *SD = soal_SD_ptr(zz);

	bool only_U = strchr(flags, 'u') ? true : false;
	bool only_L = strchr(flags, 'l') ? true : false;

	// unpeg flags....
	if (do_Y && L->TwoSided() &&
	    ! (L->flags & MLF_LowerUnpegged) &&
	    ! (L->flags & MLF_UpperUnpegged) &&
	    SD->MidTex()[0] == '-' &&
	    SD->LowerTex()[0] != '-' &&
	    PartialTexCmp(SD->LowerTex(), SD->UpperTex()) == 0)
	{
		int new_flags = L->flags;

		if (! only_L) new_flags |= MLF_LowerUnpegged;
		if (! only_U) new_flags |= MLF_UpperUnpegged;

		BA_ChangeLD(soal_ld(zz), LineDef::F_FLAGS, new_flags);
	}

	int sd = soal_sd(zz);

	// no adjoiner case : simply clear the offset(s)
	if (adj < 0)
	{
		if (do_X) BA_ChangeSD(sd, SideDef::F_X_OFFSET, 0);
		if (do_Y) BA_ChangeSD(sd, SideDef::F_Y_OFFSET, 0);

		return;
	}

	int adj_length = I_ROUND(soal_LD_ptr(adj)->CalcLength());

	// FIXME !!!

	if (do_X) BA_ChangeSD(sd, SideDef::F_X_OFFSET, soal_SD_ptr(adj)->x_offset + adj_length);
}


static int CheckSideInList(side_on_a_line_t zz,
						   std::vector<side_on_a_line_t>& sides)
{
	for (unsigned int k = 0 ; k < sides.size() ; k++)
		if (sides[k] == zz)
			return true;

	return false;
}


static int PickSideDefToAlign(std::vector<side_on_a_line_t>& sides,
							  std::vector<side_on_a_line_t>& adjoiners)
{
	// want a sidedef whose adjoiner is not in the sidedef list
	// (since we need to process the adjoiner _before_ the sidedef).
	// however there could be loops, so must pick something.

	unsigned int k;

	for (k = 0 ; k < sides.size() ; k++)
	{
		if (sides[k] < 0)
			continue;

		side_on_a_line_t adj = adjoiners[k];

		if (adj >= 0 && CheckSideInList(adj, sides))
			continue;

		return (int)k;
	}

	for (k = 0 ; k < sides.size() ; k++)
		if (sides[k] >= 0)
			return (int)k;

	return -1;  // none left
}


/* Align textures on linedefs.
 *
 * Possible flags:
 *    x : align X offset
 *    y : align Y offset
 *
 *    u : upper only
 *    l : lower / middle only
 *
 *    f : front side only
 *    b : back side only
 *
 */
void LIN_Align(void)
{
	const char *flags = EXEC_Param[0];

	bool do_X = strchr(flags, 'x') ? true : false;
	bool do_Y = strchr(flags, 'y') ? true : false;

	if (! (do_X || do_Y))
	{
		Beep("LIN_Align: need x or y flag");
		return;
	}

	selection_c lines(OBJ_LINEDEFS);
	selection_iterator_c it;

	if (! GetCurrentObjects(&lines) || NumSideDefs == 0)
	{
		Beep("No lines to align");
		return;
	}


	/* collect sidedefs to align */

	std::vector<side_on_a_line_t>  sides;
	std::vector<side_on_a_line_t>  adjoiners;

	for (lines.begin(&it) ; !it.at_end() ; ++it)
	{
		int ld = *it;

		for (int pass = 0 ; pass < 2 ; pass++)
		{
			int side = pass ? SIDE_LEFT : SIDE_RIGHT;

			if (WantToAlignSideDef(ld, pass ? SIDE_LEFT : SIDE_RIGHT, flags))
			{
				side_on_a_line_t zz = soal_make(ld, side);

				sides.push_back(zz);

				adjoiners.push_back(DetermineAdjoiner(zz, flags));
			}
		}
	}


	/* process each sidedef */

	BA_Begin();

	for (;;)
	{
		int index = PickSideDefToAlign(sides, adjoiners);

		if (index < 0)
			break;

		DoAlignSideDef(sides[index], adjoiners[index], flags, do_X, do_Y);

		sides[index] = adjoiners[index] = -1;
	}

	BA_End();
}


//------------------------------------------------------------------------


void FlipLineDef(int ld)
{
	int old_start = LineDefs[ld]->start;
	int old_end   = LineDefs[ld]->end;

	BA_ChangeLD(ld, LineDef::F_START, old_end);
	BA_ChangeLD(ld, LineDef::F_END, old_start);

	int old_right = LineDefs[ld]->right;
	int old_left  = LineDefs[ld]->left;

	BA_ChangeLD(ld, LineDef::F_RIGHT, old_left);
	BA_ChangeLD(ld, LineDef::F_LEFT, old_right);
}


void FlipLineDefGroup(selection_c& flip)
{
	selection_iterator_c it;

	for (flip.begin(&it) ; !it.at_end() ; ++it)
	{
		FlipLineDef(*it);
	}
}


/*
   flip one or several LineDefs
*/
void LIN_Flip(void)
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to flip");
		return;
	}

	BA_Begin();

	FlipLineDefGroup(list);

	BA_End();
}


int SplitLineDefAtVertex(int ld, int new_v)
{
	LineDef * L = LineDefs[ld];
	Vertex  * V = Vertices[new_v];

	// create new linedef
	int new_l = BA_New(OBJ_LINEDEFS);
	
	LineDef * L2 = LineDefs[new_l];

	// it is OK to directly set fields of newly created objects
	L2->RawCopy(L);

	L2->start = new_v;
	L2->end   = L->end;

	// update vertex on original line
	BA_ChangeLD(ld, LineDef::F_END, new_v);

	// compute lengths (to update sidedef X offsets)
	int orig_length = (int)ComputeDist(
			L->Start()->x - L->End()->x,
			L->Start()->y - L->End()->y);

	int new_length = (int)ComputeDist(
			L->Start()->x - V->x,
			L->Start()->y - V->y);

	// update sidedefs

	if (L->Right())
	{
		L2->right = BA_New(OBJ_SIDEDEFS);
		L2->Right()->RawCopy(L->Right());

		if (! leave_offsets_alone)
			L2->Right()->x_offset += new_length;
	}

	if (L->Left())
	{
		L2->left = BA_New(OBJ_SIDEDEFS);
		L2->Left()->RawCopy(L->Left());

		if (! leave_offsets_alone)
		{
			int new_x_ofs = L->Left()->x_offset + orig_length - new_length;

			BA_ChangeSD(L->left, SideDef::F_X_OFFSET, new_x_ofs);
		}
	}

	return new_l;
}


static bool DoSplitLineDef(int ld)
{
	LineDef * L = LineDefs[ld];

	int new_x = (L->Start()->x + L->End()->x) / 2;
	int new_y = (L->Start()->y + L->End()->y) / 2;

	// prevent creating tiny lines (especially zero-length)
	if (abs(L->Start()->x - L->End()->x) < 4 &&
	    abs(L->Start()->y - L->End()->y) < 4)
		return false;

	int new_v = BA_New(OBJ_VERTICES);

	Vertex * V = Vertices[new_v];

	V->x = new_x;
	V->y = new_y;

	SplitLineDefAtVertex(ld, new_v);

	return true;
}


/*
   split one or more LineDefs in two, adding new Vertices in the middle
*/
void LIN_SplitHalf(void)
{
	selection_c list;
	selection_iterator_c it;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to split");
		return;
	}

	bool was_selected = edit.Selected->notempty();

	// clear current selection, since the size needs to grow due to
	// new linedefs being added to the map.
	edit.Selected->clear_all();

	int new_first = NumLineDefs;
	int new_count = 0;

	BA_Begin();

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		if (DoSplitLineDef(*it))
			new_count++;
	}

	BA_End();

	// Hmmmmm -- should abort early if some lines are too short??
	if (new_count < list.count_obj())
		Beep("Some lines were too short!");

	if (was_selected && new_count > 0)
	{
		// reselect the old _and_ new linedefs
		for (list.begin(&it) ; !it.at_end() ; ++it)
			edit.Selected->set(*it);

		edit.Selected->frob_range(new_first, new_first + new_count - 1, BOP_ADD);
	}
}


void LD_MergedSecondSideDef(int ld)
{
	// similar to above, but with existing sidedefs

	LineDef * L = LineDefs[ld];

	SYS_ASSERT(L->TwoSided());

	int new_flags = L->flags;

	new_flags |=  MLF_TwoSided;
	new_flags &= ~MLF_Blocking;

	BA_ChangeLD(ld, LineDef::F_FLAGS, new_flags);

	// FIXME: make this a global pseudo-constant
	int null_tex = BA_InternaliseString("-");

	// determine textures for each side
	int  left_tex = 0;
	int right_tex = 0;

	if (isalnum(L->Left()->MidTex()[0]))
		left_tex = L->Left()->mid_tex;

	if (isalnum(L->Right()->MidTex()[0]))
		right_tex = L->Right()->mid_tex;
	
	if (! left_tex)  left_tex = right_tex;
	if (! right_tex) right_tex = left_tex;

	// use default texture if both sides are empty
	if (! left_tex)
	{
		 left_tex = BA_InternaliseString(default_mid_tex);
		right_tex = left_tex;
	}

	BA_ChangeSD(L->left,  SideDef::F_MID_TEX, null_tex);
	BA_ChangeSD(L->right, SideDef::F_MID_TEX, null_tex);

	BA_ChangeSD(L->left,  SideDef::F_LOWER_TEX, left_tex);
	BA_ChangeSD(L->left,  SideDef::F_UPPER_TEX, left_tex);

	BA_ChangeSD(L->right, SideDef::F_LOWER_TEX, right_tex);
	BA_ChangeSD(L->right, SideDef::F_UPPER_TEX, right_tex);
}


void LIN_MergeTwo(void)
{
	if (edit.Selected->count_obj() == 1 && edit.highlighted())
	{
		edit.Selected->set(edit.highlighted.num);
	}

	if (edit.Selected->count_obj() != 2)
	{
		Beep("Need 2 linedefs to merge (got %d)", edit.Selected->count_obj());
		return;
	}

	// we will merge the second into the first

	int ld2 = edit.Selected->find_first();
	int ld1 = edit.Selected->find_second();

	const LineDef * L1 = LineDefs[ld1];
	const LineDef * L2 = LineDefs[ld2];

	if (! (L1->OneSided() && L2->OneSided()))
	{
		Beep("Linedefs to merge must be single sided.");
		return;
	}

	edit.Selected->clear_all();


	BA_Begin();

	// ld2 steals the sidedef from ld1

	BA_ChangeLD(ld2, LineDef::F_LEFT, L1->right);
	BA_ChangeLD(ld1, LineDef::F_RIGHT, -1);

	LD_MergedSecondSideDef(ld2);

	// fix existing lines connected to ld1 : reconnect to ld2

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		if (n == ld1 || n == ld2)
			continue;

		const LineDef * L = LineDefs[n];

		if (L->start == L1->start)
			BA_ChangeLD(n, LineDef::F_START, L2->end);
		else if (L->start == L1->end)
			BA_ChangeLD(n, LineDef::F_START, L2->start);

		if (L->end == L1->start)
			BA_ChangeLD(n, LineDef::F_END, L2->end);
		else if (L->end == L1->end)
			BA_ChangeLD(n, LineDef::F_END, L2->start);
	}

	// delete ld1 and any unused vertices

	selection_c del_line(OBJ_LINEDEFS);

	del_line.set(ld1);

	DeleteLineDefs(&del_line);

	BA_End();
}


void MoveCoordOntoLineDef(int ld, int *x, int *y)
{
	const LineDef *L = LineDefs[ld];

	double x1 = L->Start()->x;
	double y1 = L->Start()->y;
	double x2 = L->End()->x;
	double y2 = L->End()->y;

	double dx = x2 - x1;
	double dy = y2 - y1;

	double len_squared = dx*dx + dy*dy;

	SYS_ASSERT(len_squared > 0);

	// compute along distance
	double along = (*x - x1) * dx + (*y - y1) * dy;

	// result = start + along * line unit vector
	double new_x = x1 + along * dx / len_squared;
	double new_y = y1 + along * dy / len_squared;

	*x = I_ROUND(new_x);
	*y = I_ROUND(new_y);
}


static bool LineDefStartWillBeMoved(int ld, selection_c& list)
{
	int start = LineDefs[ld]->start;

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
	{
		if (*it == ld)
			continue;

		const LineDef *L = LineDefs[*it];

		if (L->end == start)
			return true;
	}

	return false;
}


static int PickLineDefToExtend(selection_c& list)
{
	// we want a line whose start is not going to be moved in the future
	// (otherwise the length will be wrecked by the later change).
	// however there could be loops, so need to always pick something.

	selection_iterator_c it;

	for (list.begin(&it) ; !it.at_end() ; ++it)
		if (! LineDefStartWillBeMoved(*it, list))
			return *it;

	return list.find_first();
}


static void LD_SetLength(int ld, int new_len, int angle)
{
	const LineDef *L = LineDefs[ld];

	double dx = new_len * cos(angle * M_PI / 32768.0);
	double dy = new_len * sin(angle * M_PI / 32768.0);

	int idx = I_ROUND(dx);
	int idy = I_ROUND(dy);

	if (idx == 0 && idy == 0)
	{
		if (dx < 0) idx = (int)floor(dx); else idx = (int)ceil(dx);
		if (dy < 0) idy = (int)floor(dy); else idy = (int)ceil(dy);
	}

	if (idx == 0 && idy == 0)
		idx = 1;

	BA_ChangeVT(L->end, Vertex::F_X, L->Start()->x + idx);
	BA_ChangeVT(L->end, Vertex::F_Y, L->Start()->y + idy);
}


void LineDefs_SetLength(int new_len)
{
	selection_c list;

	if (! GetCurrentObjects(&list))
	{
		Beep("No lines to extend");
		return;
	}

	// remember angles
	std::vector<int> angles(NumLineDefs);

	for (int n = 0 ; n < NumLineDefs ; n++)
	{
		const LineDef *L = LineDefs[n];

		angles[n] = ComputeAngle(L->End()->x - L->Start()->x,
								 L->End()->y - L->Start()->y);
	}

	BA_Begin();

	while (! list.empty())
	{
		int ld = PickLineDefToExtend(list);

		list.clear(ld);

		LD_SetLength(ld, new_len, angles[ld]);
	}

	BA_End();
}


#if 0  // FIXME: MakeRectangularNook

/*
 *  MakeRectangularNook - Make a nook or boss in a wall
 *  
 *  Before :    After :
 *                                  ^--->
 *                                  |   |
 *  +----------------->     +------->   v------->
 *      1st sidedef             1st sidedef
 *
 *  The length of the sides of the nook is sidelen.
 *  This is true when convex is false. If convex is true, the nook
 *  is actually a bump when viewed from the 1st sidedef.
 */
void MakeRectangularNook (SelPtr obj, int width, int depth, int convex)
{
	....
}

#endif


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
