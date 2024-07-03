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
#include "m_testmap.h"
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

static Document makeFreshDocument(Instance &inst, const ConfigData &config, MapFormat levelFormat)
{
	Document doc(inst);
	auto sec = std::make_unique<Sector>();

	sec->SetDefaults(config);
	doc.sectors.push_back(std::move(sec));

	for (int i = 0 ; i < 4 ; i++)
	{
		auto v = std::make_shared<Vertex>();

		v->SetRawX(levelFormat, (i >= 2) ? 256 : -256);
		v->SetRawY(levelFormat, (i==1 || i==2) ? 256 :-256);
		doc.vertices.push_back(std::move(v));

		auto sd = std::make_shared<SideDef>();
		sd->SetDefaults(config, false);
		doc.sidedefs.push_back(std::move(sd));

		auto ld = std::make_shared<LineDef>();
		ld->start = i;
		ld->end   = (i+1) % 4;
		ld->flags = MLF_Blocking;
		ld->right = i;
		doc.linedefs.push_back(std::move(ld));
	}

	for (int pl = 1 ; pl <= 4 ; pl++)
	{
		auto th = std::make_unique<Thing>();

		th->type  = pl;
		th->angle = 90;

		th->SetRawX(levelFormat, (pl == 1) ? 0 : (pl - 3) * 48);
		th->SetRawY(levelFormat, (pl == 1) ? 48 : (pl == 3) ? -48 : 0);
		doc.things.push_back(std::move(th));
	}

	doc.CalculateLevelBounds();
	
	return doc;
}

void Instance::FreshLevel()
{
	level.clear();
	level = makeFreshDocument(*this, conf, loaded.levelFormat);

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

static void updateLoading(const UI_ProjectSetup::Result &result, LoadingData &loading)
{
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
}

void Instance::Project_ApplyChanges(const UI_ProjectSetup::Result &result) noexcept(false)
{
	LoadingData loading = loaded;
	updateLoading(result, loading);
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
	NewResources newres{};
	ConfigData backupConfig = conf;
	LoadingData backupLoading = loaded;
	WadData backupWadData = wad;
	tl::optional<Document> backupDoc;
	try
	{
		if (!level.Main_ConfirmQuit("create a new project"))
			return;
		
		/* first, ask for the output file */
		
		tl::optional<fs::path> filename = Project_AskFile();
		
		if (!filename)
			return;
		
		if(global::recent.hasIwadByPath(*filename))
		{
			DLG_Notify("Cannot overwrite a game IWAD: %s", filename.value().u8string().c_str());
			return;
		}
		
		
		/* second, query what Game, Port and Resources to use */
		// TODO: new instance
		UI_ProjectSetup dialog(*this, true /* new_project */, false /* is_startup */);
		
		tl::optional<UI_ProjectSetup::Result> result = dialog.Run();
		
		if (!result)
		{
			return;
		}
		
		if(FileExists(*filename))
		{
			if(!DLG_Confirm({"Cancel", "&Overwrite"}, "Are you sure you want to overwrite %s with a new project?", filename.value().u8string().c_str()))
			{
				return;
			}
		}

		// Backup existing file before overwriting
		try
		{
			std::shared_ptr<Wad_file> oldFile = Wad_file::Open(*filename, WadOpenMode::read);
			if(oldFile)
			{
				if(oldFile->IsIWAD())
				{
					DLG_Notify("Overwriting game IWAD files is not allowed: %s", filename->u8string().c_str());
					return;
				}
				M_BackupWad(oldFile.get());
			}
		}
		catch(const std::runtime_error &e)
		{
			gLog.printf("Error reading old WAD %s: %s\n", filename->u8string().c_str(), e.what());
		}

		
		LoadingData loading = loaded;
		updateLoading(*result, loading);
		newres = loadResources(loading, wad);
		
		
		// determine map name (same as first level in the IWAD)
		SString map_name = "MAP01";
		
		int idx = newres.waddata.master.gameWad()->LevelFindFirst();
		
		if (idx >= 0)
		{
			idx = newres.waddata.master.gameWad()->LevelHeader(idx);
			map_name = newres.waddata.master.gameWad()->GetLump(idx)->Name();
		}
		
		gLog.printf("Creating New File : %s in %s\n", map_name.c_str(), filename->u8string().c_str());
		
		
		std::shared_ptr<Wad_file> wad = Wad_file::Open(*filename, WadOpenMode::write);
		
		if (!wad)
		{
			DLG_Notify("Unable to create the new WAD file.");
			return;
		}

		backupDoc = std::move(level);
		level = makeFreshDocument(*this, newres.config, newres.loading.levelFormat);
		conf = std::move(newres.config);
		loaded = std::move(newres.loading);
		if(main_win)
			testmap::updateMenuName(main_win->menu_bar, loaded);
		
		this->wad = std::move(newres.waddata);
		
		SaveLevel(loaded, map_name, *wad, false);
		this->wad.master.ReplaceEditWad(wad);
		ConfirmLevelSaveSuccess(loaded, *wad);
		UpdateViewOnResources();
		
		RedrawMap();
	}
	catch(const std::runtime_error &e)
	{
		conf = std::move(backupConfig);
		loaded = std::move(backupLoading);
		wad = std::move(backupWadData);
		if(backupDoc)
			level = std::move(backupDoc.value());
		if(main_win)
			testmap::updateMenuName(main_win->menu_bar, loaded);
		
		DLG_ShowError(false, "Could not create new project: %s", e.what());
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
		
		if(main_win)
			testmap::updateMenuName(main_win->menu_bar, loaded);
	}

	return result.has_value();
}


void Instance::CMD_FreshMap()
{
	tl::optional<Document> backupDoc;
	try
	{
		if (!wad.master.editWad())
		{
			DLG_Notify("Cannot create a fresh map unless editing a PWAD.");
			return;
		}

		if (wad.master.editWad()->IsReadOnly())
		{
			DLG_Notify("Cannot create a fresh map: file is read-only.");
			return;
		}

		if (!level.Main_ConfirmQuit("create a fresh map"))
			return;

		SString map_name;
		{
			UI_ChooseMap dialog(loaded.levelName.c_str());
			
			dialog.PopulateButtons(static_cast<char>(toupper(loaded.levelName[0])), wad.master.editWad().get());
			
			map_name = dialog.Run();
		}

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
		backupDoc = std::move(level);
		FreshLevel();

		// save it now : sets Level_name and window title
		SaveLevelAndUpdateWindow(loaded, map_name, *wad.master.editWad(), false);
	}
	catch (const std::runtime_error& e)
	{
		if(backupDoc)
			level = std::move(*backupDoc);
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

		auto sec = std::make_shared<Sector>();

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


void Document::CreateFallbackSector(const ConfigData &config)
{
	gLog.printf("Creating a fallback sector.\n");

	auto sec = std::make_shared<Sector>();

	sec->SetDefaults(config);

	sectors.push_back(std::move(sec));
}

void Document::CreateFallbackSideDef(const ConfigData &config)
{
	// we need a valid sector too!
	if (numSectors() == 0)
		CreateFallbackSector(config);

	gLog.printf("Creating a fallback sidedef.\n");

	auto sd = std::make_shared<SideDef>();

	sd->SetDefaults(config, false);

	sidedefs.push_back(std::move(sd));
}

void Document::CreateFallbackVertices()
{
	gLog.printf("Creating two fallback vertices.\n");

	auto v1 = std::make_unique<Vertex>();
	auto v2 = std::make_unique<Vertex>();

	v1->raw_x = FFixedPoint(-777);
	v1->raw_y = FFixedPoint(-777);

	v2->raw_x = FFixedPoint(555);
	v2->raw_y = FFixedPoint(555);

	vertices.push_back(std::move(v1));
	vertices.push_back(std::move(v2));
}

void Document::ValidateSidedefRefs(LineDef & ld, int num, const ConfigData &config, BadCount &bad)
{
	if (ld.right >= numSidedefs() || ld.left >= numSidedefs())
	{
		gLog.printf("WARNING: linedef #%d has invalid sidedefs (%d / %d)\n",
				  num, ld.right, ld.left);

		bad.sidedef_refs++;

		// ensure we have a usable sidedef
		if (numSidedefs() == 0)
			CreateFallbackSideDef(config);

		if (ld.right >= numSidedefs())
			ld.right = 0;

		if (ld.left >= numSidedefs())
			ld.left = 0;
	}
}

void Document::ValidateVertexRefs(LineDef &ld, int num, BadCount &bad)
{
	if (ld.start >= numVertices() || ld.end >= numVertices() ||
	    ld.start == ld.end)
	{
		gLog.printf("WARNING: linedef #%d has invalid vertices (%d -> %d)\n",
		          num, ld.start, ld.end);

		bad.linedef_count++;

		// ensure we have a valid vertex
		if (numVertices() < 2)
			CreateFallbackVertices();

		ld.start = 0;
		ld.end   = numVertices() - 1;
	}
}

void Document::ValidateSectorRef(SideDef &sd, int num, const ConfigData &config, BadCount &bad)
{
	if (sd.sector >= numSectors())
	{
		gLog.printf("WARNING: sidedef #%d has invalid sector (%d)\n",
		          num, sd.sector);

		bad.sector_refs++;

		// ensure we have a valid sector
		if (numSectors() == 0)
			CreateFallbackSector(config);

		sd.sector = 0;
	}
}


void Document::LoadHeader(int loading_level, const Wad_file &load_wad)
{
	const Lump_c *lump = load_wad.GetLump(load_wad.LevelHeader(loading_level));
	headerData = lump->getData();
}


void Document::LoadBehavior(int loading_level, const Wad_file *load_wad)
{
	// IOANCH 9/2015: support Hexen maps
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "BEHAVIOR");
	if (! lump)
		ThrowException("No BEHAVIOR lump!\n");
	
	behaviorData = lump->getData();
}


void Document::LoadScripts(int loading_level, const Wad_file *load_wad)
{
	// the SCRIPTS lump is usually absent
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "SCRIPTS");
	if (! lump)
		return;
	
	scriptsData = lump->getData();
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


void Document::LoadSideDefs(int loading_level, const Wad_file *load_wad, const ConfigData &config, BadCount &bad)
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

		auto sd = std::make_shared<SideDef>();

		sd->x_offset = LE_S16(raw.x_offset);
		sd->y_offset = LE_S16(raw.y_offset);

		UpperCaseShortStr(raw.upper_tex, 8);
		UpperCaseShortStr(raw.lower_tex, 8);
		UpperCaseShortStr(raw.  mid_tex, 8);

		sd->upper_tex = BA_InternaliseString(SString(raw.upper_tex, 8));
		sd->lower_tex = BA_InternaliseString(SString(raw.lower_tex, 8));
		sd->  mid_tex = BA_InternaliseString(SString(raw.  mid_tex, 8));

		sd->sector = LE_U16(raw.sector);

		ValidateSectorRef(*sd, i, config, bad);

		sidedefs.push_back(std::move(sd));
	}
}


void Document::LoadLineDefs(int loading_level, const Wad_file *load_wad, const ConfigData &config, BadCount &bad)
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

		auto ld = std::make_shared<LineDef>();

		ld->start = LE_U16(raw.start);
		ld->end   = LE_U16(raw.end);

		ld->flags = LE_U16(raw.flags);
		ld->type  = LE_U16(raw.type);
		ld->tag   = LE_S16(raw.tag);

		ld->right = LE_U16(raw.right);
		ld->left  = LE_U16(raw.left);

		if (ld->right == 0xFFFF) ld->right = -1;
		if (ld-> left == 0xFFFF) ld-> left = -1;

		ValidateVertexRefs(*ld, i, bad);
		ValidateSidedefRefs(*ld, i, config, bad);

		linedefs.push_back(std::move(ld));
	}
}


// IOANCH 9/2015
void Document::LoadLineDefs_Hexen(int loading_level, const Wad_file *load_wad, const ConfigData &config, BadCount &bad)
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

		auto ld = std::make_shared<LineDef>();

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

		ValidateVertexRefs(*ld, i, bad);
		ValidateSidedefRefs(*ld, i, config, bad);

		linedefs.push_back(std::move(ld));
	}
}


void Document::RemoveUnusedVerticesAtEnd()
{
	if (numVertices() == 0)
		return;

	bitvec_c used_verts(numVertices());

	for (const auto &linedef : linedefs)
	{
		used_verts.set(linedef->start);
		used_verts.set(linedef->end);
	}

	int new_count = numVertices();

	while (new_count > 2 && !used_verts.get(new_count-1))
		new_count--;

	// we directly modify the vertex array here (which is not
	// normally kosher, but level loading is a special case).
	if (new_count < numVertices())
	{
		gLog.printf("Removing %d unused vertices at end\n", numVertices() - new_count);

		vertices.resize(new_count);
	}
}


static void ShowLoadProblem(const BadCount &bad)
{
	gLog.printf("Map load problems:\n");
	gLog.printf("   %d linedefs with bad vertex refs\n", bad.linedef_count);
	gLog.printf("   %d linedefs with bad sidedef refs\n", bad.sidedef_refs);
	gLog.printf("   %d sidedefs with bad sector refs\n", bad.sector_refs);

	SString message;

	if (bad.linedef_count > 0)
	{
		message = SString::printf("Found %d linedefs with bad vertex references.\n"
			"These references have been replaced.",
			bad.linedef_count);
	}
	else
	{
		message = SString::printf("Found %d bad sector refs, %d bad sidedef refs.\n"
			"These references have been replaced.",
			bad.sector_refs, bad.sidedef_refs);
	}

	DLG_Notify("Map validation report:\n\n%s", message.c_str());
}

//
// Read in the level data
//

void Instance::LoadLevel(const Wad_file *wad, const SString &level) noexcept(false)
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

	if (main_win)
	{
		main_win->UpdateTotals(this->level);
		main_win->UpdateGameInfo(loaded, conf);
		main_win->InvalidatePanelObj();
		main_win->redraw();

		main_win->SetTitle(wad->PathName().u8string(), level, wad->IsReadOnly());

		// load the user state associated with this map
		if (! M_LoadUserState())
		{
			M_DefaultUserState();
		}
	}

	loaded.levelName = level.asUpper();

	Status_Set("Loaded %s", loaded.levelName.c_str());

	RedrawMap();
}

NewDocument Instance::openDocument(const LoadingData &inLoading, const Wad_file &wad, int level)
{
	assert(level >= 0 && level < wad.LevelCount());

	NewDocument newdoc = { Document(*this), inLoading, BadCount() };

	Document& doc = newdoc.doc;
	LoadingData& loading = newdoc.loading;
	BadCount& bad = newdoc.bad;
	
	loading.levelFormat = wad.LevelFormat(level);
	doc.LoadHeader(level, wad);
	if(loading.levelFormat == MapFormat::udmf)
	{
		UDMF_LoadLevel(level, &wad, doc, loading, bad);
	}
	else try
	{
		if(loading.levelFormat == MapFormat::hexen)
			doc.LoadThings_Hexen(level, &wad);
		else
			doc.LoadThings(level, &wad);
		doc.LoadVertices(level, &wad);
		doc.LoadSectors(level, &wad);
		doc.LoadSideDefs(level, &wad, conf, bad);
		if(loading.levelFormat == MapFormat::hexen)
		{
			doc.LoadLineDefs_Hexen(level, &wad, conf, bad);
			doc.LoadBehavior(level, &wad);
			doc.LoadScripts(level, &wad);
		}
		else
			doc.LoadLineDefs(level, &wad, conf, bad);
	}
	catch(const std::runtime_error &e)
	{
		gLog.printf("%s\n", e.what());
		throw;
	}
	doc.RemoveUnusedVerticesAtEnd();
	doc.checks.sidedefsUnpack(true);
	doc.CalculateLevelBounds();
	doc.MadeChanges = false;
	
	return newdoc;
}

void Instance::LoadLevelNum(const Wad_file *wad, int lev_num) noexcept(false)
{
	NewDocument newdoc = openDocument(loaded, *wad, lev_num);
	if (newdoc.bad.linedef_count || newdoc.bad.sector_refs || newdoc.bad.sidedef_refs)
	{
		ShowLoadProblem(newdoc.bad);
	}
	loaded = newdoc.loading;
	level = std::move(newdoc.doc);
	if(main_win)
		testmap::updateMenuName(main_win->menu_bar, loaded);
	Subdiv_InvalidateAll();
}

void Instance::refreshViewAfterLoad(const BadCount& bad, const Wad_file *wad, const SString &map_name, bool new_resources)
{
	if (bad.exists())
		ShowLoadProblem(bad);

	Subdiv_InvalidateAll();

	// reset various editor state
	Editor_ClearAction();
	Selection_InvalidateLast();
	edit.Selected->clear_all();
	edit.highlight.clear();

	if (main_win)
	{
		main_win->UpdateTotals(level);
		main_win->UpdateGameInfo(loaded, conf);
		main_win->InvalidatePanelObj();
		main_win->redraw();

		main_win->SetTitle(wad->PathName().u8string(), map_name, wad->IsReadOnly());

		// load the user state associated with this map
		if (!M_LoadUserState())
			M_DefaultUserState();
	}
	loaded.levelName = map_name.asUpper();
	Status_Set("Loaded %s", loaded.levelName.c_str());
	RedrawMap();

	if (new_resources && main_win)
	{
		if (main_win->canvas)
			main_win->canvas->DeleteContext();

		main_win->browser->Populate();

		// TODO: only call this when the IWAD has changed
		main_win->propsLoadValues();
	}
}


//
// open a new wad file.
// when 'map_name' is not NULL, try to open that map.
//
void OpenFileMap(const fs::path &filename, const SString &map_namem) noexcept(false)
{
	// TODO: change this to start a new instance
	SString map_name = map_namem;
	if (! gInstance->level.Main_ConfirmQuit("open another map"))
		return;


	std::shared_ptr<Wad_file> wad;

	// make sure file exists, as Open() with 'a' would create it otherwise
	if (FileExists(filename))
	{
		wad = Wad_file::loadFromFile(filename);
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

	LoadingData loading = gInstance->loaded;

	if (wad->FindLump(EUREKA_LUMP))
	{
		if (! loading.parseEurekaLump(global::home_dir, global::install_dir, global::recent, wad.get()))
		{
			return;
		}
	}


	/* OK, open it */


	// this wad replaces the current PWAD

	// always grab map_name from the actual level
	{
		int idx = wad->LevelHeader(lev_num);
		map_name  = wad->GetLump(idx)->Name();
	}

	gLog.printf("Loading Map : %s of %s\n", map_name.c_str(), wad->PathName().u8string().c_str());

	// These 2 may throw, but it's safe here
	NewDocument newdoc = gInstance->openDocument(loading, *wad, lev_num);
	NewResources newres = loadResources(newdoc.loading, gInstance->wad);

	gInstance->level = std::move(newdoc.doc);
	gInstance->conf = std::move(newres.config);
	gInstance->loaded = std::move(newres.loading);
	gInstance->wad = std::move(newres.waddata);
	gInstance->wad.master.ReplaceEditWad(wad);
	
	if(gInstance->main_win)
		testmap::updateMenuName(gInstance->main_win->menu_bar, gInstance->loaded);

	gInstance->refreshViewAfterLoad(newdoc.bad, wad.get(), map_name, true);
}


void Instance::CMD_OpenMap()
{
	if (!level.Main_ConfirmQuit("open another map"))
		return;

	SString map_name;
	bool did_load = false;

	std::shared_ptr<Wad_file> wad = UI_OpenMap(*this).Run(&map_name, &did_load);

	if (! wad)	// cancelled
		return;

	// this shouldn't happen -- but just in case...
	int lev_num = wad->LevelFind(map_name);
	if (lev_num < 0)
	{
		DLG_Notify("Hmmmm, cannot find that map !?!");

		return;
	}

	LoadingData loading = loaded;
	if (did_load && wad->FindLump(EUREKA_LUMP) && !loading.parseEurekaLump(global::home_dir,
		global::install_dir, global::recent, wad.get()))
	{
		return;
	}

	// does this wad replace the currently edited wad?
	bool new_resources = false;
	std::shared_ptr<Wad_file> newEditWad;
	bool removeEditWad = false;

	if (did_load)
	{
		SYS_ASSERT(wad != this->wad.master.editWad());
		SYS_ASSERT(wad != this->wad.master.gameWad());

		newEditWad = wad;

		new_resources = true;
	}
	// ...or does it remove the edit_wad? (e.g. wad == game_wad)
	else if (this->wad.master.editWad() && wad != this->wad.master.editWad())
	{
		removeEditWad = true;
		new_resources = true;
	}

	gLog.printf("Loading Map : %s of %s\n", map_name.c_str(), wad->PathName().u8string().c_str());

	NewDocument newdoc = { Document(*this), LoadingData(), BadCount() };
	try
	{
		newdoc = openDocument(loading, *wad, lev_num);
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not open %s of %s. %s", map_name.c_str(), 
			wad->PathName().u8string().c_str(), e.what());
		return;
	}

	
	if (new_resources)
	{
		// TODO: call a safe version of Main_LoadResources
		NewResources newres = {};
		try
		{
			newres = loadResources(newdoc.loading, this->wad);
		}
		catch (const std::runtime_error& e)
		{
			DLG_ShowError(false, "Could not reload resources: %s", e.what());
			return;
		}

		// success loading resources
		conf = std::move(newres.config);
		loaded = std::move(newres.loading);
		this->wad = std::move(newres.waddata);
		
		if(main_win)
			testmap::updateMenuName(main_win->menu_bar, loaded);

		gLog.printf("--- DONE ---\n");
		gLog.printf("\n");
	}

	// on success

	if (removeEditWad)
		this->wad.master.RemoveEditWad();
	else if (newEditWad)
		this->wad.master.ReplaceEditWad(newEditWad);

	level = std::move(newdoc.doc);
	if(!new_resources)	// we already updated loaded with resources
		loaded = std::move(newdoc.loading);
	if(main_win)
		testmap::updateMenuName(main_win->menu_bar, loaded);

	refreshViewAfterLoad(newdoc.bad, wad.get(), map_name, new_resources);
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

	try
	{
		OpenFileMap(global::Pwad_list[index], NULL);
	}
	catch (const std::runtime_error& e)
	{
		gLog.printf("%s\n", e.what());
		DLG_ShowError(false, "Cannot load given file %s: %s", global::Pwad_list[index].u8string().c_str(), e.what());
	}
}


void Instance::CMD_FlipMap()
{
	SString mode = EXEC_Param[0];

	if (mode.empty())
	{
		Beep("FlipMap: missing keyword");
		return;
	}


	if (!level.Main_ConfirmQuit("open another map"))
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

int Document::SaveHeader(Wad_file& wad, const SString &level) const
{
	int size = (int)headerData.size();

	int saving_level;
	Lump_c *lump = wad.AddLevel(level, &saving_level);

	if (size > 0)
	{
		lump->Write(&headerData[0], size);
	}

	return saving_level;
}


void Document::SaveBehavior(Wad_file &wad) const
{
	int size = (int)behaviorData.size();

	Lump_c &lump = wad.AddLump("BEHAVIOR");

	if (size > 0)
	{
		lump.Write(&behaviorData[0], size);
	}
}


void Document::SaveScripts(Wad_file& wad) const
{
	int size = (int)scriptsData.size();

	if (size > 0)
	{
		Lump_c &lump = wad.AddLump("SCRIPTS");

		lump.Write(&scriptsData[0], size);
	}
}


void Document::SaveVertices(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("VERTEXES");

	for (const auto &vert : vertices)
	{
		raw_vertex_t raw{};

		raw.x = LE_S16(static_cast<int>(vert->raw_x));
		raw.y = LE_S16(static_cast<int>(vert->raw_y));

		lump.Write(&raw, sizeof(raw));
	}
}


void Document::SaveSectors(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("SECTORS");

	for (const auto& sec : sectors)
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


void Document::SaveThings(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("THINGS");

	for (const auto &th : things)
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
void Document::SaveThings_Hexen(Wad_file& wad) const
{
	Lump_c &lump = wad.AddLump("THINGS");

	for (const auto &th : things)
	{
		raw_hexen_thing_t raw{};

		raw.tid = LE_S16(th->tid);

		raw.x = LE_S16(static_cast<int>(th->raw_x));
		raw.y = LE_S16(static_cast<int>(th->raw_y));
		raw.height = LE_S16(static_cast<int>(th->raw_h));

		raw.angle   = LE_U16(th->angle);
		raw.type    = LE_U16(th->type);
		raw.options = LE_U16(th->options);

		raw.special = static_cast<uint8_t>(th->special);
		raw.args[0] = static_cast<uint8_t>(th->arg1);
		raw.args[1] = static_cast<uint8_t>(th->arg2);
		raw.args[2] = static_cast<uint8_t>(th->arg3);
		raw.args[3] = static_cast<uint8_t>(th->arg4);
		raw.args[4] = static_cast<uint8_t>(th->arg5);

		lump.Write(&raw, sizeof(raw));
	}
}


void Document::SaveSideDefs(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("SIDEDEFS");

	for (const auto &side : sidedefs)
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


void Document::SaveLineDefs(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("LINEDEFS");

	for (const auto &ld : linedefs)
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
void Document::SaveLineDefs_Hexen(Wad_file &wad) const
{
	Lump_c &lump = wad.AddLump("LINEDEFS");

	for (const auto &ld : linedefs)
	{
		raw_hexen_linedef_t raw{};

		raw.start = LE_U16(ld->start);
		raw.end   = LE_U16(ld->end);

		raw.flags = LE_U16(ld->flags);
		raw.type  = static_cast<uint8_t>(ld->type);

		raw.args[0] = static_cast<uint8_t>(ld->tag);
		raw.args[1] = static_cast<uint8_t>(ld->arg2);
		raw.args[2] = static_cast<uint8_t>(ld->arg3);
		raw.args[3] = static_cast<uint8_t>(ld->arg4);
		raw.args[4] = static_cast<uint8_t>(ld->arg5);

		raw.right = (ld->right >= 0) ? LE_U16(ld->right) : 0xFFFF;
		raw.left  = (ld->left  >= 0) ? LE_U16(ld->left)  : 0xFFFF;

		lump.Write(&raw, sizeof(raw));
	}
}


static void EmptyLump(Wad_file& wad, const char *name)
{
	wad.AddLump(name);
}

void Instance::SaveLevel(LoadingData& loading, const SString &level, Wad_file &wad, bool inhibit_node_build)
{
	// set global level name now (for debugging code)
	loading.levelName = level.asUpper();

	// remove previous version of level (if it exists)
	int lev_num = wad.LevelFind(level);
	int level_lump = -1;

	if (lev_num >= 0)
	{
		level_lump = wad.LevelHeader(lev_num);

		wad.RemoveLevel(lev_num);
	}

	wad.InsertPoint(level_lump);

	int saving_level = this->level.SaveHeader(wad, level);

	if (loading.levelFormat == MapFormat::udmf)
	{
		UDMF_SaveLevel(loading, wad);
	}
	else
	{
		// IOANCH 9/2015: save Hexen format maps
		if (loading.levelFormat == MapFormat::hexen)
		{
			this->level.SaveThings_Hexen(wad);
			this->level.SaveLineDefs_Hexen(wad);
		}
		else
		{
			this->level.SaveThings(wad);
			this->level.SaveLineDefs(wad);
		}

		this->level.SaveSideDefs(wad);
		this->level.SaveVertices(wad);

		EmptyLump(wad, "SEGS");
		EmptyLump(wad, "SSECTORS");
		EmptyLump(wad, "NODES");

		this->level.SaveSectors(wad);

		EmptyLump(wad, "REJECT");
		EmptyLump(wad, "BLOCKMAP");

		if (loading.levelFormat == MapFormat::hexen)
		{
			this->level.SaveBehavior(wad);
			this->level.SaveScripts(wad);
		}
	}

	// build the nodes
	if (config::bsp_on_save && ! inhibit_node_build)
	{
		BuildNodesAfterSave(saving_level, loading, wad);
	}

	// this is mainly for Next/Prev-map commands
	// [ it doesn't change the on-disk wad file at all ]
	wad.SortLevels();

	loading.writeEurekaLump(wad);
	wad.writeToDisk();
}

void Instance::ConfirmLevelSaveSuccess(const LoadingData &loading, const Wad_file &wad)
{
	global::recent.addRecent(wad.PathName(), loading.levelName, global::home_dir);

	Status_Set("Saved %s", loading.levelName.c_str());

	if (main_win)
	{
		main_win->SetTitle(wad.PathName().u8string(), loading.levelName, false);

		// save the user state associated with this map
		M_SaveUserState();
	}

	this->level.MadeChanges = false;
}

//
// Write out the level data
//
void Instance::SaveLevelAndUpdateWindow(LoadingData& loading, const SString &level, Wad_file &wad, bool inhibit_node_build)
{
	SaveLevel(loading, level, wad, inhibit_node_build);
	ConfirmLevelSaveSuccess(loading, wad);
}

// these return false if user cancelled
bool Instance::M_SaveMap(bool inhibit_node_build)
{
	// we require a wad file to save into.
	// if there is none, then need to create one via Export function.

	if (!wad.master.editWad())
	{
		return M_ExportMap(inhibit_node_build);
	}

	if (wad.master.editWad()->IsReadOnly())
	{
		if (DLG_Confirm({ "Cancel", "&Export" },
		                "The current pwad is a READ-ONLY file. "
						"Do you want to export this map into a new file?") <= 0)
		{
			return false;
		}

		return M_ExportMap(inhibit_node_build);
	}


	M_BackupWad(wad.master.editWad().get());

	gLog.printf("Saving Map : %s in %s\n", loaded.levelName.c_str(), wad.master.editWad()->PathName().u8string().c_str());

	try
	{
		SaveLevelAndUpdateWindow(loaded, loaded.levelName, *wad.master.editWad(), inhibit_node_build);
	}
	catch(const std::runtime_error &e)
	{
		DLG_ShowError(false, "Could not save map: %s", e.what());
		return false;
	}

	return true;
}


bool Instance::M_ExportMap(bool inhibit_node_build)
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

	std::shared_ptr<Wad_file> wad = Wad_file::Open(filename, WadOpenMode::append);
	if (!wad)
	{
		DLG_Notify("Unable to export the map:\n\n%s",
			"Error creating output file");
		return false;
	}
	if (wad->IsReadOnly())
	{
		DLG_Notify("Cannot export the map into a READ-ONLY file.");

		return false;
	}
	// adopt iwad/port/resources of the target wad
	LoadingData loading = loaded;
	if (wad->FindLump(EUREKA_LUMP))
	{
		if (!loading.parseEurekaLump(global::home_dir, global::install_dir, global::recent, wad.get()))
			return false;
	}

	// ask user for map name

	SString map_name;
	{
		UI_ChooseMap dialog(loading.levelName.c_str());

		dialog.PopulateButtons(static_cast<char>(toupper(loading.levelName[0])),
								wad.get());

		map_name = dialog.Run();
	}

	// cancelled?
	if (map_name.empty())
	{
		return false;
	}


	// we will write into the chosen wad.
	// however if the level already exists, get confirmation first

	if (wad->LevelFind(map_name) >= 0)
	{
		if (DLG_Confirm({ "Cancel", "&Overwrite" },
		                overwrite_message, "selected") <= 0)
		{
			return false;
		}
	}

	// back-up an existing wad
	if (wad->NumLumps() > 0)
	{
		M_BackupWad(wad.get());
	}


	gLog.printf("Exporting Map : %s in %s\n", map_name.c_str(), wad->PathName().u8string().c_str());
	
	try
	{
		
		SaveLevel(loading, map_name, *wad, inhibit_node_build);
	}
	catch(const std::runtime_error &e)
	{
		DLG_ShowError(false, "Could not export map: %s", e.what());
		return false;
	}
		
	try
	{
		// do this after the save (in case it fatal errors)
		Main_LoadResources(loading);
	}
	catch(const std::runtime_error &e)
	{
		DLG_ShowError(false, "Successfully exported map, but could not load the new resources: %s", e.what());
		return false;
	}

	// the new wad replaces the current PWAD
	this->wad.master.ReplaceEditWad(wad);
	ConfirmLevelSaveSuccess(loaded, *wad);

	return true;
}


void Instance::CMD_SaveMap()
{
	M_SaveMap(false);
}


void Instance::CMD_ExportMap()
{
	M_ExportMap(false);
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
			DLG_Notify("Cannot copy map: file is read-only.");
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

		SaveLevelAndUpdateWindow(loaded, new_name, *wad.master.editWad(), false);

		Status_Set("Copied to %s", loaded.levelName.c_str());
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Could not copy map: %s", e.what());
	}
	
}


void Instance::CMD_RenameMap()
{
	tl::optional<SString> backupName;
	tl::optional<int> backupIndex;
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
			backupIndex = level_lump;
			backupName = wad.master.editWad()->GetLump(level_lump)->Name();

			wad.master.editWad()->RenameLump(level_lump, new_name.c_str());
			wad.master.editWad()->writeToDisk();
		}

		loaded.levelName = new_name.asUpper();

		main_win->SetTitle(wad.master.editWad()->PathName().u8string(), loaded.levelName, false);

		Status_Set("Renamed to %s", loaded.levelName.c_str());
	}
	catch (const std::runtime_error& e)
	{
		if(backupIndex && backupName)
			wad.master.editWad()->RenameLump(*backupIndex, backupName->c_str());
		DLG_ShowError(false, "Could not rename map: %s", e.what());
	}
	
}


void Instance::CMD_DeleteMap()
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

	M_BackupWad(wad.master.editWad().get());

	// kick it to the curb
	int backupPoint = wad.master.editWad()->LevelHeader(lev_num);
	std::vector<Lump_c> backupLumps = wad.master.editWad()->RemoveLevel(lev_num);
	try
	{
		wad.master.editWad()->writeToDisk();
	}
	catch (const std::runtime_error& e)
	{
		// Restore deleted lumps
		wad.master.editWad()->InsertPoint(backupPoint);
		for (const Lump_c& backup : backupLumps)
		{
			if(&backup == &backupLumps[0])
				wad.master.editWad()->AddLevel(backup.Name())->Write(backup.getData().data(), (int)backup.getData().size());
			else
				wad.master.editWad()->AddLump(backup.Name()).Write(backup.getData().data(), (int)backup.getData().size());
		}
		wad.master.editWad()->SortLevels();
		DLG_ShowError(false, "Cannot delete map: %s", e.what());
		return;
	}


	// choose a new level to load
	try
	{
		if (lev_num >= wad.master.editWad()->LevelCount())
			lev_num = wad.master.editWad()->LevelCount() - 1;

		int lump_idx = wad.master.editWad()->LevelHeader(lev_num);
		const Lump_c* lump = wad.master.editWad()->GetLump(lump_idx);
		const SString& map_name = lump->Name();

		gLog.printf("OK.  Loading : %s....\n", map_name.c_str());

		// TODO: overhaul the interface to NOT go back to the IWAD
		LoadLevel(wad.master.editWad().get(), map_name);
	}
	catch (const std::runtime_error& e)
	{
		DLG_ShowError(false, "Failed changing map after deleting a level: %s\n\nThe PWAD will be closed.", e.what());
		if (!wad.master.gameWad())
			throw;
		
		int lump_idx = wad.master.gameWad()->LevelHeader(0);
		const Lump_c* lump = wad.master.gameWad()->GetLump(lump_idx);
		if (!lump)
			throw;

		wad.master.RemoveEditWad();
		const SString& map_name = lump->Name();
		LoadLevel(wad.master.gameWad().get(), map_name);
	}
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
