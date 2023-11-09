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
#include "LineDef.h"
#include "m_config.h"
#include "m_files.h"
#include "m_loadsave.h"
#include "r_subdiv.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"
#include "w_wad.h"

#include "ui_window.h"
#include "ui_file.h"

#include <memory>

static const char overwrite_message[] =
	"The %s PWAD already contains this map.  "
	"This operation will destroy that map (overwrite it)."
	"\n\n"
	"Are you sure you want to continue?";


void MasterDir::RemoveEditWad()
{
	edit_wad.reset();
}


void MasterDir::ReplaceEditWad(const std::shared_ptr<Wad_file> &new_wad)
{
	edit_wad = new_wad;
}


void Instance::FreshLevel()
{
	level.clear();

	auto sec = std::make_unique<Sector>();


	sec->SetDefaults(conf);
	level.sectors.push_back(std::move(sec));

	for (int i = 0 ; i < 4 ; i++)
	{
		auto v = std::make_unique<Vertex>();

		v->SetRawX(loaded.levelFormat, (i >= 2) ? 256 : -256);
		v->SetRawY(loaded.levelFormat, (i==1 || i==2) ? 256 :-256);
		level.vertices.push_back(std::move(v));

		auto sd = std::make_unique<SideDef>();
		sd->SetDefaults(conf, false);
		level.sidedefs.push_back(std::move(sd));

		auto ld = std::make_unique<LineDef>();
		ld->start = i;
		ld->end   = (i+1) % 4;
		ld->flags = MLF_Blocking;
		ld->right = i;
		level.linedefs.push_back(std::move(ld));
	}

	for (int pl = 1 ; pl <= 4 ; pl++)
	{
		auto th = std::make_unique<Thing>();

		th->type  = pl;
		th->angle = 90;

		th->SetRawX(loaded.levelFormat, (pl == 1) ? 0 : (pl - 3) * 48);
		th->SetRawY(loaded.levelFormat, (pl == 1) ? 48 : (pl == 3) ? -48 : 0);
		level.things.push_back(std::move(th));
	}

	CalculateLevelBounds();

	ZoomWholeMap();

	edit.defaultState();
}


tl::optional<fs::path> Instance::Project_AskFile() const
{
	// this returns false if user cancelled

	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to create");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder().u8string().c_str());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			gLog.printf("New Project: error choosing file:\n");
			gLog.printf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to create a new project:\n\n%s", chooser.errmsg());
			return {};

		case 1:
			gLog.printf("New Project: cancelled by user\n");
			return {};

		default:
			break;  // OK
	}

	// if extension is missing, add ".wad"
	fs::path filename = fs::u8path(chooser.filename());

	fs::path extension = filename.extension();
	if(extension.empty())
		filename = fs::u8path(filename.u8string() + ".wad");

	return filename;
}


void Instance::Project_ApplyChanges(const UI_ProjectSetup::Result &result) noexcept(false)
{
	// grab the new information
    LoadingData loading = loaded;
	loading.gameName = result.game;
	loading.portName = result.port;
	const fs::path *iwad = global::recent.queryIWAD(result.game);
	SYS_ASSERT(iwad);
	loading.iwadName = *iwad;
	loading.levelFormat = result.mapFormat;
	loading.udmfNamespace = result.nameSpace;
	loading.resourceList.clear();
	for(int i = 0; i < UI_ProjectSetup::RES_NUM; ++i)
		if(!result.resources[i].empty())
			loading.resourceList.push_back(result.resources[i]);
	Fl::wait(0.1);
	Main_LoadResources(loading);
	Fl::wait(0.1);
}


void Instance::CMD_ManageProject()
{
	try
	{
		UI_ProjectSetup dialog(*this, false /* new_project */, false /* is_startup */);
		tl::optional<UI_ProjectSetup::Result> result = dialog.Run();

		if (result)
		{
			Project_ApplyChanges(*result);
		}
	}
	catch(const std::runtime_error &e)
	{
		DLG_ShowError(false, "Error managing project: %s", e.what());
	}
}


void Instance::CMD_NewProject()
{
	std::shared_ptr<Wad_file> initialEditWad = wad.master.editWad();
	try
	{
		if (!Main_ConfirmQuit("create a new project"))
			return;

		/* first, ask for the output file */

		tl::optional<fs::path> filename = Project_AskFile();

		if (!filename)
			return;


		/* second, query what Game, Port and Resources to use */
		// TODO: new instance
		UI_ProjectSetup dialog(*this, true /* new_project */, false /* is_startup */);

		tl::optional<UI_ProjectSetup::Result> result = dialog.Run();

		if (!result)
		{
			return;
		}

		wad.master.RemoveEditWad();

		// this calls Main_LoadResources which resets the master directory
		Project_ApplyChanges(*result);

		// determine map name (same as first level in the IWAD)
		SString map_name = "MAP01";

		int idx = wad.master.gameWad()->LevelFindFirst();

		if (idx >= 0)
		{
			idx = wad.master.gameWad()->LevelHeader(idx);
			map_name = wad.master.gameWad()->GetLump(idx)->Name();
		}

		gLog.printf("Creating New File : %s in %s\n", map_name.c_str(), filename->u8string().c_str());


		std::shared_ptr<Wad_file> wad = Wad_file::Open(*filename, WadOpenMode::write);

		if (!wad)
		{
			DLG_Notify("Unable to create the new WAD file.");
			return;
		}

		this->wad.master.ReplaceEditWad(wad);

		// TODO: new instance
		FreshLevel();

		// save it now : sets Level_name and window title
		SaveLevel(map_name);
	}
	catch (const std::runtime_error& e)
	{
		// Restore state before command
		wad.master.ReplaceEditWad(initialEditWad);
		DLG_ShowError(false, "Could not start new project: %s", e.what());
	}

}


bool Instance::MissingIWAD_Dialog()
{
	UI_ProjectSetup dialog(*this, false /* new_project */, true /* is_startup */);

	tl::optional<UI_ProjectSetup::Result> result = dialog.Run();

	if (result)
	{
		loaded.gameName = result->game;
		SYS_ASSERT(!loaded.gameName.empty());

		const fs::path *iwad = global::recent.queryIWAD(loaded.gameName);
		SYS_ASSERT(!!iwad);
		loaded.iwadName = *iwad;
	}

	return result.has_value();
}


void Instance::CMD_FreshMap()
{
	try
	{
		if (!wad.master.editWad())
		{
			DLG_Notify("Cannot create a fresh map unless editing a PWAD.");
			return;
		}

		if (wad.master.editWad()->IsReadOnly())
		{
			DLG_Notify("Cannot create a fresh map : file is read-only.");
			return;
		}

		if (!Main_ConfirmQuit("create a fresh map"))
			return;


		UI_ChooseMap* dialog = new UI_ChooseMap(loaded.levelName.c_str());

		dialog->PopulateButtons(static_cast<char>(toupper(loaded.levelName[0])), wad.master.editWad().get());

		SString map_name = dialog->Run();

		delete dialog;

		// cancelled?
		if (map_name.empty())
			return;

		// would this replace an existing map?
		if (wad.master.editWad()->LevelFind(map_name) >= 0)
		{
			if (DLG_Confirm({ "Cancel", "&Overwrite" },
				overwrite_message, "current") <= 0)
			{
				return;
			}
		}


		M_BackupWad(wad.master.editWad().get());

		gLog.printf("Created NEW map : %s\n", map_name.c_str());

		// TODO: make this allow running another level
		FreshLevel();

		// save it now : sets Level_name and window title
		SaveLevel(map_name);
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not create fresh map: %s", e.what());
	}

}


//------------------------------------------------------------------------
//  LOADING CODE
//------------------------------------------------------------------------

static void UpperCaseShortStr(char *buf, int max_len)
{
	for (int i = 0 ; (i < max_len) && buf[i] ; i++)
	{
		buf[i] = static_cast<char>(toupper(buf[i]));
	}
}


const Lump_c *Load_LookupAndSeek(int loading_level, const Wad_file *load_wad, const char *name)
{
	int idx = load_wad->LevelLookupLump(loading_level, name);

	if (idx < 0)
		return NULL;

	const Lump_c *lump = load_wad->GetLump(idx);

	return lump;
}


void Document::LoadVertices(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "VERTEXES");
	if (! lump)
		ThrowException("No vertex lump!\n");

	int count = lump->Length() / sizeof(raw_vertex_t);

# if DEBUG_LOAD
	PrintDebug("GetVertices: num = %d\n", count);
# endif

	vertices.reserve(count);
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_vertex_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading vertices.\n");

		auto vert = std::make_unique<Vertex>();

		vert->raw_x = FFixedPoint(LE_S16(raw.x));
		vert->raw_y = FFixedPoint(LE_S16(raw.y));

		vertices.push_back(std::move(vert));
	}
}


void Document::LoadSectors(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "SECTORS");
	if (! lump)
		ThrowException("No sector lump!\n");

	int count = lump->Length() / sizeof(raw_sector_t);

# if DEBUG_LOAD
	PrintDebug("GetSectors: num = %d\n", count);
# endif

	sectors.reserve(count);
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_sector_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading sectors.\n");

		auto sec = std::make_unique<Sector>();

		sec->floorh = LE_S16(raw.floorh);
		sec->ceilh  = LE_S16(raw.ceilh);

		UpperCaseShortStr(raw.floor_tex, 8);
		UpperCaseShortStr(raw. ceil_tex, 8);

		sec->floor_tex = BA_InternaliseString(SString(raw.floor_tex, 8));
		sec->ceil_tex  = BA_InternaliseString(SString(raw.ceil_tex,  8));

		sec->light = LE_U16(raw.light);
		sec->type  = LE_U16(raw.type);
		sec->tag   = LE_S16(raw.tag);

		sectors.push_back(std::move(sec));
	}
}


void Instance::CreateFallbackSector()
{
	gLog.printf("Creating a fallback sector.\n");

	auto sec = std::make_unique<Sector>();

	sec->SetDefaults(conf);

	level.sectors.push_back(std::move(sec));
}

void Instance::CreateFallbackSideDef()
{
	// we need a valid sector too!
	if (level.numSectors() == 0)
		CreateFallbackSector();

	gLog.printf("Creating a fallback sidedef.\n");

	auto sd = std::make_unique<SideDef>();

	sd->SetDefaults(conf, false);

	level.sidedefs.push_back(std::move(sd));
}

static void CreateFallbackVertices(Document &doc)
{
	gLog.printf("Creating two fallback vertices.\n");

	auto v1 = std::make_unique<Vertex>();
	auto v2 = std::make_unique<Vertex>();

	v1->raw_x = FFixedPoint(-777);
	v1->raw_y = FFixedPoint(-777);

	v2->raw_x = FFixedPoint(555);
	v2->raw_y = FFixedPoint(555);

	doc.vertices.push_back(std::move(v1));
	doc.vertices.push_back(std::move(v2));
}


void Instance::ValidateSidedefRefs(LineDef * ld, int num)
{
	if (ld->right >= level.numSidedefs() || ld->left >= level.numSidedefs())
	{
		gLog.printf("WARNING: linedef #%d has invalid sidedefs (%d / %d)\n",
				  num, ld->right, ld->left);

		bad_sidedef_refs++;

		// ensure we have a usable sidedef
		if (level.numSidedefs() == 0)
			CreateFallbackSideDef();

		if (ld->right >= level.numSidedefs())
			ld->right = 0;

		if (ld->left >= level.numSidedefs())
			ld->left = 0;
	}
}

void Instance::ValidateVertexRefs(LineDef *ld, int num)
{
	if (ld->start >= level.numVertices() || ld->end >= level.numVertices() ||
	    ld->start == ld->end)
	{
		gLog.printf("WARNING: linedef #%d has invalid vertices (%d -> %d)\n",
		          num, ld->start, ld->end);

		bad_linedef_count++;

		// ensure we have a valid vertex
		if (level.numVertices() < 2)
			CreateFallbackVertices(level);

		ld->start = 0;
		ld->end   = level.numVertices() - 1;
	}
}

void Instance::ValidateSectorRef(SideDef *sd, int num)
{
	if (sd->sector >= level.numSectors())
	{
		gLog.printf("WARNING: sidedef #%d has invalid sector (%d)\n",
		          num, sd->sector);

		bad_sector_refs++;

		// ensure we have a valid sector
		if (level.numSectors() == 0)
			CreateFallbackSector();

		sd->sector = 0;
	}
}


void Document::LoadHeader(int loading_level, const Wad_file &load_wad)
{
	const Lump_c *lump = load_wad.GetLump(load_wad.LevelHeader(loading_level));
	headerData = lump->getData();
}


void Instance::LoadBehavior(int loading_level, const Wad_file *load_wad)
{
	// IOANCH 9/2015: support Hexen maps
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "BEHAVIOR");
	if (! lump)
		ThrowException("No BEHAVIOR lump!\n");
	
	level.behaviorData = lump->getData();
}


void Instance::LoadScripts(int loading_level, const Wad_file *load_wad)
{
	// the SCRIPTS lump is usually absent
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "SCRIPTS");
	if (! lump)
		return;
	
	level.scriptsData = lump->getData();
}


void Document::LoadThings(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "THINGS");
	if (! lump)
		ThrowException("No things lump!\n");

	int count = lump->Length() / sizeof(raw_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif
	
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_thing_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading things.\n");

		auto th = std::make_unique<Thing>();

		th->raw_x = FFixedPoint(LE_S16(raw.x));
		th->raw_y = FFixedPoint(LE_S16(raw.y));

		th->angle   = LE_U16(raw.angle);
		th->type    = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		things.push_back(std::move(th));
	}
}


// IOANCH 9/2015
void Document::LoadThings_Hexen(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "THINGS");
	if (! lump)
		ThrowException("No things lump!\n");

	int count = lump->Length() / sizeof(raw_hexen_thing_t);

# if DEBUG_LOAD
	PrintDebug("GetThings: num = %d\n", count);
# endif
	
	LumpInputStream stream(*lump);

	for (int i = 0; i < count; ++i)
	{
		raw_hexen_thing_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading things.\n");

		auto th = std::make_unique<Thing>();

		th->tid = LE_S16(raw.tid);
		th->raw_x = FFixedPoint(LE_S16(raw.x));
		th->raw_y = FFixedPoint(LE_S16(raw.y));
		th->raw_h = FFixedPoint(LE_S16(raw.height));

		th->angle = LE_U16(raw.angle);
		th->type = LE_U16(raw.type);
		th->options = LE_U16(raw.options);

		th->special = raw.special;
		th->arg1 = raw.args[0];
		th->arg2 = raw.args[1];
		th->arg3 = raw.args[2];
		th->arg4 = raw.args[3];
		th->arg5 = raw.args[4];

		things.push_back(std::move(th));
	}
}


void Instance::LoadSideDefs(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "SIDEDEFS");
	if(!lump)
		ThrowException("No sidedefs lump!\n");

	int count = lump->Length() / sizeof(raw_sidedef_t);

# if DEBUG_LOAD
	PrintDebug("GetSidedefs: num = %d\n", count);
# endif
	
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_sidedef_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading sidedefs.\n");

		auto sd = std::make_unique<SideDef>();

		sd->x_offset = LE_S16(raw.x_offset);
		sd->y_offset = LE_S16(raw.y_offset);

		UpperCaseShortStr(raw.upper_tex, 8);
		UpperCaseShortStr(raw.lower_tex, 8);
		UpperCaseShortStr(raw.  mid_tex, 8);

		sd->upper_tex = BA_InternaliseString(SString(raw.upper_tex, 8));
		sd->lower_tex = BA_InternaliseString(SString(raw.lower_tex, 8));
		sd->  mid_tex = BA_InternaliseString(SString(raw.  mid_tex, 8));

		sd->sector = LE_U16(raw.sector);

		ValidateSectorRef(sd.get(), i);

		level.sidedefs.push_back(std::move(sd));
	}
}


void Instance::LoadLineDefs(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "LINEDEFS");
	if (! lump)
		ThrowException("No linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		return;
	
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_linedef_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading linedefs.\n");

		auto ld = std::make_unique<LineDef>();

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type  = LE_U16(raw.type);
		ld->tag   = LE_S16(raw.tag);

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		if (ld->right == 0xFFFF) ld->right = -1;
		if (ld-> left == 0xFFFF) ld-> left = -1;

		ValidateVertexRefs(ld.get(), i);
		ValidateSidedefRefs(ld.get(), i);

		level.linedefs.push_back(std::move(ld));
	}
}


// IOANCH 9/2015
void Instance::LoadLineDefs_Hexen(int loading_level, const Wad_file *load_wad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "LINEDEFS");
	if (! lump)
		ThrowException("No linedefs lump!\n");

	int count = lump->Length() / sizeof(raw_hexen_linedef_t);

# if DEBUG_LOAD
	PrintDebug("GetLinedefs: num = %d\n", count);
# endif

	if (count == 0)
		return;
	
	LumpInputStream stream(*lump);

	for (int i = 0 ; i < count ; i++)
	{
		raw_hexen_linedef_t raw;

		if (! stream.read(&raw, sizeof(raw)))
			ThrowException("Error reading linedefs.\n");

		auto ld = std::make_unique<LineDef>();

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

		ValidateVertexRefs(ld.get(), i);
		ValidateSidedefRefs(ld.get(), i);

		level.linedefs.push_back(std::move(ld));
	}
}


static void RemoveUnusedVerticesAtEnd(Document &doc)
{
	if (doc.numVertices() == 0)
		return;

	bitvec_c used_verts(doc.numVertices());

	for (const auto &linedef : doc.linedefs)
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
		gLog.printf("Removing %d unused vertices at end\n", doc.numVertices() - new_count);

		doc.vertices.resize(new_count);
	}
}


void Instance::ShowLoadProblem() const
{
	gLog.printf("Map load problems:\n");
	gLog.printf("   %d linedefs with bad vertex refs\n", bad_linedef_count);
	gLog.printf("   %d linedefs with bad sidedef refs\n", bad_sidedef_refs);
	gLog.printf("   %d sidedefs with bad sector refs\n", bad_sector_refs);

	SString message;

	if (bad_linedef_count > 0)
	{
		message = SString::printf("Found %d linedefs with bad vertex references.\n"
			"These references have been replaced.",
			bad_linedef_count);
	}
	else
	{
		message = SString::printf("Found %d bad sector refs, %d bad sidedef refs.\n"
			"These references have been replaced.",
			bad_sector_refs, bad_sidedef_refs);
	}

	DLG_Notify("Map validation report:\n\n%s", message.c_str());
}

//
// Read in the level data
//

void Instance::LoadLevel(const Wad_file *wad, const SString &level)
{
	int lev_num = wad->LevelFind(level);

	if (lev_num < 0)
		ThrowException("No such map: %s\n", level.c_str());

	LoadLevelNum(wad, lev_num);

	// reset various editor state
	Editor_ClearAction();
	Selection_InvalidateLast();

	edit.Selected->clear_all();
	edit.highlight.clear();

	main_win->UpdateTotals();
	main_win->UpdateGameInfo();
	main_win->InvalidatePanelObj();
	main_win->redraw();

	if (main_win)
	{
		main_win->SetTitle(wad->PathName().u8string(), level, wad->IsReadOnly());

		// load the user state associated with this map
		crc32_c adler_crc;

		this->level.getLevelChecksum(adler_crc);

		if (! M_LoadUserState())
		{
			M_DefaultUserState();
		}
	}

	loaded.levelName = level.asUpper();

	Status_Set("Loaded %s", loaded.levelName.c_str());

	RedrawMap();
}


void Instance::LoadLevelNum(const Wad_file *wad, int lev_num)
{
	int loading_level = lev_num;

	loaded.levelFormat = wad->LevelFormat(loading_level);

	level.clear();

	bad_linedef_count = 0;
	bad_sector_refs   = 0;
	bad_sidedef_refs  = 0;

	level.LoadHeader(loading_level, *wad);

	if (loaded.levelFormat == MapFormat::udmf)
	{
		UDMF_LoadLevel(loading_level, wad);
	}
	else
	{
		if (loaded.levelFormat == MapFormat::hexen)
			level.LoadThings_Hexen(loading_level, wad);
		else
			level.LoadThings(loading_level, wad);

		level.LoadVertices(loading_level, wad);
		level.LoadSectors(loading_level, wad);
		LoadSideDefs(loading_level, wad);

		if (loaded.levelFormat == MapFormat::hexen)
		{
			LoadLineDefs_Hexen(loading_level, wad);

			LoadBehavior(loading_level, wad);
			LoadScripts(loading_level, wad);
		}
		else
		{
			LoadLineDefs(loading_level, wad);
		}
	}

	if (bad_linedef_count || bad_sector_refs || bad_sidedef_refs)
	{
		ShowLoadProblem();
	}

	// Node builders create a lot of new vertices for segs.
	// However they just get in the way for editing, so remove them.
	RemoveUnusedVerticesAtEnd(level);

	level.checks.sidedefsUnpack(true);

	CalculateLevelBounds();
	Subdiv_InvalidateAll();

	MadeChanges = false;
}


//
// open a new wad file.
// when 'map_name' is not NULL, try to open that map.
//
void OpenFileMap(const fs::path &filename, const SString &map_namem) noexcept(false)
{
	// TODO: change this to start a new instance
	SString map_name = map_namem;
	if (! gInstance.Main_ConfirmQuit("open another map"))
		return;


	std::shared_ptr<Wad_file> wad;

	// make sure file exists, as Open() with 'a' would create it otherwise
	if (FileExists(filename))
	{
		wad = Wad_file::Open(filename, WadOpenMode::append);
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

		return;
	}

	if (wad->FindLump(EUREKA_LUMP))
	{
		if (! gInstance.loaded.parseEurekaLump(global::home_dir, global::install_dir, global::recent, wad.get()))
		{
			return;
		}
	}


	/* OK, open it */


	// this wad replaces the current PWAD
	gInstance.wad.master.ReplaceEditWad(wad);

	SYS_ASSERT(gInstance.wad.master.editWad() == wad);


	// always grab map_name from the actual level
	{
		int idx = gInstance.wad.master.editWad()->LevelHeader(lev_num);
		map_name  = gInstance.wad.master.editWad()->GetLump(idx)->Name();
	}

	gLog.printf("Loading Map : %s of %s\n", map_name.c_str(), gInstance.wad.master.editWad()->PathName().u8string().c_str());

	// TODO: new instance
	gInstance.LoadLevel(gInstance.wad.master.editWad().get(), map_name);

	// must be after LoadLevel as we need the Level_format
	// TODO: same here
	gInstance.Main_LoadResources(gInstance.loaded);
	
	// TODO: RESTORE loaded, wad.master, LoadLevel ON FAILURE
	// LoadLevel: level.basis
}


void Instance::CMD_OpenMap()
{
	if (! Main_ConfirmQuit("open another map"))
		return;

	
	SString map_name;
	bool did_load = false;
	
	std::shared_ptr<Wad_file> wad = UI_OpenMap(*this).Run(&map_name, &did_load);

	if (! wad)	// cancelled
		return;


	// this shouldn't happen -- but just in case...
	if (wad->LevelFind(map_name) < 0)
	{
		DLG_Notify("Hmmmm, cannot find that map !?!");

		return;
	}


	if (did_load && wad->FindLump(EUREKA_LUMP))
	{
		if (! loaded.parseEurekaLump(global::home_dir, global::install_dir, global::recent, wad.get()))
			return;
	}


	// does this wad replace the currently edited wad?
	bool new_resources = false;

	if (did_load)
	{
		SYS_ASSERT(wad != this->wad.master.editWad());
		SYS_ASSERT(wad != this->wad.master.gameWad());

		this->wad.master.ReplaceEditWad(wad);

		new_resources = true;
	}
	// ...or does it remove the edit_wad? (e.g. wad == game_wad)
	else if (this->wad.master.editWad() && wad != this->wad.master.editWad())
	{
		this->wad.master.RemoveEditWad();

		new_resources = true;
	}

	gLog.printf("Loading Map : %s of %s\n", map_name.c_str(), wad->PathName().u8string().c_str());

	// TODO: overhaul the interface to select map from the same wad
	LoadLevel(wad.get(), map_name);

	if (new_resources)
	{
		// this can invalidate the 'wad' var (since it closes/reopens
		// all wads in the master_dir), so it MUST be after LoadLevel.
		// less importantly, we need to know the Level_format.
		Main_LoadResources(loaded);
	}
}


void Instance::CMD_GivenFile()
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
		Beep("GivenFile: unknown keyword: %s", mode.c_str());
		return;
	}

	if (index < 0 || index >= (int)global::Pwad_list.size())
	{
		Beep("No more files");
		return;
	}

	last_given_file = index;

	// TODO: remember last map visited in this wad

	OpenFileMap(global::Pwad_list[index], NULL);
}


void Instance::CMD_FlipMap()
{
	SString mode = EXEC_Param[0];

	if (mode.empty())
	{
		Beep("FlipMap: missing keyword");
		return;
	}


	if (! Main_ConfirmQuit("open another map"))
		return;


	const Wad_file *wad = this->wad.master.activeWad().get();

	// the level might not be found (lev_num < 0) -- that is OK
	int lev_idx = wad->LevelFind(loaded.levelName);
	int max_idx = wad->LevelCount() - 1;

	if (max_idx < 0)
	{
		Beep("No maps ?!?");
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
			Beep("No more maps");
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
			Beep("No more maps");
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
		Beep("FlipMap: unknown keyword: %s", mode.c_str());
		return;
	}

	SYS_ASSERT(lev_idx >= 0);
	SYS_ASSERT(lev_idx <= max_idx);


	int lump_idx = wad->LevelHeader(lev_idx);
	Lump_c * lump  = wad->GetLump(lump_idx);
	const SString &map_name = lump->Name();

	gLog.printf("Flipping Map to : %s\n", map_name.c_str());

	LoadLevel(wad, map_name);
}


//------------------------------------------------------------------------
//  SAVING CODE
//------------------------------------------------------------------------

void Instance::SaveHeader(const SString &level)
{
	int size = (int)this->level.headerData.size();

	Lump_c *lump = wad.master.editWad()->AddLevel(level, &saving_level);

	if (size > 0)
	{
		lump->Write(&this->level.headerData[0], size);
	}
}


void Instance::SaveBehavior()
{
	int size = (int)level.behaviorData.size();

	Lump_c &lump = wad.master.editWad()->AddLump("BEHAVIOR");

	if (size > 0)
	{
		lump.Write(&level.behaviorData[0], size);
	}
}


void Instance::SaveScripts()
{
	int size = (int)level.scriptsData.size();

	if (size > 0)
	{
		Lump_c &lump = wad.master.editWad()->AddLump("SCRIPTS");

		lump.Write(&level.scriptsData[0], size);
	}
}


void Instance::SaveVertices()
{
	Lump_c &lump = wad.master.editWad()->AddLump("VERTEXES");

	for (const auto &vert : level.vertices)
	{
		raw_vertex_t raw{};

		raw.x = LE_S16(static_cast<int>(vert->raw_x));
		raw.y = LE_S16(static_cast<int>(vert->raw_y));

		lump.Write(&raw, sizeof(raw));
	}
}


void Instance::SaveSectors()
{
	Lump_c &lump = wad.master.editWad()->AddLump("SECTORS");

	for (const auto& sec : level.sectors)
	{
		raw_sector_t raw{};

		raw.floorh = LE_S16(sec->floorh);
		raw.ceilh  = LE_S16(sec->ceilh);

		W_StoreString(raw.floor_tex, sec->FloorTex(), sizeof(raw.floor_tex));
		W_StoreString(raw.ceil_tex,  sec->CeilTex(),  sizeof(raw.ceil_tex));

		raw.light = LE_U16(sec->light);
		raw.type  = LE_U16(sec->type);
		raw.tag   = LE_U16(sec->tag);

		lump.Write(&raw, sizeof(raw));
	}
}


void Instance::SaveThings()
{
	Lump_c &lump = wad.master.editWad()->AddLump("THINGS");

	for (const auto &th : level.things)
	{
		raw_thing_t raw{};

		raw.x = LE_S16(static_cast<int>(th->raw_x));
		raw.y = LE_S16(static_cast<int>(th->raw_y));

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		lump.Write(&raw, sizeof(raw));
	}
}


// IOANCH 9/2015
void Instance::SaveThings_Hexen()
{
	Lump_c &lump = wad.master.editWad()->AddLump("THINGS");

	for (const auto &th : level.things)
	{
		raw_hexen_thing_t raw{};

		raw.tid = LE_S16(th->tid);

		raw.x = LE_S16(static_cast<int>(th->raw_x));
		raw.y = LE_S16(static_cast<int>(th->raw_y));
		raw.height = LE_S16(static_cast<int>(th->raw_h));

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		raw.special = static_cast<u8_t>(th->special);
		raw.args[0] = static_cast<u8_t>(th->arg1);
		raw.args[1] = static_cast<u8_t>(th->arg2);
		raw.args[2] = static_cast<u8_t>(th->arg3);
		raw.args[3] = static_cast<u8_t>(th->arg4);
		raw.args[4] = static_cast<u8_t>(th->arg5);

		lump.Write(&raw, sizeof(raw));
	}
}


void Instance::SaveSideDefs()
{
	Lump_c &lump = wad.master.editWad()->AddLump("SIDEDEFS");

	for (const auto &side : level.sidedefs)
	{
		raw_sidedef_t raw{};

		raw.x_offset = LE_S16(side->x_offset);
		raw.y_offset = LE_S16(side->y_offset);

		W_StoreString(raw.upper_tex, side->UpperTex(), sizeof(raw.upper_tex));
		W_StoreString(raw.lower_tex, side->LowerTex(), sizeof(raw.lower_tex));
		W_StoreString(raw.mid_tex,   side->MidTex(),   sizeof(raw.mid_tex));

		raw.sector = LE_U16(side->sector);

		lump.Write(&raw, sizeof(raw));
	}
}


void Instance::SaveLineDefs()
{
	Lump_c &lump = wad.master.editWad()->AddLump("LINEDEFS");

	for (const auto &ld : level.linedefs)
	{
		raw_linedef_t raw{};

		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = LE_U16(ld->type);
		raw.tag   = LE_S16(ld->tag);

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump.Write(&raw, sizeof(raw));
	}
}


// IOANCH 9/2015
void Instance::SaveLineDefs_Hexen()
{
	Lump_c &lump = wad.master.editWad()->AddLump("LINEDEFS");

	for (const auto &ld : level.linedefs)
	{
		raw_hexen_linedef_t raw{};

		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = static_cast<u8_t>(ld->type);

		raw.args[0] = static_cast<u8_t>(ld->tag);
		raw.args[1] = static_cast<u8_t>(ld->arg2);
		raw.args[2] = static_cast<u8_t>(ld->arg3);
		raw.args[3] = static_cast<u8_t>(ld->arg4);
		raw.args[4] = static_cast<u8_t>(ld->arg5);

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump.Write(&raw, sizeof(raw));
	}
}


void Instance::EmptyLump(const char *name) const
{
	wad.master.editWad()->AddLump(name);
}


//
// Write out the level data
//

void Instance::SaveLevel(const SString &level)
{
	// set global level name now (for debugging code)
	loaded.levelName = level.asUpper();

	// remove previous version of level (if it exists)
	int lev_num = wad.master.editWad()->LevelFind(level);
	int level_lump = -1;

	if (lev_num >= 0)
	{
		level_lump = wad.master.editWad()->LevelHeader(lev_num);

		wad.master.editWad()->RemoveLevel(lev_num);
	}

	wad.master.editWad()->InsertPoint(level_lump);

	SaveHeader(level);

	if (loaded.levelFormat == MapFormat::udmf)
	{
		UDMF_SaveLevel();
	}
	else
	{
		// IOANCH 9/2015: save Hexen format maps
		if (loaded.levelFormat == MapFormat::hexen)
		{
			SaveThings_Hexen();
			SaveLineDefs_Hexen();
		}
		else
		{
			SaveThings();
			SaveLineDefs();
		}

		SaveSideDefs();
		SaveVertices();

		EmptyLump("SEGS");
		EmptyLump("SSECTORS");
		EmptyLump("NODES");

		SaveSectors();

		EmptyLump("REJECT");
		EmptyLump("BLOCKMAP");

		if (loaded.levelFormat == MapFormat::hexen)
		{
			SaveBehavior();
			SaveScripts();
		}
	}

	// write out the new directory
	wad.master.editWad()->writeToDisk();


	// build the nodes
	if (config::bsp_on_save && ! inhibit_node_build)
	{
		BuildNodesAfterSave(saving_level);
	}


	// this is mainly for Next/Prev-map commands
	// [ it doesn't change the on-disk wad file at all ]
	wad.master.editWad()->SortLevels();

	loaded.writeEurekaLump(*wad.master.editWad().get());
	wad.master.editWad()->writeToDisk();

	global::recent.addRecent(wad.master.editWad()->PathName(), loaded.levelName, global::home_dir);

	Status_Set("Saved %s", loaded.levelName.c_str());

	if (main_win)
	{
		main_win->SetTitle(wad.master.editWad()->PathName().u8string(), loaded.levelName, false);

		// save the user state associated with this map
		M_SaveUserState();
	}

	MadeChanges = false;
}

// these return false if user cancelled
bool Instance::M_SaveMap()
{
	// we require a wad file to save into.
	// if there is none, then need to create one via Export function.

	if (!wad.master.editWad())
	{
		return M_ExportMap();
	}

	if (wad.master.editWad()->IsReadOnly())
	{
		if (DLG_Confirm({ "Cancel", "&Export" },
		                "The current pwad is a READ-ONLY file. "
						"Do you want to export this map into a new file?") <= 0)
		{
			return false;
		}

		return M_ExportMap();
	}


	M_BackupWad(wad.master.editWad().get());

	gLog.printf("Saving Map : %s in %s\n", loaded.levelName.c_str(), wad.master.editWad()->PathName().u8string().c_str());

	SaveLevel(loaded.levelName);

	return true;
}


bool Instance::M_ExportMap()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to export to");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("Wads\t*.wad");
	chooser.directory(Main_FileOpFolder().u8string().c_str());

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			gLog.printf("Export Map: error choosing file:\n");
			gLog.printf("   %s\n", chooser.errmsg());

			DLG_Notify("Unable to export the map:\n\n%s", chooser.errmsg());
			return false;

		case 1:
			gLog.printf("Export Map: cancelled by user\n");
			return false;

		default:
			break;  // OK
	}

	// if extension is missing then add ".wad"
	fs::path filename = fs::u8path(chooser.filename());

	fs::path extension = filename.extension();
	if(extension.empty())
		filename = fs::u8path(filename.u8string() + ".wad");

	// don't export into a file we currently have open
	if (wad.master.MasterDir_HaveFilename(filename.u8string()))
	{
		DLG_Notify("Unable to export the map:\n\nFile already in use");
		return false;
	}


	// does the file already exist?  if not, create it...
	bool exists = FileExists(filename);

	std::shared_ptr<Wad_file> wad;

	if (exists)
	{
		wad = Wad_file::Open(filename, WadOpenMode::append);

		if (wad && wad->IsReadOnly())
		{
			DLG_Notify("Cannot export the map into a READ-ONLY file.");

			return false;
		}

		// adopt iwad/port/resources of the target wad
		if (wad->FindLump(EUREKA_LUMP))
		{
			if (! loaded.parseEurekaLump(global::home_dir, global::install_dir, global::recent, wad.get()))
				return false;
		}
	}
	else
	{
		wad = Wad_file::Open(filename, WadOpenMode::write);
	}

	if (! wad)
	{
		DLG_Notify("Unable to export the map:\n\n%s",
		           "Error creating output file");
		return false;
	}


	// ask user for map name

	SString map_name;
	{
		auto dialog = std::make_unique<UI_ChooseMap>(loaded.levelName.c_str());

		dialog->PopulateButtons(static_cast<char>(toupper(loaded.levelName[0])),
								wad.get());

		map_name = dialog->Run();
	}

	// cancelled?
	if (map_name.empty())
	{
		return false;
	}


	// we will write into the chosen wad.
	// however if the level already exists, get confirmation first

	if (exists && wad->LevelFind(map_name) >= 0)
	{
		if (DLG_Confirm({ "Cancel", "&Overwrite" },
		                overwrite_message, "selected") <= 0)
		{
			return false;
		}
	}

	// back-up an existing wad
	if (exists)
	{
		M_BackupWad(wad.get());
	}


	gLog.printf("Exporting Map : %s in %s\n", map_name.c_str(), wad->PathName().u8string().c_str());

	// the new wad replaces the current PWAD
	this->wad.master.ReplaceEditWad(wad);

	SaveLevel(map_name);

	// do this after the save (in case it fatal errors)
	Main_LoadResources(loaded);

	return true;
}


void Instance::CMD_SaveMap()
{
	try
	{
		M_SaveMap();
	}
	catch(const WadWriteException &e)
	{
		DLG_ShowError(false, "%s", e.what());
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not save map: %s", e.what());
	}
}


void Instance::CMD_ExportMap()
{
	try
	{
		M_ExportMap();
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not export map: %s", e.what());
	}
}


//------------------------------------------------------------------------
//  COPY, RENAME and DELETE MAP
//------------------------------------------------------------------------

void Instance::CMD_CopyMap()
{
	try
	{
		if (!wad.master.editWad())
		{
			DLG_Notify("Cannot copy a map unless editing a PWAD.");
			return;
		}

		if (wad.master.editWad()->IsReadOnly())
		{
			DLG_Notify("Cannot copy map : file is read-only.");
			return;
		}

		// ask user for map name

		SString new_name;
		{
			auto dialog = std::make_unique<UI_ChooseMap>(loaded.levelName.c_str(),
				wad.master.editWad());

			dialog->PopulateButtons(static_cast<char>(toupper(loaded.levelName[0])), wad.master.editWad().get());

			new_name = dialog->Run();
		}

		// cancelled?
		if (new_name.empty())
			return;

		// sanity check that the name is different
		// (should be prevented by the choose-map dialog)
		if (y_stricmp(new_name.c_str(), loaded.levelName.c_str()) == 0)
		{
			Beep("Name is same!?!");
			return;
		}

		// perform the copy (just a save)
		gLog.printf("Copying Map : %s --> %s\n", loaded.levelName.c_str(), new_name.c_str());

		SaveLevel(new_name);

		Status_Set("Copied to %s", loaded.levelName.c_str());
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not copy map: %s", e.what());
	}
	
}


void Instance::CMD_RenameMap()
{
	try
	{
		if(!wad.master.gameWad())
		{
			gLog.printf("No IWAD file!\n");
			return;
		}
		if (!wad.master.editWad())
		{
			DLG_Notify("Cannot rename a map unless editing a PWAD.");
			return;
		}

		if (wad.master.editWad()->IsReadOnly())
		{
			DLG_Notify("Cannot rename map : file is read-only.");
			return;
		}


		// ask user for map name
		SString new_name;
		{
			auto dialog = std::make_unique<UI_ChooseMap>(loaded.levelName.c_str(),
				wad.master.editWad() /* rename_wad */);

			// pick level format from the IWAD
			// [ user may be trying to rename map after changing the IWAD ]
			char format = 'M';
			{
				int idx = wad.master.gameWad()->LevelFindFirst();

				if (idx >= 0)
				{
					idx = wad.master.gameWad()->LevelHeader(idx);
					const SString& name = wad.master.gameWad()->GetLump(idx)->Name();
					format = static_cast<char>(toupper(name[0]));
				}
			}

			dialog->PopulateButtons(format, wad.master.editWad().get());

			new_name = dialog->Run();
		}

		// cancelled?
		if (new_name.empty())
			return;

		// sanity check that the name is different
		// (should be prevented by the choose-map dialog)
		if (y_stricmp(new_name.c_str(), loaded.levelName.c_str()) == 0)
		{
			Beep("Name is same!?!");
			return;
		}


		// perform the rename
		int lev_num = wad.master.editWad()->LevelFind(loaded.levelName);

		if (lev_num >= 0)
		{
			int level_lump = wad.master.editWad()->LevelHeader(lev_num);

			wad.master.editWad()->RenameLump(level_lump, new_name.c_str());
			wad.master.editWad()->writeToDisk();
		}

		loaded.levelName = new_name.asUpper();

		main_win->SetTitle(wad.master.editWad()->PathName().u8string(), loaded.levelName, false);

		Status_Set("Renamed to %s", loaded.levelName.c_str());
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not rename map: %s", e.what());
	}
	
}


void Instance::CMD_DeleteMap()
{
	try
	{
		if (!wad.master.editWad())
		{
			DLG_Notify("Cannot delete a map unless editing a PWAD.");
			return;
		}

		if (wad.master.editWad()->IsReadOnly())
		{
			DLG_Notify("Cannot delete map : file is read-only.");
			return;
		}

		if (wad.master.editWad()->LevelCount() < 2)
		{
			// perhaps ask either to Rename map, or Delete the file (and Eureka will shut down)

			DLG_Notify("Cannot delete the last map in a PWAD.");
			return;
		}

		if (DLG_Confirm({ "Cancel", "&Delete" },
			"Are you sure you want to delete this map? "
			"It will be permanently removed from the current PWAD.") <= 0)
		{
			return;
		}

		gLog.printf("Deleting Map : %s...\n", loaded.levelName.c_str());

		int lev_num = wad.master.editWad()->LevelFind(loaded.levelName);

		if (lev_num < 0)
		{
			Beep("No such map ?!?");
			return;
		}


		// kick it to the curb
		wad.master.editWad()->RemoveLevel(lev_num);
		wad.master.editWad()->writeToDisk();


		// choose a new level to load
		{
			if (lev_num >= wad.master.editWad()->LevelCount())
				lev_num = wad.master.editWad()->LevelCount() - 1;

			int lump_idx = wad.master.editWad()->LevelHeader(lev_num);
			Lump_c* lump = wad.master.editWad()->GetLump(lump_idx);
			const SString& map_name = lump->Name();

			gLog.printf("OK.  Loading : %s....\n", map_name.c_str());

			// TODO: overhaul the interface to NOT go back to the IWAD
			LoadLevel(wad.master.editWad().get(), map_name);
		}
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Cannot delete map: %s", e.what());
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
