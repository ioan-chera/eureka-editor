//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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
#include "e_checks.h"
#include "m_dialog.h"
#include "levels.h"
#include "objects.h"
#include "selectn.h"
#include "w_rawdef.h"


static void CheckingObjects ();

// TODO: REMOVE THIS
#define Expert  true


/*
 *  CheckLevel
 *  check the level consistency
 *  FIXME should be integrated into editloop.cc
 */
void CheckLevel (int x0, int y0)
{
#if 0 // TODO: CheckLevel

	const char *line5 = 0;

	if (Registered)
	{
		/* Commented out AYM 19971130 - wouldn't work with Doom 2
		   if (! FindMasterDir (MasterDir, "TEXTURE2"))
		   NumVertexes--;
		   else
		   */
		line5 = "Check texture names";
	}
	switch (vDisplayMenu (x0, y0, "Check level consistency",
				"Number of objects",   YK_, 0,
				"Check if all Sectors are closed", YK_, 0,
				"Check all cross-references",  YK_, 0,
				"Check for missing textures",  YK_, 0,
				line5,       YK_, 0,
				NULL))
	{
		case 1:
			Statistics ();
			break;
		case 2:
			CheckSectors ();
			break;
		case 3:
			CheckCrossReferences ();
			break;
		case 4:
			CheckTextures ();
			break;
		case 5:
			CheckTextureNames ();
			break;
	}
#endif
}


/*
   display number of objects, etc.
   */

void Statistics ()
{
	// FIXME !!!
#if 0
	int height;
	int width;
	int out_x0;
	int out_y0;
	int text_x0;
	int text_y0;

	width  = 2 * BOX_BORDER + 2 * WIDE_HSPACING + 33 * FONTW;
	height = 2 * BOX_BORDER + 2 * WIDE_VSPACING + 9 * FONTH;
	out_x0 = (ScrMaxX + 1 - width) / 2;
	out_y0 = (ScrMaxY + 1 - height) / 2;
	text_x0 = out_x0 + BOX_BORDER + WIDE_HSPACING;
	text_y0 = out_y0 + BOX_BORDER + WIDE_VSPACING;

	DrawScreenBox3D (out_x0, out_y0, out_x0 + width - 1, out_y0 + height - 1);
	set_colour (WHITE);
	DrawScreenText (text_x0, text_y0, "Statistics");
	DrawScreenText (-1, -1, "");

	if (! Things)
		set_colour (WINFG_DIM);
	else
		set_colour (WINFG);
	DrawScreenText (-1, -1, "Number of things:   %4d (%lu K)", NumThings,
			((unsigned long) NumThings * sizeof (struct Thing) + 512L) / 1024L);

	if (! Vertices)
		set_colour (WINFG_DIM);
	else
		set_colour (WINFG);
	DrawScreenText (-1, -1, "Number of vertices: %4d (%lu K)", NumVertices,
			((unsigned long) NumVertices * sizeof (struct Vertex) + 512L) / 1024L);

	if (! LineDefs)
		set_colour (WINFG_DIM);
	else
		set_colour (WINFG);
	DrawScreenText (-1, -1, "Number of linedefs: %4d (%lu K)", NumLineDefs,
			((unsigned long) NumLineDefs * sizeof (struct LineDef) + 512L) / 1024L);

	if (! SideDefs)
		set_colour (WINFG_DIM);
	else
		set_colour (WINFG);
	DrawScreenText (-1, -1, "Number of sidedefs: %4d (%lu K)", NumSideDefs,
			((unsigned long) NumSideDefs * sizeof (struct SideDef) + 512L) / 1024L);

	if (! Sectors)
		set_colour (WINFG_DIM);
	else
		set_colour (WINFG);
	DrawScreenText (-1, -1, "Number of sectors:  %4d (%lu K)", NumSectors,
			((unsigned long) NumSectors * sizeof (struct Sector) + 512L) / 1024L);

	DrawScreenText (-1, -1, "");
	set_colour (WINTITLE);
	DrawScreenText (-1, -1, "Press any key to continue...");
#endif
}


/*
   display a message while the user is waiting...
   */

static void CheckingObjects ()
{
	//DisplayMessage (-1, -1, "Grinding...");
}


/*
   display a message, then ask if the check should continue (prompt2 may be NULL)
   */

bool CheckFailed (int x0, int y0, char *prompt1, char *prompt2, bool fatal,
		bool &first_time)
{
#if 0
	int key;
	int maxlen;

	if (fatal)
		maxlen = 44;
	else
		maxlen = 27;
	if (strlen (prompt1) > maxlen)
		maxlen = strlen (prompt1);
	if (prompt2 && strlen (prompt2) > maxlen)
		maxlen = strlen (prompt2);
	int width = 2 * BOX_BORDER + 2 * WIDE_HSPACING + FONTW * maxlen;
	int height = 2 * BOX_BORDER + 2 * WIDE_VSPACING + FONTH * (prompt2 ? 6 : 5);
	if (x0 < 0)
		x0 = (ScrMaxX - width) / 2;
	if (y0 < 0)
		y0 = (ScrMaxY - height) / 2;
	int text_x0 = x0 + BOX_BORDER + WIDE_HSPACING;
	int text_y0 = y0 + BOX_BORDER + WIDE_VSPACING;
	int cur_y = text_y0;
	DrawScreenBox3D (x0, y0, x0 + width - 1, y0 + height - 1);
	set_colour (LIGHTRED);
	DrawScreenText (text_x0, cur_y, "Verification failed:");
	if (first_time)
		Beep ();
	set_colour (WHITE);
	DrawScreenText (text_x0, cur_y += FONTH, prompt1);
	LogPrintf("\t%s\n", prompt1);
	if (prompt2)
	{
		DrawScreenText (text_x0, cur_y += FONTH, prompt2);
		LogPrintf("\t%s\n", prompt2);
	}
	if (fatal)
	{
		DrawScreenText (text_x0, cur_y += 2 * FONTH,
				"The game will crash if you play with this level.");
		set_colour (WINTITLE);
		DrawScreenText (text_x0, cur_y += FONTH,
				"Press any key to see the object");
		LogPrintf("\n");
	}
	else
	{
		set_colour (WINTITLE);
		DrawScreenText (text_x0, cur_y += 2 * FONTH,
				"Press Esc to see the object,");
		DrawScreenText (text_x0, cur_y += FONTH,
				"or any other key to continue");
	}
	key = FL_Escape; //!!!! get_key ();
	if (key != FL_Escape)
	{
		DrawScreenBox3D (x0, y0, x0 + width - 1, y0 + height - 1);
		//   DrawScreenText (x0 + 10 + 4 * (maxlen - 26), y0 + 28,
		//      "Verifying other objects...");
	}
	first_time = false;
	return (key == FL_Escape);
#else
	return false;
#endif
}


/*
   check if all sectors are closed
   */

void CheckSectors ()
{
#if 0  // TODO
	int        s, n, sd;
	char *ends;
	char       msg1[80], msg2[80];
	bool       first_time = true;

	CheckingObjects ();
	LogPrintf("\nVerifying Sectors...\n");

	ends = (char *) GetMemory (NumVertices);
	for (s = 0 ; s < NumSectors ; s++)
	{
		/* clear the "ends" array */
		for (n = 0 ; n < NumVertices ; n++)
			ends[n] = 0;
		/* for each sidedef bound to the Sector, store a "1" in the "ends" */
		/* array for its starting Vertex, and a "2" for its ending Vertex  */
		for (n = 0 ; n < NumLineDefs ; n++)
		{
			sd = LineDefs[n].side_R;
			if (sd >= 0 && SideDefs[sd].sector == s)
			{
				ends[LineDefs[n].start] |= 1;
				ends[LineDefs[n].end] |= 2;
			}
			sd = LineDefs[n].side_L;
			if (sd >= 0 && SideDefs[sd].sector == s)
			{
				ends[LineDefs[n].end] |= 1;
				ends[LineDefs[n].start] |= 2;
			}
		}
		/* every entry in the "ends" array should be "0" or "3" */
		for (n = 0 ; n < NumVertices ; n++)
		{
			if (ends[n] == 1)
			{
				sprintf (msg1, "Sector #%d is not closed!", s);
				sprintf (msg2, "There is no sidedef ending at Vertex #%d", n);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_VERTICES, n));
					return;
				}
			}
			if (ends[n] == 2)
			{
				sprintf (msg1, "Sector #%d is not closed!", s);
				sprintf (msg2, "There is no sidedef starting at Vertex #%d", n);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_VERTICES, n));
					return;
				}
			}
		}
	}
	FreeMemory (ends);

	/*
	   Note from RQ:
	   This is a very simple idea, but it works!  The first test (above)
	   checks that all Sectors are closed.  But if a closed set of LineDefs
	   is moved out of a Sector and has all its "external" SideDefs pointing
	   to that Sector instead of the new one, then we need a second test.
	   That's why I check if the SideDefs facing each other are bound to
	   the same Sector.

	   Other note from RQ:
	   Nowadays, what makes the power of a good editor is its automatic tests.
	   So, if you are writing another Doom editor, you will probably want
	   to do the same kind of tests in your program.  Fine, but if you use
	   these ideas, don't forget to credit DEU...  Just a reminder... :-)
	   */

	/* now check if all SideDefs are facing a sidedef with the same Sector number */
	for (n = 0 ; n < NumLineDefs ; n++)
	{
		sd = LineDefs[n].side_R;
		if (sd >= 0)
		{
			s = GetOppositeSector (n, 1);

			if (s < 0 || SideDefs[sd].sector != s)
			{
				if (s < 0)
				{
					sprintf (msg1, "Sector #%d is not closed!", SideDefs[sd].sector);
					sprintf (msg2, "Check linedef #%d (first sidedef: #%d)", n, sd);
				}
				else
				{
					sprintf (msg1, "Sectors #%d and #%d are not closed!",
							SideDefs[sd].sector, s);
					sprintf (msg2, "Check linedef #%d (first sidedef: #%d)"
							" and the one facing it", n, sd);
				}
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
			}
		}

		sd = LineDefs[n].side_L;
		if (sd >= 0)
		{
			s = GetOppositeSector (n, 0);

			if (s < 0 || SideDefs[sd].sector != s)
			{
				if (s < 0)
				{
					sprintf (msg1, "Sector #%d is not closed!", SideDefs[sd].sector);
					sprintf (msg2, "Check linedef #%d (second sidedef: #%d)", n, sd);
				}
				else
				{
					sprintf (msg1, "Sectors #%d and #%d are not closed!",
							SideDefs[sd].sector, s);
					sprintf (msg2, "Check linedef #%d (second sidedef: #%d)"
							" and the one facing it", n, sd);
				}
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
			}
		}
	}
#endif
}


/*
   check cross-references and delete unused objects
   */

void CheckCrossReferences ()
{
// FIXME !!!!  CheckCrossReferences
#if 0

	char   msg[80];
	int    n, m;
	SelPtr cur;
	bool   first_time = true;

	CheckingObjects ();
	LogPrintf("\nVerifying cross-references...\n");

	for (n = 0 ; n < NumLineDefs ; n++)
	{
		/* Check for missing first sidedefs */
		if (LineDefs[n].side_R < 0)
		{
			sprintf (msg, "ERROR: linedef #%d has no first sidedef!", n);
			CheckFailed (-1, -1, msg, 0, 1, first_time);
			GoToObject (Objid (OBJ_LINEDEFS, n));
			return;
		}

		// FIXME should make this a mere warning
#ifdef OLD  /* ifdef'd out AYM 19970725, I do it and it works */
		/* Check for sidedefs used twice in the same linedef */
		if (LineDefs[n].sidedef1 == LineDefs[n].sidedef2)
		{
			sprintf (msg, "ERROR: linedef #%d uses the same sidedef twice (#%d)",
					n, LineDefs[n].sidedef1);
			CheckFailed (-1, -1, msg, 0, 1, first_time);
			GoToObject (Objid (OBJ_LINEDEFS, n));
			return;
		}
#endif

		/* Check for vertices used twice in the same linedef */
		if (LineDefs[n].start == LineDefs[n].end)
		{
			sprintf (msg, "ERROR: linedef #%d uses the same vertex twice (#%d)",
					n, LineDefs[n].start);
			CheckFailed (-1, -1, msg, 0, 1, first_time);
			GoToObject (Objid (OBJ_LINEDEFS, n));
			return;
		}
	}

	/* check if there aren't two linedefs between the same Vertices */
	cur = 0;
	/* AYM 19980203 FIXME should use new algorithm */
	for (n = NumLineDefs - 1 ; n >= 1 ; n--)
	{
		for (m = n - 1 ; m >= 0 ; m--)
			if ((LineDefs[n].start == LineDefs[m].start
						&& LineDefs[n].end   == LineDefs[m].end)
					|| (LineDefs[n].start == LineDefs[m].end
						&& LineDefs[n].end   == LineDefs[m].start))
			{
				SelectObject (&cur, n);
				break;
			}
	}
	if (cur
			&& (Expert
				|| Confirm (-1, -1, "There are multiple linedefs between the same vertices",
					"Do you want to delete the redundant linedefs?")))
		DeleteObjects (OBJ_LINEDEFS, &cur);
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	/* check for invalid flags in the linedefs */
	for (n = 0 ; n < NumLineDefs ; n++)
		if ((LineDefs[n].flags & 0x01) == 0 && LineDefs[n].side_L < 0)
			SelectObject (&cur, n);
	if (cur && (Expert
				|| Confirm (-1, -1, "Some linedefs have only one side but their I flag is"
					" not set",
					"Do you want to set the 'Impassible' flag?")))
		while (cur)
		{
			LogPrintf("Check: 1-sided linedef without I flag: %d", cur->objnum);
			LineDefs[cur->objnum].flags |= 0x01;
			UnSelectObject (&cur, cur->objnum);
		}
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	for (n = 0 ; n < NumLineDefs ; n++)
		if ((LineDefs[n].flags & 0x04) != 0 && LineDefs[n].side_L < 0)
			SelectObject (&cur, n);
	if (cur
			&& (Expert
				|| Confirm (-1, -1, "Some linedefs have only one side but their 2 flag"
					" is set",
					"Do you want to clear the 'two-sided' flag?")))
	{
		while (cur)
		{
			LogPrintf("Check: 1-sided linedef with 2s bit: %d", cur->objnum);
			LineDefs[cur->objnum].flags &= ~0x04;
			UnSelectObject (&cur, cur->objnum);
		}
	}
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	for (n = 0 ; n < NumLineDefs ; n++)
		if ((LineDefs[n].flags & 0x04) == 0 && LineDefs[n].side_L >= 0)
			SelectObject (&cur, n);
	if (cur
			&& (Expert
				|| Confirm (-1, -1,
					"Some linedefs have two sides but their 2S bit is not set",
					"Do you want to set the 'two-sided' flag?")))
	{
		while (cur)
		{
			LineDefs[cur->objnum].flags |= 0x04;
			UnSelectObject (&cur, cur->objnum);
		}
	}
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	/* select all Vertices */
	for (n = 0 ; n < NumVertices ; n++)
		SelectObject (&cur, n);
	/* unselect Vertices used in a LineDef */
	for (n = 0 ; n < NumLineDefs ; n++)
	{
		m = LineDefs[n].start;
		if (cur && m >= 0)
			UnSelectObject (&cur, m);
		m = LineDefs[n].end;
		if (cur && m >= 0)
			UnSelectObject (&cur, m);
		continue;
	}
	/* check if there are any Vertices left */
	if (cur
			&& (Expert
				|| Confirm (-1, -1, "Some vertices are not bound to any linedef",
					"Do you want to delete these unused Vertices?")))
	{
		DeleteObjects (OBJ_VERTICES, &cur);

	}
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	/* select all SideDefs */
	for (n = 0 ; n < NumSideDefs ; n++)
		SelectObject (&cur, n);
	/* unselect SideDefs bound to a LineDef */
	for (n = 0 ; n < NumLineDefs ; n++)
	{
		m = LineDefs[n].side_R;
		if (cur && m >= 0)
			UnSelectObject (&cur, m);
		m = LineDefs[n].side_L;
		if (cur && m >= 0)
			UnSelectObject (&cur, m);
		continue;
	}
	/* check if there are any SideDefs left */
	if (cur
			&& (Expert
				|| Confirm (-1, -1, "Some sidedefs are not bound to any linedef",
					"Do you want to delete these unused sidedefs?")))
		DeleteObjects (OBJ_SIDEDEFS, &cur);
	else
		{ delete cur; cur = NULL; }

	CheckingObjects ();
	/* select all Sectors */
	for (n = 0 ; n < NumSectors ; n++)
		SelectObject (&cur, n);
	/* unselect Sectors bound to a sidedef */
	for (n = 0 ; n < NumLineDefs ; n++)
	{

		m = LineDefs[n].side_L;

		if (cur && m >= 0 /* && SideDefs[m].sector >= 0 AYM 1998-06-13 */)
			UnSelectObject (&cur, SideDefs[m].sector);

		m = LineDefs[n].side_R;

		if (cur && m >= 0 /* && SideDefs[m].sector >= 0 AYM 1998-06-13 */)
			UnSelectObject (&cur, SideDefs[m].sector);
		continue;
	}
	/* check if there are any Sectors left */
	if (cur
			&& (Expert
				|| Confirm (-1, -1, "Some sectors are not bound to any sidedef",
					"Do you want to delete these unused sectors?")))
		DeleteObjects (OBJ_SECTORS, &cur);
	else
		{ delete cur; cur = NULL; }
#endif
}


/*
   check for missing textures
   */

void CheckTextures ()
{
#if 0  // TODO
	int  n;
	int  sd1, sd2;
	int  s1, s2;
	char msg1[80], msg2[80];
	bool first_time = true;

	CheckingObjects ();
	LogPrintf("\nVerifying textures...\n");

	for (n = 0 ; n < NumSectors ; n++)
	{
		if (strcmp (Sectors[n].ceil_tex, "-") == 0
				|| strcmp (Sectors[n].ceil_tex, "") == 0
				|| memcmp (Sectors[n].ceil_tex, "        ", 8) == 0)
		{
			sprintf (msg1, "Error: sector #%d has no ceiling texture", n);
			sprintf (msg2, "You probably used a brain-damaged editor to do that...");
			CheckFailed (-1, -1, msg1, msg2, 1, first_time);
			GoToObject (Objid (OBJ_SECTORS, n));
			return;
		}
		if (strcmp (Sectors[n].floor_tex, "-") == 0
				|| strcmp (Sectors[n].floor_tex, "") == 0
				|| memcmp (Sectors[n].floor_tex, "        ", 8) == 0)
		{
			sprintf (msg1, "Error: sector #%d has no floor texture", n);
			sprintf (msg2, "You probably used a brain-damaged editor to do that...");
			CheckFailed (-1, -1, msg1, msg2, 1, first_time);
			GoToObject (Objid (OBJ_SECTORS, n));
			return;
		}
		if (Sectors[n].ceilh < Sectors[n].floorh)
		{
			sprintf (msg1,
					"Error: sector #%d has its ceiling lower than its floor", n);
			sprintf (msg2,
					"The textures will never be displayed if you cannot go there");
			CheckFailed (-1, -1, msg1, msg2, 1, first_time);
			GoToObject (Objid (OBJ_SECTORS, n));
			return;
		}
#if 0  /* AYM 2000-08-13 */
		if (Sectors[n].ceilh - Sectors[n].floorh > 1023)
		{
			sprintf (msg1, "Error: sector #%d has its ceiling too high", n);
			sprintf (msg2, "The maximum difference allowed is 1023 (ceiling - floor)");
			CheckFailed (-1, -1, msg1, msg2, 1, first_time);
			GoToObject (Objid (OBJ_SECTORS, n));
			return;
		}
#endif
	}

	for (n = 0 ; n < NumLineDefs ; n++)
	{

		sd1 = LineDefs[n].side_R;
		sd2 = LineDefs[n].side_L;

		if (sd1 >= 0)
			s1 = SideDefs[sd1].sector;
		else
			s1 = OBJ_NO_NONE;
		if (sd2 >= 0)
			s2 = SideDefs[sd2].sector;
		else
			s2 = OBJ_NO_NONE;
		if (is_obj (s1) && ! is_obj (s2))
		{
			if (SideDefs[sd1].mid_tex[0] == '-' && SideDefs[sd1].mid_tex[1] == '\0')
			{
				sprintf (msg1, "Error in one-sided linedef #%d:"
						" sidedef #%d has no middle texture", n, sd1);
				sprintf (msg2, "Do you want to set the texture to \"%s\""
						" and continue?", default_middle_texture);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
				strncpy (SideDefs[sd1].mid_tex, default_middle_texture, WAD_TEX_NAME);
				MarkChanges();
				CheckingObjects ();
			}
		}
		if (is_obj (s1) && is_obj (s2) && Sectors[s1].ceilh > Sectors[s2].ceilh)
		{
			if (SideDefs[sd1].upper_tex[0] == '-' && SideDefs[sd1].upper_tex[1] == '\0'
					&& (! is_sky (Sectors[s1].ceil_tex) || ! is_sky (Sectors[s2].ceil_tex)))
			{
				sprintf (msg1, "Error in first sidedef of linedef #%d:"
						" sidedef #%d has no upper texture", n, sd1);
				sprintf (msg2, "Do you want to set the texture to \"%s\""
						" and continue?", default_upper_texture);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
				strncpy (SideDefs[sd1].upper_tex, default_upper_texture, WAD_TEX_NAME);
				MarkChanges();
				CheckingObjects ();
			}
		}
		if (is_obj (s1) && is_obj (s2) && Sectors[s1].floorh < Sectors[s2].floorh)
		{
			if (SideDefs[sd1].lower_tex[0] == '-' && SideDefs[sd1].lower_tex[1] == '\0')
			{
				sprintf (msg1, "Error in first sidedef of linedef #%d:"
						" sidedef #%d has no lower texture", n, sd1);
				sprintf (msg2, "Do you want to set the texture to \"%s\""
						" and continue?", default_lower_texture);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
				strncpy (SideDefs[sd1].lower_tex, default_lower_texture, WAD_TEX_NAME);
				MarkChanges();
				CheckingObjects ();
			}
		}
		if (is_obj (s1) && is_obj (s2) && Sectors[s2].ceilh > Sectors[s1].ceilh)
		{
			if (SideDefs[sd2].upper_tex[0] == '-' && SideDefs[sd2].upper_tex[1] == '\0'
					&& (! is_sky (Sectors[s1].ceil_tex) || ! is_sky (Sectors[s2].ceil_tex)))
			{
				sprintf (msg1, "Error in second sidedef of linedef #%d:"
						" sidedef #%d has no upper texture", n, sd2);
				sprintf (msg2, "Do you want to set the texture to \"%s\""
						" and continue?", default_upper_texture);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
				strncpy (SideDefs[sd2].upper_tex, default_upper_texture, WAD_TEX_NAME);
				MarkChanges();
				CheckingObjects ();
			}
		}
		if (is_obj (s1) && is_obj (s2) && Sectors[s2].floorh < Sectors[s1].floorh)
		{
			if (SideDefs[sd2].lower_tex[0] == '-' && SideDefs[sd2].lower_tex[1] == '\0')
			{
				sprintf (msg1, "Error in second sidedef of linedef #%d:"
						" sidedef #%d has no lower texture", n, sd2);
				sprintf (msg2, "Do you want to set the texture to \"%s\""
						" and continue?", default_lower_texture);
				if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
				{
					GoToObject (Objid (OBJ_LINEDEFS, n));
					return;
				}
				strncpy (SideDefs[sd2].lower_tex, default_lower_texture, WAD_TEX_NAME);
				MarkChanges();
				CheckingObjects ();
			}
		}
	}
#endif
}


/*
   check if a texture name matches one of the elements of a list
   */

bool IsTextureNameInList (char *name, char **list, int numelems)
{
	int n;

	for (n = 0 ; n < numelems ; n++)
		if (! y_strnicmp (name, list[n], WAD_TEX_NAME))
			return true;
	return false;
}


/*
   check for invalid texture names
   */

void CheckTextureNames ()
{
#if 0  // TODO
	int  n;
	char msg1[80], msg2[80];
	bool first_time = true;

	CheckingObjects ();
	LogPrintf("\nVerifying texture names...\n");

	// AYM 2000-07-24: could someone explain this one ?
	if (! FindMasterDir (MasterDir, "F2_START"))
		NumThings--;


	for (n = 0 ; n < NumSectors ; n++)
	{
		if (! is_flat_name_in_list (Sectors[n].ceil_tex))
		{
			sprintf (msg1, "Invalid ceiling texture in sector #%d", n);
			sprintf (msg2, "The name \"%.*s\" is not a floor/ceiling texture",
					(int) WAD_FLAT_NAME, Sectors[n].ceil_tex);
			if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
			{
				GoToObject (Objid (OBJ_SECTORS, n));
				return;
			}
			CheckingObjects ();
		}
		if (! is_flat_name_in_list (Sectors[n].floor_tex))
		{
			sprintf (msg1, "Invalid floor texture in sector #%d", n);
			sprintf (msg2, "The name \"%.*s\" is not a floor/ceiling texture",
					(int) WAD_FLAT_NAME, Sectors[n].floor_tex);
			if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
			{
				GoToObject (Objid (OBJ_SECTORS, n));
				return;
			}
			CheckingObjects ();
		}
	}

	for (n = 0 ; n < NumSideDefs ; n++)
	{
		if (! IsTextureNameInList (SideDefs[n].upper_tex, WTexture, NumWTexture))
		{
			sprintf (msg1, "Invalid upper texture in sidedef #%d", n);
			sprintf (msg2, "The name \"%.*s\" is not a wall texture",
					(int) WAD_TEX_NAME, SideDefs[n].upper_tex);
			if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
			{
				GoToObject (Objid (OBJ_SIDEDEFS, n));
				return;
			}
			CheckingObjects ();
		}
		if (! IsTextureNameInList (SideDefs[n].lower_tex, WTexture, NumWTexture))
		{
			sprintf (msg1, "Invalid lower texture in sidedef #%d", n);
			sprintf (msg2, "The name \"%.*s\" is not a wall texture",
					(int) WAD_TEX_NAME, SideDefs[n].lower_tex);
			if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
			{
				GoToObject (Objid (OBJ_SIDEDEFS, n));
				return;
			}
			CheckingObjects ();
		}
		if (! IsTextureNameInList (SideDefs[n].mid_tex, WTexture, NumWTexture))
		{
			sprintf (msg1, "Invalid middle texture in sidedef #%d", n);
			sprintf (msg2, "The name \"%.*s\" is not a wall texture",
					(int) WAD_TEX_NAME, SideDefs[n].mid_tex);
			if (CheckFailed (-1, -1, msg1, msg2, 0, first_time))
			{
				GoToObject (Objid (OBJ_SIDEDEFS, n));
				return;
			}
			CheckingObjects ();
		}
	}
#endif
}


/*
   check for players starting points
   */

bool CheckStartingPos ()
{
	char msg1[80], msg2[80];
	bool p1 = false;
	bool p2 = false;
	bool p3 = false;
	bool p4 = false;
	int dm = 0;
	int  t;

	for (t = 0 ; t < NumThings ; t++)
	{
		if (Things[t]->type == THING_PLAYER1)
			p1 = true;
		if (Things[t]->type == THING_PLAYER2)
			p2 = true;
		if (Things[t]->type == THING_PLAYER3)
			p3 = true;
		if (Things[t]->type == THING_PLAYER4)
			p4 = true;
		if (Things[t]->type == THING_DEATHMATCH)
			dm++;
	}
	if (! p1)
	{
		//Beep ();
		if (! Confirm (-1, -1, "Warning: there is no player 1 starting point. The"
					" game", "will crash if you play with this level. Save anyway ?"))
			return false;
		else 
			return true;  // No point in doing further checking !
	}
	if (1) ////### Expert)
		return true;
	if (! p2 || ! p3 || ! p4)
	{
		if (! p4)
			t = 4;
		if (! p3)
			t = 3;
		if (! p2)
			t = 2;
		sprintf (msg1, "Warning: there is no player %d start."
				" You will not be able", t);
		sprintf (msg2, "to use this level for multi-player games."
				" Save anyway ?");
		if (! Confirm (-1, -1, msg1, msg2))
			return false;
		else
			return true;  // No point in doing further checking !
	}
	if (dm < DOOM_MIN_DEATHMATCH_STARTS)
	{
		if (dm == 0)
			sprintf (msg1, "Warning: there are no deathmatch starts."
					" You need at least %d", DOOM_MIN_DEATHMATCH_STARTS);
		else if (dm == 1)
			sprintf (msg1, "Warning: there is only one deathmatch start."
					" You need at least %d", DOOM_MIN_DEATHMATCH_STARTS);
		else
			sprintf (msg1, "Warning: there are only %d deathmatch starts."
					" You need at least %d", dm, DOOM_MIN_DEATHMATCH_STARTS);
		sprintf (msg2, "deathmatch starts to play deathmatch games."
				" Save anyway ?");
		if (! Confirm (-1, -1, msg1, msg2))
			return false;
	}
	return true;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
