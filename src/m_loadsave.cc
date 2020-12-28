//------------------------------------------------------------------------
//  LEVEL LOAD / SAVE / NEW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C)      2015 Ioan Chera
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include "lib_adler.h"

#include "e_basis.h"
#include "e_checks.h"
#include "e_main.h"  // CalculateLevelBounds()
#include "m_config.h"
#include "m_files.h"
#include "m_loadsave.h"
#include "m_nodes.h"
#include "m_udmf.h"
#include "r_subdiv.h"
#include "w_rawdef.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"


int last_given_file;


// this is only used to prevent a M_SaveMap which happens inside
// CMD_BuildAllNodes from building that saved level twice.
bool inhibit_node_build;


static void SaveLevel(Instance &inst, const SString &level);

static const char * overwrite_message =
	"The %s PWAD already contains this map.  "
	"This operation will destroy that map (overwrite it)."
	"\n\n"
	"Are you sure you want to continue?";


void Instance::RemoveEditWad()
{
	if (!edit_wad)
		return;

	MasterDir_Remove(edit_wad);
	delete edit_wad;

	edit_wad  = NULL;
	Pwad_name.clear();
}


static void ReplaceEditWad(Instance &inst, Wad_file *new_wad)
{
	inst.RemoveEditWad();

	inst.edit_wad = new_wad;

	Pwad_name = inst.edit_wad->PathName();

	inst.MasterDir_Add(inst.edit_wad);
}


static void FreshLevel(Instance &inst)
{
	inst.level.basis.clearAll();

	Sector *sec = new Sector;
	inst.level.sectors.push_back(sec);

	sec->SetDefaults(inst);

	for (int i = 0 ; i < 4 ; i++)
	{
		Vertex *v = new Vertex;
		inst.level.vertices.push_back(v);

		v->SetRawX(inst, (i >= 2) ? 256 : -256);
		v->SetRawY(inst, (i==1 || i==2) ? 256 :-256);

		SideDef *sd = new SideDef;
		inst.level.sidedefs.push_back(sd);

		sd->SetDefaults(inst, false);

		LineDef *ld = new LineDef;
		inst.level.linedefs.push_back(ld);

		ld->start = i;
		ld->end   = (i+1) % 4;
		ld->flags = MLF_Blocking;
		ld->right = i;
	}

	for (int pl = 1 ; pl <= 4 ; pl++)
	{
		Thing *th = new Thing;
		inst.level.things.push_back(th);

		th->type  = pl;
		th->angle = 90;

		th->SetRawX(inst, (pl == 1) ? 0 : (pl - 3) * 48);
		th->SetRawY(inst, (pl == 1) ? 48 : (pl == 3) ? -48 : 0);
	}

	CalculateLevelBounds(inst);

	ZoomWholeMap(inst);

	inst.Editor_DefaultState();
}


static bool Project_AskFile(SString &filename)
{
	// this returns false if user cancelled

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to create");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder().c_str());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			LogPrintf("New Project: error choosing file:\n");
			LogPrintf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to create a new project:\n\n%s", chooser.errmsg());
			return false;

		case 1:
			LogPrintf("New Project: cancelled by user\n");
			return false;

		default:
			break;  // OK
	}

	// if extension is missing, add ".wad"
	filename = chooser.filename();

	const char *pos = fl_filename_ext(filename.c_str());
	if(!*pos)
		filename += ".wad";

	return true;
}


static void Project_ApplyChanges(Instance &inst, UI_ProjectSetup *dialog)
{
	// grab the new information

	inst.Game_name = dialog->game;
	inst.Port_name = dialog->port;

	SYS_ASSERT(!inst.Game_name.empty());

	inst.Iwad_name = M_QueryKnownIWAD(inst.Game_name);
	SYS_ASSERT(!inst.Iwad_name.empty());

	inst.Level_format = dialog->map_format;
	inst.Udmf_namespace = dialog->name_space;

	SYS_ASSERT(inst.Level_format != MapFormat::invalid);

	inst.Resource_list.clear();

	for (int i = 0 ; i < UI_ProjectSetup::RES_NUM ; i++)
	{
		if (!dialog->res[i].empty())
			inst.Resource_list.push_back(dialog->res[i]);
	}

	Fl::wait(0.1);

	inst.Main_LoadResources();

	Fl::wait(0.1);
}


void CMD_ManageProject(Instance &inst)
{
	UI_ProjectSetup * dialog = new UI_ProjectSetup(inst, false /* new_project */, false /* is_startup */);

	bool ok = dialog->Run();

	if (ok)
	{
		Project_ApplyChanges(inst, dialog);
	}

	delete dialog;
}


void CMD_NewProject(Instance &inst)
{
	if (! Main_ConfirmQuit("create a new project"))
		return;


	/* first, ask for the output file */

	SString filename;

	if (! Project_AskFile(filename))
		return;


	/* second, query what Game, Port and Resources to use */
	// TODO: new instance
	UI_ProjectSetup * dialog = new UI_ProjectSetup(gInstance, true /* new_project */, false /* is_startup */);

	bool ok = dialog->Run();

	if (! ok)
	{
		delete dialog;
		return;
	}


	/* third, delete file if it already exists
	   [ the file chooser should have asked for confirmation ]
	 */

	if (FileExists(filename))
	{
		// TODO??  M_BackupWad(wad);

		if (! FileDelete(filename))
		{
			DLG_Notify("Unable to delete the existing file.");

			delete dialog;
			return;
		}

		Fl::wait(0.1);
		Fl::wait(0.1);
	}


	inst.RemoveEditWad();

	// this calls Main_LoadResources which resets the master directory
	Project_ApplyChanges(inst, dialog);

	delete dialog;


	// determine map name (same as first level in the IWAD)
	SString map_name = "MAP01";

	int idx = inst.game_wad->LevelFindFirst();

	if (idx >= 0)
	{
		idx = inst.game_wad->LevelHeader(idx);
		map_name = inst.game_wad->GetLump(idx)->Name();
	}

	LogPrintf("Creating New File : %s in %s\n", map_name.c_str(), filename.c_str());


	Wad_file * wad = Wad_file::Open(filename, WadOpenMode_write);

	if (! wad)
	{
		DLG_Notify("Unable to create the new WAD file.");
		return;
	}

	inst.edit_wad = wad;
	Pwad_name = inst.edit_wad->PathName();

	inst.MasterDir_Add(inst.edit_wad);

	// TODO: new instance
	FreshLevel(gInstance);

	// save it now : sets Level_name and window title
	SaveLevel(gInstance, map_name);
}


bool Instance::MissingIWAD_Dialog()
{
	UI_ProjectSetup * dialog = new UI_ProjectSetup(*this, false /* new_project */, true /* is_startup */);

	bool ok = dialog->Run();

	if (ok)
	{
		Game_name = dialog->game;
		SYS_ASSERT(!Game_name.empty());

		Iwad_name = M_QueryKnownIWAD(Game_name);
		SYS_ASSERT(!Iwad_name.empty());
	}

	delete dialog;

	return ok;
}


void CMD_FreshMap(Instance &inst)
{
	if (!inst.edit_wad)
	{
		DLG_Notify("Cannot create a fresh map unless editing a PWAD.");
		return;
	}

	if (inst.edit_wad->IsReadOnly())
	{
		DLG_Notify("Cannot create a fresh map : file is read-only.");
		return;
	}

	if (! Main_ConfirmQuit("create a fresh map"))
		return;


	UI_ChooseMap * dialog = new UI_ChooseMap(inst.Level_name.c_str());

	dialog->PopulateButtons(toupper(inst.Level_name[0]), inst.edit_wad);

	SString map_name = dialog->Run();

	delete dialog;

	// cancelled?
	if (map_name.empty())
		return;

	// would this replace an existing map?
	if (inst.edit_wad->LevelFind(map_name) >= 0)
	{
		if (DLG_Confirm("Cancel|&Overwrite",
		                overwrite_message, "current") <= 0)
		{
			return;
		}
	}


	M_BackupWad(inst.edit_wad);

	LogPrintf("Created NEW map : %s\n", map_name.c_str());

	// TODO: make this allow running another level
	FreshLevel(inst);

	// save it now : sets Level_name and window title
	SaveLevel(inst, map_name);
}


//------------------------------------------------------------------------
//  LOADING CODE
//------------------------------------------------------------------------

static Wad_file * load_wad;

// TODO ideally static, but needed by m_udmf.cc too
int loading_level;

static int bad_linedef_count;

static int bad_sector_refs;
static int bad_sidedef_refs;



static void UpperCaseShortStr(char *buf, int max_len)
{
	for (int i = 0 ; (i < max_len) && buf[i] ; i++)
	{
		buf[i] = toupper(buf[i]);
	}
}


Lump_c * Load_LookupAndSeek(const char *name)
{
	int idx = load_wad->LevelLookupLump(loading_level, name);

	if (idx < 0)
		return NULL;

	Lump_c *lump = load_wad->GetLump(idx);

	if (! lump->Seek())
	{
		LogPrintf("WARNING: failed to seek to %s lump!\n", name);
	}

	return lump;
}


static void LoadVertices(Document &doc)
{
	Lump_c *lump = Load_LookupAndSeek("VERTEXES");
	if (! lump)
		ThrowException("No vertex lump!\n");

	int count = lump->Length() / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	PrintDebug("GetVertices: num = %d\n", count);
# endif

	doc.vertices.reserve(count);

	for (int i = 0 ; i < count ; i++)
	{
		raw_vertex_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading vertices.\n");

		Vertex *vert = new Vertex;

		vert->raw_x = INT_TO_COORD(LE_S16(raw.x));
		vert->raw_y = INT_TO_COORD(LE_S16(raw.y));

		doc.vertices.push_back(vert);
	}
}


static void LoadSectors(Document &doc)
{
	Lump_c *lump = Load_LookupAndSeek("SECTORS");
	if (! lump)
		FatalError("No sector lump!\n");

	int count = lump->Length() / sizeof(raw_sector_t);

# if DEBUG_LOAD
	PrintDebug("GetSectors: num = %d\n", count);
# endif

	doc.sectors.reserve(count);

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

		sec->floor_tex = BA_InternaliseString(SString(raw.floor_tex, 8));
		sec->ceil_tex  = BA_InternaliseString(SString(raw.ceil_tex,  8));

		sec->light = LE_U16(raw.light);
		sec->type  = LE_U16(raw.type);
		sec->tag   = LE_S16(raw.tag);

		doc.sectors.push_back(sec);
	}
}


static void CreateFallbackSector(Instance &inst)
{
	LogPrintf("Creating a fallback sector.\n");

	Sector *sec = new Sector;

	sec->SetDefaults(inst);

	inst.level.sectors.push_back(sec);
}

static void CreateFallbackSideDef(Instance &inst)
{
	// we need a valid sector too!
	if (inst.level.numSectors() == 0)
		CreateFallbackSector(inst);

	LogPrintf("Creating a fallback sidedef.\n");

	SideDef *sd = new SideDef;

	sd->SetDefaults(inst, false);

	inst.level.sidedefs.push_back(sd);
}

static void CreateFallbackVertices(Document &doc)
{
	LogPrintf("Creating two fallback vertices.\n");

	Vertex *v1 = new Vertex;
	Vertex *v2 = new Vertex;

	v1->raw_x = INT_TO_COORD(-777);
	v1->raw_y = INT_TO_COORD(-777);

	v2->raw_x = INT_TO_COORD(555);
	v2->raw_y = INT_TO_COORD(555);

	doc.vertices.push_back(v1);
	doc.vertices.push_back(v2);
}


void ValidateSidedefRefs(Instance &inst, LineDef * ld, int num)
{
	if (ld->right >= inst.level.numSidedefs() || ld->left >= inst.level.numSidedefs())
	{
		LogPrintf("WARNING: linedef #%d has invalid sidedefs (%d / %d)\n",
				  num, ld->right, ld->left);

		bad_sidedef_refs++;

		// ensure we have a usable sidedef
		if (inst.level.numSidedefs() == 0)
			CreateFallbackSideDef(inst);

		if (ld->right >= inst.level.numSidedefs())
			ld->right = 0;

		if (ld->left >= inst.level.numSidedefs())
			ld->left = 0;
	}
}

void ValidateVertexRefs(Instance &inst, LineDef *ld, int num)
{
	if (ld->start >= inst.level.numVertices() || ld->end >= inst.level.numVertices() ||
	    ld->start == ld->end)
	{
		LogPrintf("WARNING: linedef #%d has invalid vertices (%d -> %d)\n",
		          num, ld->start, ld->end);

		bad_linedef_count++;

		// ensure we have a valid vertex
		if (inst.level.numVertices() < 2)
			CreateFallbackVertices(inst.level);

		ld->start = 0;
		ld->end   = inst.level.numVertices() - 1;
	}
}

void ValidateSectorRef(Instance &inst, SideDef *sd, int num)
{
	if (sd->sector >= inst.level.numSectors())
	{
		LogPrintf("WARNING: sidedef #%d has invalid sector (%d)\n",
		          num, sd->sector);

		bad_sector_refs++;

		// ensure we have a valid sector
		if (inst.level.numSectors() == 0)
			CreateFallbackSector(inst);

		sd->sector = 0;
	}
}


static void LoadHeader(Instance &inst)
{
	Lump_c *lump = load_wad->GetLump(load_wad->LevelHeader(loading_level));

	int length = lump->Length();

	if (length == 0)
		return;

	inst.level.headerData.resize(length);

	if (! lump->Seek())
		ThrowException("Error seeking to header lump!\n");

	if (! lump->Read(&inst.level.headerData[0], length))
		ThrowException("Error reading header lump.\n");
}


static void LoadBehavior(Document &doc)
{
	// IOANCH 9/2015: support Hexen maps
	Lump_c *lump = Load_LookupAndSeek("BEHAVIOR");
	if (! lump)
		ThrowException("No BEHAVIOR lump!\n");

	int length = lump->Length();

	doc.behaviorData.resize(length);

	if (length == 0)
		return;

	if (! lump->Read(&doc.behaviorData[0], length))
		ThrowException("Error reading BEHAVIOR.\n");
}


static void LoadScripts(Document &doc)
{
	// the SCRIPTS lump is usually absent
	Lump_c *lump = Load_LookupAndSeek("SCRIPTS");
	if (! lump)
		return;

	int length = lump->Length();

	doc.scriptsData.resize(length);

	if (length == 0)
		return;

	if (! lump->Read(&doc.scriptsData[0], length))
		ThrowException("Error reading SCRIPTS.\n");
}


static void LoadThings(Document &doc)
{
	Lump_c *lump = Load_LookupAndSeek("THINGS");
	if (! lump)
		ThrowException("No things lump!\n");

	int count = lump->Length() / sizeof(raw_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif

	for (int i = 0 ; i < count ; i++)
	{
		raw_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading things.\n");

		Thing *th = new Thing;

		th->raw_x = INT_TO_COORD(LE_S16(raw.x));
		th->raw_y = INT_TO_COORD(LE_S16(raw.y));

		th->angle   = LE_U16(raw.angle);
		th->type    = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		doc.things.push_back(th);
	}
}


// IOANCH 9/2015
static void LoadThings_Hexen(Document &doc)
{
	Lump_c *lump = Load_LookupAndSeek("THINGS");
	if (! lump)
		ThrowException("No things lump!\n");

	int count = lump->Length() / sizeof(raw_hexen_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif

	for (int i = 0; i < count; ++i)
	{
		raw_hexen_thing_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading things.\n");

		Thing *th = new Thing;

		th->tid = LE_S16(raw.tid);
		th->raw_x = INT_TO_COORD(LE_S16(raw.x));
		th->raw_y = INT_TO_COORD(LE_S16(raw.y));
		th->raw_h = INT_TO_COORD(LE_S16(raw.height));

		th->angle = LE_U16(raw.angle);
		th->type = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		th->special = raw.special;
		th->arg1 = raw.args[0];
		th->arg2 = raw.args[1];
		th->arg3 = raw.args[2];
		th->arg4 = raw.args[3];
		th->arg5 = raw.args[4];

		doc.things.push_back(th);
	}
}


static void LoadSideDefs(Instance &inst)
{
	Lump_c *lump = Load_LookupAndSeek("SIDEDEFS");
	if(!lump)
		ThrowException("No sidedefs lump!\n");

	int count = lump->Length() / sizeof(raw_sidedef_t);

# if DEBUG_LOAD
	PrintDebug("GetSidedefs: num = %d\n", count);
# endif

	for (int i = 0 ; i < count ; i++)
	{
		raw_sidedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading sidedefs.\n");

		SideDef *sd = new SideDef;

		sd->x_offset = LE_S16(raw.x_offset);
		sd->y_offset = LE_S16(raw.y_offset);

		UpperCaseShortStr(raw.upper_tex, 8);
		UpperCaseShortStr(raw.lower_tex, 8);
		UpperCaseShortStr(raw.  mid_tex, 8);

		sd->upper_tex = BA_InternaliseString(SString(raw.upper_tex, 8));
		sd->lower_tex = BA_InternaliseString(SString(raw.lower_tex, 8));
		sd->  mid_tex = BA_InternaliseString(SString(raw.  mid_tex, 8));

		sd->sector = LE_U16(raw.sector);

		ValidateSectorRef(inst, sd, i);

		inst.level.sidedefs.push_back(sd);
	}
}


static void LoadLineDefs(Instance &inst)
{
	Lump_c *lump = Load_LookupAndSeek("LINEDEFS");
	if (! lump)
		ThrowException("No linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		return;

	for (int i = 0 ; i < count ; i++)
	{
		raw_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading linedefs.\n");

		LineDef *ld = new LineDef;

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type  = LE_U16(raw.type);
		ld->tag   = LE_S16(raw.tag);

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		if (ld->right == 0xFFFF) ld->right = -1;
		if (ld-> left == 0xFFFF) ld-> left = -1;

		ValidateVertexRefs(inst, ld, i);
		ValidateSidedefRefs(inst, ld, i);

		inst.level.linedefs.push_back(ld);
	}
}


// IOANCH 9/2015
static void LoadLineDefs_Hexen(Instance &inst)
{
	Lump_c *lump = Load_LookupAndSeek("LINEDEFS");
	if (! lump)
		ThrowException("No linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_hexen_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		return;

	for (int i = 0 ; i < count ; i++)
	{
		raw_hexen_linedef_t raw;

		if (! lump->Read(&raw, sizeof(raw)))
			ThrowException("Error reading linedefs.\n");

		LineDef *ld = new LineDef;

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type = raw.type;
		ld->tag  = raw.args[0];
		ld->arg2 = raw.args[1];
		ld->arg3 = raw.args[2];
		ld->arg4 = raw.args[3];
		ld->arg5 = raw.args[4];

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		if (ld->right == 0xFFFF) ld->right = -1;
		if (ld-> left == 0xFFFF) ld-> left = -1;

		ValidateVertexRefs(inst, ld, i);
		ValidateSidedefRefs(inst, ld, i);

		inst.level.linedefs.push_back(ld);
	}
}


static void RemoveUnusedVerticesAtEnd(Document &doc)
{
	if (doc.numVertices() == 0)
		return;

	bitvec_c used_verts(doc.numVertices());

	for (const LineDef *linedef : doc.linedefs)
	{
		used_verts.set(linedef->start);
		used_verts.set(linedef->end);
	}

	int new_count = doc.numVertices();

	while (new_count > 2 && !used_verts.get(new_count-1))
		new_count--;

	// we directly modify the vertex array here (which is not
	// normally kosher, but level loading is a special case).
	if (new_count < doc.numVertices())
	{
		LogPrintf("Removing %d unused vertices at end\n", doc.numVertices() - new_count);

		for (int i = new_count ; i < doc.numVertices(); i++)
			delete doc.vertices[i];

		doc.vertices.resize(new_count);
	}
}


static void ShowLoadProblem()
{
	LogPrintf("Map load problems:\n");
	LogPrintf("   %d linedefs with bad vertex refs\n", bad_linedef_count);
	LogPrintf("   %d linedefs with bad sidedef refs\n", bad_sidedef_refs);
	LogPrintf("   %d sidedefs with bad sector refs\n", bad_sector_refs);

	static char message[MSG_BUF_LEN];

	if (bad_linedef_count > 0)
	{
		snprintf(message, sizeof(message),
			"Found %d linedefs with bad vertex references.\n"
			"These references have been replaced.",
			bad_linedef_count);
	}
	else
	{
		snprintf(message, sizeof(message),
			"Found %d bad sector refs, %d bad sidedef refs.\n"
			"These references have been replaced.",
			bad_sector_refs, bad_sidedef_refs);
	}

	DLG_Notify("Map validation report:\n\n%s", message);
}

//
// Read in the level data
//

void LoadLevel(Instance &inst, Wad_file *wad, const SString &level)
{
	int lev_num = wad->LevelFind(level);

	if (lev_num < 0)
		ThrowException("No such map: %s\n", level.c_str());

	LoadLevelNum(inst, wad, lev_num);

	// reset various editor state
	Editor_ClearAction(inst);
	Selection_InvalidateLast();

	inst.edit.Selected->clear_all();
	inst.edit.highlight.clear();

	inst.main_win->UpdateTotals();
	inst.main_win->UpdateGameInfo();
	inst.main_win->InvalidatePanelObj();
	inst.main_win->redraw();

	if (inst.main_win)
	{
		inst.main_win->SetTitle(wad->PathName(), level, wad->IsReadOnly());

		// load the user state associated with this map
		crc32_c adler_crc;

		inst.level.getLevelChecksum(adler_crc);

		if (! M_LoadUserState(inst))
		{
			M_DefaultUserState(inst);
		}
	}

	inst.Level_name = level.asUpper();

	inst.Status_Set("Loaded %s", inst.Level_name.c_str());

	RedrawMap(inst);
}


void LoadLevelNum(Instance &inst, Wad_file *wad, int lev_num)
{
	load_wad = wad;
	loading_level = lev_num;

	inst.Level_format = load_wad->LevelFormat(loading_level);

	inst.level.basis.clearAll();

	bad_linedef_count = 0;
	bad_sector_refs   = 0;
	bad_sidedef_refs  = 0;

	LoadHeader(inst);

	if (inst.Level_format == MapFormat::udmf)
	{
		UDMF_LoadLevel(inst);
	}
	else
	{
		if (inst.Level_format == MapFormat::hexen)
			LoadThings_Hexen(inst.level);
		else
			LoadThings(inst.level);

		LoadVertices(inst.level);
		LoadSectors(inst.level);
		LoadSideDefs(inst);

		if (inst.Level_format == MapFormat::hexen)
		{
			LoadLineDefs_Hexen(inst);

			LoadBehavior(inst.level);
			LoadScripts(inst.level);
		}
		else
		{
			LoadLineDefs(inst);
		}
	}

	if (bad_linedef_count || bad_sector_refs || bad_sidedef_refs)
	{
		ShowLoadProblem();
	}

	// Node builders create a lot of new vertices for segs.
	// However they just get in the way for editing, so remove them.
	RemoveUnusedVerticesAtEnd(inst.level);

	inst.level.checks.sidedefsUnpack(true);

	CalculateLevelBounds(inst);
	Subdiv_InvalidateAll();

	MadeChanges = 0;
}


//
// open a new wad file.
// when 'map_name' is not NULL, try to open that map.
//
void OpenFileMap(const SString &filename, const SString &map_namem)
{
	// TODO: change this to start a new instance
	SString map_name = map_namem;
	if (! Main_ConfirmQuit("open another map"))
		return;


	Wad_file *wad = NULL;

	// make sure file exists, as Open() with 'a' would create it otherwise
	if (FileExists(filename))
	{
		wad = Wad_file::Open(filename, WadOpenMode_append);
	}

	if (! wad)
	{
		// FIXME: get an error message, add it here

		DLG_Notify("Unable to open that WAD file.");
		return;
	}


	// determine which level to use
	int lev_num = -1;

	if (map_name.good())
	{
		lev_num = wad->LevelFind(map_name);
	}

	if (lev_num < 0)
	{
		lev_num = wad->LevelFindFirst();
	}

	if (lev_num < 0)
	{
		DLG_Notify("No levels found in that WAD.");

		delete wad;
		return;
	}

	if (wad->FindLump(EUREKA_LUMP))
	{
		if (! gInstance.M_ParseEurekaLump(wad))
		{
			delete wad;
			return;
		}
	}


	/* OK, open it */


	// this wad replaces the current PWAD
	ReplaceEditWad(gInstance, wad);

	SYS_ASSERT(gInstance.edit_wad == wad);


	// always grab map_name from the actual level
	{
		int idx = gInstance.edit_wad->LevelHeader(lev_num);
		map_name  = gInstance.edit_wad->GetLump(idx)->Name();
	}

	LogPrintf("Loading Map : %s of %s\n", map_name.c_str(), gInstance.edit_wad->PathName().c_str());

	// TODO: new instance
	LoadLevel(gInstance, gInstance.edit_wad, map_name);

	// must be after LoadLevel as we need the Level_format
	// TODO: same here
	gInstance.Main_LoadResources();
}


void CMD_OpenMap(Instance &inst)
{
	if (! Main_ConfirmQuit("open another map"))
		return;


	UI_OpenMap * dialog = new UI_OpenMap(inst);

	SString map_name;
	bool did_load = false;

	Wad_file *wad = dialog->Run(&map_name, &did_load);

	delete dialog;

	if (! wad)	// cancelled
		return;


	// this shouldn't happen -- but just in case...
	if (wad->LevelFind(map_name) < 0)
	{
		DLG_Notify("Hmmmm, cannot find that map !?!");

		delete wad;
		return;
	}


	if (did_load && wad->FindLump(EUREKA_LUMP))
	{
		if (! inst.M_ParseEurekaLump(wad))
		{
			delete wad;
			return;
		}
	}


	// does this wad replace the currently edited wad?
	bool new_resources = false;

	if (did_load)
	{
		SYS_ASSERT(wad != inst.edit_wad);
		SYS_ASSERT(wad != inst.game_wad);

		ReplaceEditWad(inst, wad);

		new_resources = true;
	}
	// ...or does it remove the edit_wad? (e.g. wad == game_wad)
	else if (inst.edit_wad && wad != inst.edit_wad)
	{
		inst.RemoveEditWad();

		new_resources = true;
	}

	LogPrintf("Loading Map : %s of %s\n", map_name.c_str(), wad->PathName().c_str());

	// TODO: overhaul the interface to select map from the same wad
	LoadLevel(inst, wad, map_name);

	if (new_resources)
	{
		// this can invalidate the 'wad' var (since it closes/reopens
		// all wads in the master_dir), so it MUST be after LoadLevel.
		// less importantly, we need to know the Level_format.
		inst.Main_LoadResources();
	}
}


void CMD_GivenFile(Instance &inst)
{
	SString mode = EXEC_Param[0];

	int index = last_given_file;

	if (mode.empty() || mode.noCaseEqual("current"))
	{
		// index = index + 0;
	}
	else if (mode.noCaseEqual("next"))
	{
		index = index + 1;
	}
	else if (mode.noCaseEqual("prev"))
	{
		index = index - 1;
	}
	else if (mode.noCaseEqual("first"))
	{
		index = 0;
	}
	else if (mode.noCaseEqual("last"))
	{
		index = (int)global::Pwad_list.size() - 1;
	}
	else
	{
		inst.Beep("GivenFile: unknown keyword: %s", mode.c_str());
		return;
	}

	if (index < 0 || index >= (int)global::Pwad_list.size())
	{
		inst.Beep("No more files");
		return;
	}

	last_given_file = index;

	// TODO: remember last map visited in this wad

	OpenFileMap(global::Pwad_list[index], NULL);
}


void CMD_FlipMap(Instance &inst)
{
	SString mode = EXEC_Param[0];

	if (mode.empty())
	{
		inst.Beep("FlipMap: missing keyword");
		return;
	}


	if (! Main_ConfirmQuit("open another map"))
		return;


	Wad_file *wad = inst.edit_wad ? inst.edit_wad : inst.game_wad;

	// the level might not be found (lev_num < 0) -- that is OK
	int lev_idx = wad->LevelFind(inst.Level_name);
	int max_idx = wad->LevelCount() - 1;

	if (max_idx < 0)
	{
		inst.Beep("No maps ?!?");
		return;
	}

	SYS_ASSERT(lev_idx <= max_idx);


	if (mode.noCaseEqual("next"))
	{
		if (lev_idx < 0)
			lev_idx = 0;
		else if (lev_idx < max_idx)
			lev_idx++;
		else
		{
			inst.Beep("No more maps");
			return;
		}
	}
	else if (mode.noCaseEqual("prev"))
	{
		if (lev_idx < 0)
			lev_idx = max_idx;
		else if (lev_idx > 0)
			lev_idx--;
		else
		{
			inst.Beep("No more maps");
			return;
		}
	}
	else if (mode.noCaseEqual("first"))
	{
		lev_idx = 0;
	}
	else if (mode.noCaseEqual("last"))
	{
		lev_idx = max_idx;
	}
	else
	{
		inst.Beep("FlipMap: unknown keyword: %s", mode.c_str());
		return;
	}

	SYS_ASSERT(lev_idx >= 0);
	SYS_ASSERT(lev_idx <= max_idx);


	int lump_idx = wad->LevelHeader(lev_idx);
	Lump_c * lump  = wad->GetLump(lump_idx);
	const SString &map_name = lump->Name();

	LogPrintf("Flipping Map to : %s\n", map_name.c_str());

	LoadLevel(inst, wad, map_name);
}


//------------------------------------------------------------------------
//  SAVING CODE
//------------------------------------------------------------------------

static int saving_level;


static void SaveHeader(Instance &inst, const SString &level)
{
	int size = (int)inst.level.headerData.size();

	Lump_c *lump = inst.edit_wad->AddLevel(level, size, &saving_level);

	if (size > 0)
	{
		lump->Write(&inst.level.headerData[0], size);
	}

	lump->Finish();
}


static void SaveBehavior(Instance &inst)
{
	int size = (int)inst.level.behaviorData.size();

	Lump_c *lump = inst.edit_wad->AddLump("BEHAVIOR", size);

	if (size > 0)
	{
		lump->Write(&inst.level.behaviorData[0], size);
	}

	lump->Finish();
}


static void SaveScripts(Instance &inst)
{
	int size = (int)inst.level.scriptsData.size();

	if (size > 0)
	{
		Lump_c *lump = inst.edit_wad->AddLump("SCRIPTS", size);

		lump->Write(&inst.level.scriptsData[0], size);
		lump->Finish();
	}
}


static void SaveVertices(Instance &inst)
{
	int size = inst.level.numVertices() * (int)sizeof(raw_vertex_t);

	Lump_c *lump = inst.edit_wad->AddLump("VERTEXES", size);

	for (const Vertex *vert : inst.level.vertices)
	{
		raw_vertex_t raw;

		raw.x = LE_S16(COORD_TO_INT(vert->raw_x));
		raw.y = LE_S16(COORD_TO_INT(vert->raw_y));

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSectors(Instance &inst)
{
	int size = inst.level.numSectors() * (int)sizeof(raw_sector_t);

	Lump_c *lump = inst.edit_wad->AddLump("SECTORS", size);

	for (const Sector *sec : inst.level.sectors)
	{
		raw_sector_t raw;

		raw.floorh = LE_S16(sec->floorh);
		raw.ceilh  = LE_S16(sec->ceilh);

		W_StoreString(raw.floor_tex, sec->FloorTex(), sizeof(raw.floor_tex));
		W_StoreString(raw.ceil_tex,  sec->CeilTex(),  sizeof(raw.ceil_tex));

		raw.light = LE_U16(sec->light);
		raw.type  = LE_U16(sec->type);
		raw.tag   = LE_U16(sec->tag);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveThings(Instance &inst)
{
	int size = inst.level.numThings() * (int)sizeof(raw_thing_t);

	Lump_c *lump = inst.edit_wad->AddLump("THINGS", size);

	for (const Thing *th : inst.level.things)
	{
		raw_thing_t raw;

		raw.x = LE_S16(COORD_TO_INT(th->raw_x));
		raw.y = LE_S16(COORD_TO_INT(th->raw_y));

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


// IOANCH 9/2015
static void SaveThings_Hexen(Instance &inst)
{
	int size = inst.level.numThings() * (int)sizeof(raw_hexen_thing_t);

	Lump_c *lump = inst.edit_wad->AddLump("THINGS", size);

	for (const Thing *th : inst.level.things)
	{
		raw_hexen_thing_t raw;

		raw.tid = LE_S16(th->tid);

		raw.x = LE_S16(COORD_TO_INT(th->raw_x));
		raw.y = LE_S16(COORD_TO_INT(th->raw_y));
		raw.height = LE_S16(COORD_TO_INT(th->raw_h));

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		raw.special = th->special;
		raw.args[0] = th->arg1;
		raw.args[1] = th->arg2;
		raw.args[2] = th->arg3;
		raw.args[3] = th->arg4;
		raw.args[4] = th->arg5;

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveSideDefs(Instance &inst)
{
	int size = inst.level.numSidedefs() * (int)sizeof(raw_sidedef_t);

	Lump_c *lump = inst.edit_wad->AddLump("SIDEDEFS", size);

	for (const SideDef *side : inst.level.sidedefs)
	{
		raw_sidedef_t raw;

		raw.x_offset = LE_S16(side->x_offset);
		raw.y_offset = LE_S16(side->y_offset);

		W_StoreString(raw.upper_tex, side->UpperTex(), sizeof(raw.upper_tex));
		W_StoreString(raw.lower_tex, side->LowerTex(), sizeof(raw.lower_tex));
		W_StoreString(raw.mid_tex,   side->MidTex(),   sizeof(raw.mid_tex));

		raw.sector = LE_U16(side->sector);

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void SaveLineDefs(Instance &inst)
{
	int size = inst.level.numLinedefs() * (int)sizeof(raw_linedef_t);

	Lump_c *lump = inst.edit_wad->AddLump("LINEDEFS", size);

	for (const LineDef *ld : inst.level.linedefs)
	{
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


// IOANCH 9/2015
static void SaveLineDefs_Hexen(Instance &inst)
{
	int size = inst.level.numLinedefs() * (int)sizeof(raw_hexen_linedef_t);

	Lump_c *lump = inst.edit_wad->AddLump("LINEDEFS", size);

	for (const LineDef *ld : inst.level.linedefs)
	{
		raw_hexen_linedef_t raw;

		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = ld->type;

		raw.args[0] = ld->tag;
		raw.args[1] = ld->arg2;
		raw.args[2] = ld->arg3;
		raw.args[3] = ld->arg4;
		raw.args[4] = ld->arg5;

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump->Write(&raw, sizeof(raw));
	}

	lump->Finish();
}


static void EmptyLump(const Instance &inst, const char *name)
{
	inst.edit_wad->AddLump(name)->Finish();
}


//
// Write out the level data
//

static void SaveLevel(Instance &inst, const SString &level)
{
	// set global level name now (for debugging code)
	inst.Level_name = level.asUpper();

	inst.edit_wad->BeginWrite();

	// remove previous version of level (if it exists)
	int lev_num = inst.edit_wad->LevelFind(level);
	int level_lump = -1;

	if (lev_num >= 0)
	{
		level_lump = inst.edit_wad->LevelHeader(lev_num);

		inst.edit_wad->RemoveLevel(lev_num);
	}

	inst.edit_wad->InsertPoint(level_lump);

	SaveHeader(inst, level);

	if (inst.Level_format == MapFormat::udmf)
	{
		UDMF_SaveLevel(inst);
	}
	else
	{
		// IOANCH 9/2015: save Hexen format maps
		if (inst.Level_format == MapFormat::hexen)
		{
			SaveThings_Hexen(inst);
			SaveLineDefs_Hexen(inst);
		}
		else
		{
			SaveThings(inst);
			SaveLineDefs(inst);
		}

		SaveSideDefs(inst);
		SaveVertices(inst);

		EmptyLump(inst, "SEGS");
		EmptyLump(inst, "SSECTORS");
		EmptyLump(inst, "NODES");

		SaveSectors(inst);

		EmptyLump(inst, "REJECT");
		EmptyLump(inst, "BLOCKMAP");

		if (inst.Level_format == MapFormat::hexen)
		{
			SaveBehavior(inst);
			SaveScripts(inst);
		}
	}

	// write out the new directory
	inst.edit_wad->EndWrite();


	// build the nodes
	if (config::bsp_on_save && ! inhibit_node_build)
	{
		BuildNodesAfterSave(inst, saving_level);
	}


	// this is mainly for Next/Prev-map commands
	// [ it doesn't change the on-disk wad file at all ]
	inst.edit_wad->SortLevels();

	inst.M_WriteEurekaLump(inst.edit_wad);

	M_AddRecent(inst.edit_wad->PathName(), inst.Level_name);

	inst.Status_Set("Saved %s", inst.Level_name.c_str());

	if (inst.main_win)
	{
		inst.main_win->SetTitle(inst.edit_wad->PathName(), inst.Level_name, false);

		// save the user state associated with this map
		M_SaveUserState(inst);
	}

	MadeChanges = 0;
}

static bool M_ExportMap(Instance &inst);

bool M_SaveMap(Instance &inst)
{
	// we require a wad file to save into.
	// if there is none, then need to create one via Export function.

	if (!inst.edit_wad)
	{
		return M_ExportMap(inst);
	}

	if (inst.edit_wad->IsReadOnly())
	{
		if (DLG_Confirm("Cancel|&Export",
		                "The current pwad is a READ-ONLY file. "
						"Do you want to export this map into a new file?") <= 0)
		{
			return false;
		}

		return M_ExportMap(inst);
	}


	M_BackupWad(inst.edit_wad);

	LogPrintf("Saving Map : %s in %s\n", inst.Level_name.c_str(), inst.edit_wad->PathName().c_str());

	SaveLevel(inst, inst.Level_name);

	return true;
}


static bool M_ExportMap(Instance &inst)
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to export to");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder().c_str());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			LogPrintf("Export Map: error choosing file:\n");
			LogPrintf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to export the map:\n\n%s", chooser.errmsg());
			return false;

		case 1:
			LogPrintf("Export Map: cancelled by user\n");
			return false;

		default:
			break;  // OK
	}

	// if extension is missing then add ".wad"
	SString filename = chooser.filename();

	char *pos = (char *)fl_filename_ext(filename.c_str());
	if(!*pos)
		filename += ".wad";


	// don't export into a file we currently have open
	if (inst.MasterDir_HaveFilename(filename))
	{
		DLG_Notify("Unable to export the map:\n\nFile already in use");
		return false;
	}


	// does the file already exist?  if not, create it...
	bool exists = FileExists(filename);

	Wad_file *wad;

	if (exists)
	{
		wad = Wad_file::Open(filename, WadOpenMode_append);

		if (wad && wad->IsReadOnly())
		{
			DLG_Notify("Cannot export the map into a READ-ONLY file.");

			delete wad;
			return false;
		}

		// adopt iwad/port/resources of the target wad
		if (wad->FindLump(EUREKA_LUMP))
		{
			if (! inst.M_ParseEurekaLump(wad))
			{
				delete wad;
				return false;
			}
		}
	}
	else
	{
		wad = Wad_file::Open(filename, WadOpenMode_write);
	}

	if (! wad)
	{
		DLG_Notify("Unable to export the map:\n\n%s",
		           "Error creating output file");
		return false;
	}


	// ask user for map name

	UI_ChooseMap * dialog = new UI_ChooseMap(inst.Level_name.c_str());

	dialog->PopulateButtons(toupper(inst.Level_name[0]), wad);

	SString map_name = dialog->Run();

	delete dialog;


	// cancelled?
	if (map_name.empty())
	{
		delete wad;
		return false;
	}


	// we will write into the chosen wad.
	// however if the level already exists, get confirmation first

	if (exists && wad->LevelFind(map_name) >= 0)
	{
		if (DLG_Confirm("Cancel|&Overwrite",
		                overwrite_message, "selected") <= 0)
		{
			delete wad;
			return false;
		}
	}

	// back-up an existing wad
	if (exists)
	{
		M_BackupWad(wad);
	}


	LogPrintf("Exporting Map : %s in %s\n", map_name.c_str(), wad->PathName().c_str());

	// the new wad replaces the current PWAD
	ReplaceEditWad(inst, wad);

	SaveLevel(inst, map_name);

	// do this after the save (in case it fatal errors)
	inst.Main_LoadResources();

	return true;
}


void CMD_SaveMap(Instance &inst)
{
	M_SaveMap(inst);
}


void CMD_ExportMap(Instance &inst)
{
	M_ExportMap(inst);
}


//------------------------------------------------------------------------
//  COPY, RENAME and DELETE MAP
//------------------------------------------------------------------------

void CMD_CopyMap(Instance &inst)
{
	if (!inst.edit_wad)
	{
		DLG_Notify("Cannot copy a map unless editing a PWAD.");
		return;
	}

	if (inst.edit_wad->IsReadOnly())
	{
		DLG_Notify("Cannot copy map : file is read-only.");
		return;
	}

	// ask user for map name

	UI_ChooseMap * dialog = new UI_ChooseMap(inst.Level_name.c_str(), inst.edit_wad);

	dialog->PopulateButtons(toupper(inst.Level_name[0]), inst.edit_wad);

	SString new_name = dialog->Run();

	delete dialog;

	// cancelled?
	if (new_name.empty())
		return;

	// sanity check that the name is different
	// (should be prevented by the choose-map dialog)
	if (y_stricmp(new_name.c_str(), inst.Level_name.c_str()) == 0)
	{
		inst.Beep("Name is same!?!");
		return;
	}

	// perform the copy (just a save)
	LogPrintf("Copying Map : %s --> %s\n", inst.Level_name.c_str(), new_name.c_str());

	SaveLevel(inst, new_name);

	inst.Status_Set("Copied to %s", inst.Level_name.c_str());
}


void CMD_RenameMap(Instance &inst)
{
	if (!inst.edit_wad)
	{
		DLG_Notify("Cannot rename a map unless editing a PWAD.");
		return;
	}

	if (inst.edit_wad->IsReadOnly())
	{
		DLG_Notify("Cannot rename map : file is read-only.");
		return;
	}


	// ask user for map name

	UI_ChooseMap * dialog = new UI_ChooseMap(inst.Level_name.c_str(), inst.edit_wad /* rename_wad */);

	// pick level format from the IWAD
	// [ user may be trying to rename map after changing the IWAD ]
	char format = 'M';
	{
		int idx = inst.game_wad->LevelFindFirst();

		if (idx >= 0)
		{
			idx = inst.game_wad->LevelHeader(idx);
			const SString &name = inst.game_wad->GetLump(idx)->Name();
			format = toupper(name[0]);
		}
	}

	dialog->PopulateButtons(format, inst.edit_wad);

	SString new_name = dialog->Run();

	delete dialog;

	// cancelled?
	if (new_name.empty())
		return;

	// sanity check that the name is different
	// (should be prevented by the choose-map dialog)
	if (y_stricmp(new_name.c_str(), inst.Level_name.c_str()) == 0)
	{
		inst.Beep("Name is same!?!");
		return;
	}


	// perform the rename
	int lev_num = inst.edit_wad->LevelFind(inst.Level_name);

	if (lev_num >= 0)
	{
		int level_lump = inst.edit_wad->LevelHeader(lev_num);

		inst.edit_wad->BeginWrite();
		inst.edit_wad->RenameLump(level_lump, new_name.c_str());
		inst.edit_wad->EndWrite();
	}

	inst.Level_name = new_name.asUpper();

	inst.main_win->SetTitle(inst.edit_wad->PathName(), inst.Level_name, false);

	inst.Status_Set("Renamed to %s", inst.Level_name.c_str());
}


void CMD_DeleteMap(Instance &inst)
{
	if (!inst.edit_wad)
	{
		DLG_Notify("Cannot delete a map unless editing a PWAD.");
		return;
	}

	if (inst.edit_wad->IsReadOnly())
	{
		DLG_Notify("Cannot delete map : file is read-only.");
		return;
	}

	if (inst.edit_wad->LevelCount() < 2)
	{
		// perhaps ask either to Rename map, or Delete the file (and Eureka will shut down)

		DLG_Notify("Cannot delete the last map in a PWAD.");
		return;
	}

	if (DLG_Confirm("Cancel|&Delete",
	                "Are you sure you want to delete this map? "
					"It will be permanently removed from the current PWAD.") <= 0)
	{
		return;
	}

	LogPrintf("Deleting Map : %s...\n", inst.Level_name.c_str());

	int lev_num = inst.edit_wad->LevelFind(inst.Level_name);

	if (lev_num < 0)
	{
		inst.Beep("No such map ?!?");
		return;
	}


	// kick it to the curb
	inst.edit_wad->BeginWrite();
	inst.edit_wad->RemoveLevel(lev_num);
	inst.edit_wad->EndWrite();


	// choose a new level to load
	{
		if (lev_num >= inst.edit_wad->LevelCount())
			lev_num = inst.edit_wad->LevelCount() - 1;

		int lump_idx = inst.edit_wad->LevelHeader(lev_num);
		Lump_c * lump  = inst.edit_wad->GetLump(lump_idx);
		const SString &map_name = lump->Name();

		LogPrintf("OK.  Loading : %s....\n", map_name.c_str());

		// TODO: overhaul the interface to NOT go back to the IWAD
		LoadLevel(inst, inst.edit_wad, map_name);
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
