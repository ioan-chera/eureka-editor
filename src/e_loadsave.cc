//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE / NEW
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

#include "e_basis.h"
#include "e_loadsave.h"
#include "levels.h"  // CalculateLevelBounds()
#include "lib_adler.h"
#include "m_config.h"
#include "m_dialog.h"
#include "m_files.h"
#include "w_rawdef.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"


static void ChecksumThing(crc32_c& crc, const Thing * T)
{
	crc += T->x;
	crc += T->y;
	crc += T->angle;
	crc += T->type;
	crc += T->options;
}

static void ChecksumVertex(crc32_c& crc, const Vertex * V)
{
	crc += V->x;
	crc += V->y;
}

static void ChecksumSector(crc32_c& crc, const Sector * SEC)
{
	crc += SEC->floorh;
	crc += SEC->ceilh;
	crc += SEC->light;
	crc += SEC->type;
	crc += SEC->tag;

	crc += SEC->FloorTex();
	crc += SEC->CeilTex();
}

static void ChecksumSideDef(crc32_c& crc, const SideDef * S)
{
	crc += S->x_offset;
	crc += S->y_offset;

	crc += S->LowerTex();
	crc += S->MidTex();
	crc += S->UpperTex();

	ChecksumSector(crc, S->SecRef());
}

static void ChecksumLineDef(crc32_c& crc, const LineDef * L)
{
	crc += L->flags;
	crc += L->type;
	crc += L->tag;

	ChecksumVertex(crc, L->Start());
	ChecksumVertex(crc, L->End());

	if (L->Right())
		ChecksumSideDef(crc, L->Right());
	else
		L += L->right;

	if (L->Left())
		ChecksumSideDef(crc, L->Left());
	else
		L += L->left;
}


void BA_LevelChecksum(crc32_c& crc)
{
	// the following method conveniently skips any unused vertices,
	// sidedefs and sectors.  It also adds each sector umpteen times
	// (for each line in the sector), but that should not affect the
	// validity of the final checksum.

	int i;

	for (i = 0 ; i < NumThings ; i++)
		ChecksumThing(crc, Things[i]);

	for (i = 0 ; i < NumLineDefs ; i++)
		ChecksumLineDef(crc, LineDefs[i]);
}


//------------------------------------------------------------------------

static void FreshLevel()
{
	BA_ClearAll();

	Sector *sec = new Sector;
	Sectors.push_back(sec);

	sec->SetDefaults();

	Thing *th = new Thing;
	Things.push_back(th);

	th->x = 512;
	th->y = 512;
	th->type = 1;

	for (int i = 0 ; i < 4 ; i++)
	{
		Vertex *v = new Vertex;
		Vertices.push_back(v);

		v->x = (i >= 2) ? 768 : 256;
		v->y = (i==1 || i==2) ? 768 : 256;

		SideDef *sd = new SideDef;
		SideDefs.push_back(sd);

		sd->SetDefaults(false);

		LineDef *ld = new LineDef;
		LineDefs.push_back(ld);

		ld->start = i;
		ld->end   = (i+1) % 4;
		ld->flags = MLF_Blocking;
		ld->right = i;
	}

	CalculateLevelBounds();
}


extern void CMD_ZoomWholeMap();


void CMD_NewMap()
{
	if (! Main_ConfirmQuit("begin a new map"))
		return;

	// if there is a current PWAD, the new map will go there
	// (and need to determine where).  Otherwise no need to ask.

	Replacer = false;

	if (edit_wad)
	{
		UI_ChooseMap * dialog = new UI_ChooseMap(Level_name);

		dialog->PopulateButtons(toupper(Level_name[0]), edit_wad);

		const char *map_name = dialog->Run();

		delete dialog;

		// cancelled?
		if (! map_name)
			return;

		// would this replace an existing map?
		if (edit_wad && edit_wad->FindLevel(map_name) >= 0)
		{
			Replacer = true;
		}

		Level_name = StringUpper(map_name);

		main_win->SetTitle(Pwad_name, Level_name);
	}
	else
	{
		main_win->SetTitle(NULL, Level_name);
	}

	LogPrintf("Created NEW map : %s\n", Level_name);

	FreshLevel();

	CMD_ZoomWholeMap();

	MadeChanges = 0;
}


//------------------------------------------------------------------------
//  LOADING CODE
//------------------------------------------------------------------------

static Wad_file * load_wad;

static short loading_level;


static void UpperCaseShortStr(char *buf, int max_len)
{
	for (int i = 0 ; (i < max_len) && buf[i] ; i++)
	{
		buf[i] = toupper(buf[i]);
	}
}


static void LoadVertices()
{
	Lump_c *lump = load_wad->FindLumpInLevel("VERTEXES", loading_level);
	if (! lump)
		FatalError("No vertex lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to vertex lump!\n");

	int count = lump->Length() / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	PrintDebug("GetVertices: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Vertices\n");

	Vertices.reserve(count);

	for (int i = 0 ; i < count ; i++)
	{
		raw_vertex_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading vertices.\n");

		Vertex *vert = new Vertex;

		vert->x = LE_S16(raw.x);
		vert->y = LE_S16(raw.y);

		Vertices.push_back(vert);
	}
}


static void LoadSectors()
{
	Lump_c *lump = load_wad->FindLumpInLevel("SECTORS", loading_level);
	if (! lump)
		FatalError("No sector lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to sector lump!\n");

	int count = lump->Length() / sizeof(raw_sector_t);

# if DEBUG_LOAD
	PrintDebug("GetSectors: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Sectors\n");

	Sectors.reserve(count);

	for (int i = 0 ; i < count ; i++)
	{
		raw_sector_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sectors.\n");

		Sector *sec = new Sector;

		sec->floorh = LE_S16(raw.floorh);
		sec->ceilh  = LE_S16(raw.ceilh);

		UpperCaseShortStr(raw.floor_tex, 8);
		UpperCaseShortStr(raw. ceil_tex, 8);

		sec->floor_tex = BA_InternaliseShortStr(raw.floor_tex, 8);
		sec->ceil_tex  = BA_InternaliseShortStr(raw.ceil_tex,  8);

		sec->light = LE_U16(raw.light);
		sec->type  = LE_U16(raw.type);
		sec->tag   = LE_S16(raw.tag);

		Sectors.push_back(sec);
	}
}


static void LoadThings()
{
	Lump_c *lump = load_wad->FindLumpInLevel("THINGS", loading_level);
	if (! lump)
		FatalError("No things lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to things lump!\n");

	int count = lump->Length() / sizeof(raw_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif

	if (count == 0)
	{
		// Note: no error if no things exist, even though technically a map
		// will be unplayable without the player starts.
//!!!!		PrintWarn("Couldn't find any Things!\n");
		return;
	}

	for (int i = 0 ; i < count ; i++)
	{
		raw_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading things.\n");

		Thing *th = new Thing;

		th->x = LE_S16(raw.x);
		th->y = LE_S16(raw.y);

		th->angle   = LE_U16(raw.angle);
		th->type    = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		Things.push_back(th);
	}
}


static void LoadSideDefs()
{
	Lump_c *lump = load_wad->FindLumpInLevel("SIDEDEFS", loading_level);
	if (! lump)
		FatalError("No sidedefs lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to sidedefs lump!\n");

	int count = lump->Length() / sizeof(raw_sidedef_t);

# if DEBUG_LOAD
	PrintDebug("GetSidedefs: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Sidedefs\n");

	for (int i = 0 ; i < count ; i++)
	{
		raw_sidedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading sidedefs.\n");

		SideDef *sd = new SideDef;

		sd->x_offset = LE_S16(raw.x_offset);
		sd->y_offset = LE_S16(raw.y_offset);

		UpperCaseShortStr(raw.upper_tex, 8);
		UpperCaseShortStr(raw.lower_tex, 8);
		UpperCaseShortStr(raw.  mid_tex, 8);

		sd->upper_tex = BA_InternaliseShortStr(raw.upper_tex, 8);
		sd->lower_tex = BA_InternaliseShortStr(raw.lower_tex, 8);
		sd->  mid_tex = BA_InternaliseShortStr(raw.  mid_tex, 8);

		sd->sector = LE_U16(raw.sector);

//!!!!!!		if (sd->sector >= NumSectors)  // FIXME warning
//!!!!!!			sd->sector = 0;

		SideDefs.push_back(sd);
	}
}


static void LoadLineDefs()
{
	Lump_c *lump = load_wad->FindLumpInLevel("LINEDEFS", loading_level);
	if (! lump)
		FatalError("No linedefs lump!\n");

	if (! lump->Seek())
		FatalError("Error seeking to linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		FatalError("Couldn't find any Linedefs\n");

	for (int i = 0 ; i < count ; i++)
	{
		raw_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			FatalError("Error reading linedefs.\n");

		LineDef *ld = new LineDef;

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type  = LE_U16(raw.type);
		ld->tag   = LE_S16(raw.tag);

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		// FIXME vertex check

		if (ld->right == 0xFFFF)
			ld->right = -1;
//!!!!!!		else if (ld->right >= NumSideDefs)
//!!!!!!			ld->right = 0;  // FIXME warning

		if (ld->left == 0xFFFF)
			ld->left = -1;
//!!!!!!		else if (ld->left >= NumSideDefs)
//!!!!!!			ld->left = -1;  // FIXME warning

		LineDefs.push_back(ld);
	}
}


static void RemoveUnusedVertices()
{
	if (NumVertices == 0)
		return;

	bitvec_c *used_verts = new bitvec_c(NumVertices);

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		used_verts->set(LineDefs[i]->start);
		used_verts->set(LineDefs[i]->end);
	}

	int new_count = NumVertices;

	while (new_count > 2 && !used_verts->get(new_count-1))
		new_count--;

	// we directly modify the vertex array here (which is not
	// normally kosher, but level loading is a special case).
	if (new_count < NumVertices)
	{
		LogPrintf("Removing %d unused vertices at end\n", new_count);

		for (int i = new_count ; i < NumVertices ; i++)
			delete Vertices[i];

		Vertices.resize(new_count);
	}

	delete used_verts;
}


/*
   read in the level data
*/

void LoadLevel(Wad_file *wad, const char *level)
{
	load_wad = wad;

	loading_level = load_wad->FindLevel(level);
	if (loading_level < 0)
		FatalError("No such map: %s\n", level);

	BA_ClearAll();

	LoadThings  ();
	LoadVertices();
	LoadSectors ();
	LoadSideDefs();
	LoadLineDefs();

	// FIXME !!!!  check all references 



	// Node builders create a lot of new vertices for segs.
	// However they just get in the way for editing, so remove them.
	RemoveUnusedVertices();

	CalculateLevelBounds();

	Level_name = StringUpper(level);

	Status_Set("Loaded %s", Level_name);

	if (main_win)
	{
		main_win->SetTitle(wad->PathName(), level);

		// load the user state associated with this map
		crc32_c adler_crc;

		BA_LevelChecksum(adler_crc);

		if (! M_LoadUserState())
		{
			M_DefaultUserState();
		}
	}

    edit.RedrawMap = 1;

	MadeChanges = 0;
}


void RemoveEditWad()
{
	if (! edit_wad)
		return;

	MasterDir_Remove(edit_wad);
	delete edit_wad;

	edit_wad = NULL;
	Pwad_name = NULL;

	Replacer = false;
}


bool CMD_OpenMap()
{
	if (! Main_ConfirmQuit("open another map"))
		return false;


	Wad_file *wad = NULL;
	const char *map_name = NULL;
	bool is_new_pwad = false;

	UI_OpenMap * dialog = new UI_OpenMap();

	dialog->Run(&wad, &is_new_pwad, &map_name);

	delete dialog;


	// cancelled?
	if (! wad)
		return false;


	// this shouldn't happen -- but just in case...
	if (wad->FindLevel(map_name) < 0)
	{
		Notify(-1, -1, "Hmmmm, cannot find that map !?!", NULL);
		return false;
	}


	if (is_new_pwad && wad->FindLump(EUREKA_LUMP))
	{
		if (! M_ParseEurekaLump(wad))
			return false;

		Main_LoadResources();
	}


	// has this removed or replaced the currently edited wad?

	if (edit_wad && (wad != edit_wad))
	{
		RemoveEditWad();
	}

	if (is_new_pwad)
	{
		edit_wad = wad;
		Pwad_name = edit_wad->PathName();

		MasterDir_Add(edit_wad);
	}


	LogPrintf("Loading Map : %s of %s\n", map_name, wad->PathName());

	LoadLevel(wad, map_name);

	Replacer = false;

	return true;
}


void CMD_OpenRecentMap()
{
	const char *filename = NULL;
	const char *map_name = NULL;

	M_RecentDialog(&filename, &map_name);

	// cancelled?
	if (! filename)
		return;

	if (! Main_ConfirmQuit("open another map"))
		return;

	Wad_file *wad = NULL;

	// make sure the file exists [Open with 'a' would create it]
	if (FileExists(filename))
	{
		wad = Wad_file::Open(filename, 'a');
	}

	if (! wad)
	{
		// FIXME: get an error message, add it here

		Notify(-1, -1, "Unable to open that WAD file.\n\n"
		               "(perhaps it was renamed or deleted?)", NULL);
		return;
	}

	if (wad->FindLevel(map_name) < 0)
	{
		delete wad;

		Notify(-1, -1, "Unable to find that map in that WAD.", NULL);
		return;
	}


	if (wad->FindLump(EUREKA_LUMP))
	{
		if (! M_ParseEurekaLump(wad))
			return;

		Main_LoadResources();
	}


	// this wad replaces the current PWAD

	RemoveEditWad();

	edit_wad = wad;
	Pwad_name = edit_wad->PathName();

	MasterDir_Add(edit_wad);


	LogPrintf("Loading Map : %s of %s\n", map_name, wad->PathName());

	LoadLevel(wad, map_name);

	Replacer = false;
}


//------------------------------------------------------------------------
//  SAVING CODE
//------------------------------------------------------------------------

static Wad_file *save_wad;


static void SaveVertices()
{
	int size = NumVertices * (int)sizeof(raw_vertex_t);

	Lump_c *lump = save_wad->AddLump("VERTEXES", size);

	for (int i = 0 ; i < NumVertices ; i++)
	{
		Vertex *vert = Vertices[i];

		raw_vertex_t raw;

		raw.x = LE_S16(vert->x);
		raw.y = LE_S16(vert->y);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSectors()
{
	int size = NumSectors * (int)sizeof(raw_sector_t);

	Lump_c *lump = save_wad->AddLump("SECTORS", size);

	for (int i = 0 ; i < NumSectors ; i++)
	{
		Sector *sec = Sectors[i];

		raw_sector_t raw;
		
		raw.floorh = LE_S16(sec->floorh);
		raw.ceilh  = LE_S16(sec->ceilh);

		strncpy(raw.floor_tex, sec->FloorTex(), sizeof(raw.floor_tex));
		strncpy(raw.ceil_tex,  sec->CeilTex(),  sizeof(raw.ceil_tex));

		raw.light = LE_U16(sec->light);
		raw.type  = LE_U16(sec->type);
		raw.tag   = LE_U16(sec->tag);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveThings()
{
	int size = NumThings * (int)sizeof(raw_thing_t);

	Lump_c *lump = save_wad->AddLump("THINGS", size);

	for (int i = 0 ; i < NumThings ; i++)
	{
		Thing *th = Things[i];

		raw_thing_t raw;
		
		raw.x = LE_S16(th->x);
		raw.y = LE_S16(th->y);

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSideDefs()
{
	int size = NumSideDefs * (int)sizeof(raw_sidedef_t);

	Lump_c *lump = save_wad->AddLump("SIDEDEFS", size);

	for (int i = 0 ; i < NumSideDefs ; i++)
	{
		SideDef *side = SideDefs[i];

		raw_sidedef_t raw;
		
		raw.x_offset = LE_S16(side->x_offset);
		raw.y_offset = LE_S16(side->y_offset);

		strncpy(raw.upper_tex, side->UpperTex(), sizeof(raw.upper_tex));
		strncpy(raw.lower_tex, side->LowerTex(), sizeof(raw.lower_tex));
		strncpy(raw.mid_tex,   side->MidTex(),   sizeof(raw.mid_tex));

		raw.sector = LE_U16(side->sector);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveLineDefs()
{
	int size = NumLineDefs * (int)sizeof(raw_linedef_t);

	Lump_c *lump = save_wad->AddLump("LINEDEFS", size);

	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		LineDef *ld = LineDefs[i];

		raw_linedef_t raw;
		
		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = LE_U16(ld->type);
		raw.tag   = LE_S16(ld->tag);

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void EmptyLump(const char *name)
{
	save_wad->AddLump(name)->Finish();
}


static void SaveLevel(Wad_file *wad, const char *level)
{
	save_wad = wad;

	save_wad->BeginWrite();

	// remove previous version of level (if it exists)
	int level_lump = save_wad->FindLevel(level);
	if (level_lump >= 0)
		save_wad->RemoveLevel(level_lump);

	save_wad->InsertPoint(level_lump);

	save_wad->AddLevel(level, 0)->Finish();

	SaveThings  ();
	SaveLineDefs();
	SaveSideDefs();
	SaveVertices();

	EmptyLump("SEGS");
	EmptyLump("SSECTORS");
	EmptyLump("NODES");

	SaveSectors();

	EmptyLump("REJECT");
	EmptyLump("BLOCKMAP");

	M_WriteEurekaLump(save_wad);

	// write out the new directory
	save_wad->EndWrite();

	Level_name = StringUpper(level);

	Status_Set("Saved %s", Level_name);

	if (main_win)
	{
		main_win->SetTitle(wad->PathName(), level);

		// save the user state associated with this map
		M_SaveUserState();
	}
}


void CMD_SaveMap()
{
	// we require a wad file to save into.
	// if there is none, then need to create one via Export function.

	if (! edit_wad)
	{
		CMD_ExportMap();
		return;
	}

	// warn user if a fresh map would destroy an existing one

	if (Replacer)
	{
		// TODO: ideally have "Export" as an option

		if (! Confirm(-1, -1, "The current PWAD already contains this map.\n"
		                      "This operation will destroy that map (overwrite it).\n"
							  "\n"
		                      "Are you sure you want to continue?", NULL))
		{
			return;
		}
	}

	M_BackupWad(edit_wad);

	LogPrintf("Saving Map : %s of %s\n", Level_name, edit_wad->PathName());

	SaveLevel(edit_wad, Level_name);

	M_AddRecent(edit_wad->PathName(), Level_name);

	Replacer = false;
	MadeChanges = 0;
}


void CMD_ExportMap()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to export to");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("Wads\t*.wad");

	//??  chooser.directory("xxx");

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			LogPrintf("Export Map: error choosing file:\n");
			LogPrintf("   %s\n", chooser.errmsg());

			Notify(-1, -1, "Unable to export the map:",
			       chooser.errmsg());
			return;

		case 1:
			LogPrintf("Export Map: cancelled by user\n");
			return;

		default:
			break;  // OK
	}

	/// if extension is missing then add ".wad"
	char filename[FL_PATH_MAX];

	strcpy(filename, chooser.filename());

	char *pos = (char *)fl_filename_ext(filename);
	if (! *pos)
		strcat(filename, ".wad");


	// does the file already exist?  if not, create it...
	bool exists = FileExists(filename);

	Wad_file *wad;

	if (exists)
	{
		wad = Wad_file::Open(filename, 'a');
	}
	else
	{
		wad = Wad_file::Open(filename, 'w');
	}

	if (! wad)
	{
		Notify(-1, -1, "Unable to export the map:", "Error creating output file");
		return;
	}


	// ask user for map name

	UI_ChooseMap * dialog = new UI_ChooseMap(Level_name);

	dialog->PopulateButtons(toupper(Level_name[0]), wad);

	const char *map_name = dialog->Run();

	delete dialog;


	// cancelled?
	if (! map_name)
	{
		delete wad;
		return;
	}


	// we will write into the chosen wad.
	// however if the level already exists, get confirmation first

	if (exists && wad->FindLevel(map_name) >= 0)
	{
		if (! Confirm(-1, -1, "The selected WAD already contains this map.\n"
		                      "This operation will destroy that map (overwrite it).\n"
							  "\n"
		                      "Are you sure you want to continue?", NULL))
		{
			delete wad;
			return;
		}
	}

	// back-up an existing wad
	if (exists)
	{
		M_BackupWad(wad);
	}


	LogPrintf("Exporting Map : %s of %s\n", map_name, wad->PathName());

	SaveLevel(wad, map_name);

	M_AddRecent(wad->PathName(), map_name);


	// the new wad replaces the current PWAD

	if (edit_wad)
	{
		MasterDir_Remove(edit_wad);

		delete edit_wad;
	}

	edit_wad = wad;
	Pwad_name = edit_wad->PathName();

	MasterDir_Add(edit_wad);

	Replacer = false;
	MadeChanges = 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
