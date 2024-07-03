//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#ifndef INSTANCE_H_
#define INSTANCE_H_

#include "bsp.h"
#include "Document.h"
#include "e_main.h"
#include "im_img.h"
#include "m_game.h"
#include "m_loadsave.h"
#include "m_nodes.h"
#include "main.h"
#include "r_grid.h"
#include "r_render.h"
#include "r_subdiv.h"
#include "ui_file.h"
#include "w_texture.h"
#include "w_wad.h"
#include "WadData.h"

#include <unordered_map>

class Fl_RGB_Image;
class Lump_c;
class UI_NodeDialog;
class UI_ProjectSetup;
struct BadCount;
struct NewDocument;
struct v2double_t;
struct v2int_t;

//
// For SelectNeighborLines and SelectNeighborSectors
//
enum SelectNeighborCriterion
{
	height,
	texture,
};

//
// An instance with a document, holding all other associated data, such as the window reference, the
// wad list.
//
class Instance : public grid::Listener
{
public:
	// E_COMMANDS
	void ACT_Click_release();
	void ACT_Drag_release();
	void ACT_SelectBox_release();
	void ACT_Transform_release();
	void ACT_AdjustOfs_release();
	void CheckBeginDrag();
	void CMD_AboutDialog();
	void CMD_ACT_Drag();
	void CMD_ACT_Click();
	void CMD_ACT_SelectBox();
	void CMD_ACT_Transform();
	void CMD_AddBehaviorLump();
	void CMD_ApplyTag();
	void CMD_BR_ClearSearch();
	void CMD_BR_CycleCategory();
	void CMD_BR_Scroll();
	void CMD_BrowserMode();
	void CMD_BuildAllNodes();
	void CMD_Clipboard_Copy();
	void CMD_Clipboard_Cut();
	void CMD_Clipboard_Paste();
	void CMD_CopyAndPaste();
	void CMD_CopyMap();
	void CMD_CopyProperties();
	void CMD_DefaultProps();
	void CMD_Delete();
	void CMD_DeleteMap();
	void CMD_Disconnect();
	void CMD_EditLump();
	void CMD_EditMode();
	void CMD_Enlarge();
	void CMD_ExportMap();
	void CMD_FindDialog();
	void CMD_FindNext();
	void CMD_FlipMap();
	void CMD_FreshMap();
	void CMD_GivenFile();
	void CMD_GoToCamera();
	void CMD_GRID_Bump();
	void CMD_GRID_Set();
	void CMD_GRID_Zoom();
	void CMD_InvertSelection();
	void CMD_JumpToObject();
	void CMD_LastSelection();
	void CMD_LIN_Align();
	void CMD_LIN_Flip();
	void CMD_LIN_SelectPath();
	void CMD_LIN_SplitHalf();
	void CMD_LIN_SwapSides();
	void CMD_LogViewer();
	void CMD_ManageProject();
	void CMD_MapCheck();
	void CMD_Merge();
	void CMD_MetaKey();
	void CMD_Mirror();
	void CMD_MoveObjects_Dialog();
	void CMD_NAV_MouseScroll();
	void CMD_NAV_Scroll_Down();
	void CMD_NAV_Scroll_Left();
	void CMD_NAV_Scroll_Right();
	void CMD_NAV_Scroll_Up();
	void CMD_NewProject();
	void CMD_Nothing();
	void CMD_ObjectInsert();
	void CMD_OnlineDocs();
	void CMD_OpenMap();
	void CMD_OperationMenu();
	void CMD_PlaceCamera();
	void CMD_Preferences();
	void CMD_PruneUnused();
	void CMD_Quantize();
	void CMD_Quit();
	void CMD_RecalcSectors();
	void CMD_RenameMap();
	void CMD_Redo();
	void CMD_Rotate90();
	void CMD_RotateObjects_Dialog();
	void CMD_SaveMap();
	void CMD_ScaleObjects_Dialog();
	void CMD_Scroll();
	void CMD_SEC_Ceil();
	void CMD_SEC_Floor();
	void CMD_SEC_Light();
	void CMD_SEC_SelectGroup();
	void CMD_SEC_SwapFlats();
	void CMD_Select();
	void CMD_SelectAll();
	void CMD_SelectNeighbors();
	void CMD_SetVar();
	void CMD_Shrink();
	void CMD_TestMap();
	void CMD_ChangeTestSettings();
	void CMD_TH_SpinThings();
	void CMD_ToggleVar();
	void CMD_Undo();
	void CMD_UnselectAll();
	void CMD_VT_ShapeArc();
	void CMD_VT_ShapeLine();
	void CMD_WHEEL_Scroll();
	void CMD_Zoom();
	void CMD_ZoomSelection();
	void CMD_ZoomWholeMap();
	void NAV_MouseScroll_release();
	void NAV_Scroll_Left_release();
	void NAV_Scroll_Right_release();
	void NAV_Scroll_Up_release();
	void NAV_Scroll_Down_release();
	void R3D_ACT_AdjustOfs();
	void R3D_Backward();
	void R3D_Forward();
	void R3D_Left();
	void R3D_NAV_Forward();
	void R3D_NAV_Forward_release();
	void R3D_NAV_Back();
	void R3D_NAV_Back_release();
	void R3D_NAV_Right();
	void R3D_NAV_Right_release();
	void R3D_NAV_Left();
	void R3D_NAV_Left_release();
	void R3D_NAV_Up();
	void R3D_NAV_Up_release();
	void R3D_NAV_Down();
	void R3D_NAV_Down_release();
	void R3D_NAV_TurnLeft();
	void R3D_NAV_TurnLeft_release();
	void R3D_NAV_TurnRight();
	void R3D_NAV_TurnRight_release();
	void R3D_Down();
	void R3D_DropToFloor();
	void R3D_Right();
	void R3D_Set();
	void R3D_Toggle();
	void R3D_Turn();
	void R3D_Up();
	void R3D_WHEEL_Move();
	void Transform_Update();

	// E_LINEDEF
	bool LD_RailHeights(int &z1, int &z2, const LineDef *L, const SideDef *sd,
		const Sector *front, const Sector *back) const;

	// E_MAIN
	void Editor_ChangeMode(char mode_char);
	void Editor_ChangeMode_Raw(ObjType new_mode);
	void Editor_ClearAction() noexcept;
	void Editor_Init();
	bool Editor_ParseUser(const std::vector<SString> &tokens);
	void Editor_WriteUser(std::ostream &os) const;
	void MapStuff_NotifyBegin();
	void MapStuff_NotifyChange(ObjType type, int objnum, int field);
	void MapStuff_NotifyDelete(ObjType type, int objnum);
	void MapStuff_NotifyEnd();
	void MapStuff_NotifyInsert(ObjType type, int objnum);
	void ObjectBox_NotifyBegin();
	void ObjectBox_NotifyChange(ObjType type, int objnum, int field);
	void ObjectBox_NotifyDelete(ObjType type, int objnum);
	void ObjectBox_NotifyEnd() const;
	void ObjectBox_NotifyInsert(ObjType type, int objnum);
	bool RecUsed_ParseUser(const std::vector<SString> &tokens);
	void RecUsed_WriteUser(std::ostream &os) const;
	void RedrawMap();
	void Selection_Clear(bool no_save = false);
	void Selection_InvalidateLast() noexcept;
	void Selection_NotifyBegin();
	void Selection_NotifyDelete(ObjType type, int objnum);
	void Selection_NotifyEnd();
	void Selection_NotifyInsert(ObjType type, int objnum);
	void Selection_Push();
	void UpdateHighlight();
	void ZoomWholeMap();

	// E_PATH
	void GoToErrors();
	void GoToObject(const Objid &objid);
	void GoToSelection();
	const byte *SoundPropagation(int start_sec);

	// M_CONFIG
	void M_DefaultUserState();
	bool M_LoadUserState();
	bool M_SaveUserState() const;

	// M_EVENTS
	void ClearStickyMod();
	void Editor_ScrollMap(int mode, v2int_t dpos = {}, keycode_t mod = 0);
	void Editor_SetAction(EditorAction new_action);
	void EV_EscapeKey();
	int EV_HandleEvent(int event);
	void M_LoadOperationMenus();
	bool Nav_ActionKey(keycode_t key, nav_release_func_t func);
	void Nav_Clear();
	void Nav_Navigate();
	bool Nav_SetKey(keycode_t key, nav_release_func_t func);
	unsigned Nav_TimeDiff();

	// M_FILES
	fs::path M_PickDefaultIWAD() const;
	bool M_TryOpenMostRecent();

	// M_GAME
	bool is_sky(const SString &flat) const;
	char M_GetFlatType(const SString &name) const;
	const sectortype_t &M_GetSectorType(int type) const;
	char M_GetTextureType(const SString &name) const;
	SString M_LineCategoryString(SString &letters) const;
	SString M_TextureCategoryString(SString &letters, bool do_flats) const;
	SString M_ThingCategoryString(SString &letters) const;

	// M_KEYS
	void Beep(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(2, 3);
	bool Exec_HasFlag(const char *flag) const;
	bool ExecuteCommand(const editor_command_t *cmd, const SString &param1 = "", const SString &param2 = "", const SString &param3 = "", const SString &param4 = "");
	bool ExecuteCommand(const SString &name, const SString &param1 = "", const SString &param2 = "", const SString &param3 = "", const SString &param4 = "");
	bool ExecuteKey(keycode_t key, KeyContext context);

	// M_LOADSAVE
	NewDocument openDocument(const LoadingData &inLoading, const Wad_file &wad, int level);
	void LoadLevel(const Wad_file *wad, const SString &level) noexcept(false);
	void LoadLevelNum(const Wad_file *wad, int lev_num) noexcept(false);
	bool MissingIWAD_Dialog();
	bool M_SaveMap(bool inhibit_node_build);
	void refreshViewAfterLoad(const BadCount& bad, const Wad_file* wad, const SString& map_name, bool new_resources);

	// M_NODES
	void BuildNodesAfterSave(int lev_idx, const LoadingData& loading, Wad_file &wad);
	void GB_PrintMsg(EUR_FORMAT_STRING(const char *str), ...) EUR_PRINTF(2, 3);

	// M_TESTMAP
	bool M_PortSetupDialog(const SString& port, const SString& game, const tl::optional<SString> &commandLine);

	// M_UDMF
	void UDMF_LoadLevel(int loading_level, const Wad_file *load_wad, Document &doc, LoadingData &loading, BadCount &bad) const;
	void UDMF_SaveLevel(const LoadingData &loading, Wad_file &wad) const;

	// MAIN
	fs::path Main_FileOpFolder() const;
	void Main_LoadResources(const LoadingData &loading) noexcept(false);
	void UpdateViewOnResources();

	// R_RENDER
	void Render3D_CB_Copy() ;
	void Render3D_GetCameraPos(v2double_t &pos, float *angle) const;
	void Render3D_MouseMotion(v2int_t pos, keycode_t mod, v2int_t dpos);
	bool Render3D_ParseUser(const std::vector<SString> &tokens);
	void Render3D_SetCameraPos(const v2double_t &newpos);
	void Render3D_Setup();
	void Render3D_UpdateHighlight();
	void Render3D_WriteUser(std::ostream &os) const;

	// R_SOFTWARE
	bool SW_QueryPoint(Objid &hl, int qx, int qy);
	void SW_RenderWorld(int ox, int oy, int ow, int oh);

	// R_SUBDIV
	sector_3dfloors_c *Subdiv_3DFloorsForSector(int num);
	void Subdiv_InvalidateAll() noexcept;
	bool Subdiv_SectorOnScreen(int num, double map_lx, double map_ly, double map_hx, double map_hy);
	sector_subdivision_c *Subdiv_PolygonsForSector(int num);

	// UI_BROWSER
	void Browser_WriteUser(std::ostream &os) const;

	// UI_DEFAULT
	void Props_WriteUser(std::ostream &os) const;

	// UI_INFOBAR
	void Status_Set(EUR_FORMAT_STRING(const char *fmt), ...) const EUR_PRINTF(2, 3);
	void Status_Clear() const;
	
	// GridListener
	void gridRedrawMap() override
	{
		RedrawMap();
	}
	
	void gridSetGrid(int grid) override
	{
		if (main_win)
			main_win->info_bar->SetGrid(grid);
	}
	
	void gridUpdateSnap() override
	{
		if (main_win)
			main_win->info_bar->UpdateSnap();
	}
	
	void gridAdjustPos() override
	{
		if (main_win)
			main_win->scroll->AdjustPos();
	}
	
	void gridPointerPos() override
	{
		if (main_win)
			main_win->canvas->PointerPos();
	}
	
	void gridSetScale(double scale) override
	{
		if (main_win)
			main_win->info_bar->SetScale(scale);
	}
	
	void gridBeep(const char *message) override
	{
		Beep("%s", message);
	}

	void gridUpdateRatio() override
	{
		if(main_win)
			main_win->info_bar->UpdateRatio();
	}

private:
	// New private methods
	void navigationScroll(float *editNav, nav_release_func_t func);
	void navigation3DMove(float *editNav, nav_release_func_t func, bool fly);
	void navigation3DTurn(float *editNav, nav_release_func_t func);

	// E_COMMANDS
	void DoBeginDrag();

	// E_CUTPASTE
	bool Clipboard_DoCopy();
	bool Clipboard_DoPaste();
	void ReselectGroup();

	// E_LINEDEF
	void commandLinedefMergeTwo();

	// E_MAIN
	void Editor_ClearErrorMode();
	void UpdateDrawLine();
	void zoom_fit();
	void SelectNeighborSectors(int objnum, SelectNeighborCriterion option, byte parts);

	// E_SECTOR
	void commandSectorMerge();

	// E_VERTEX
	void commandLineDisconnect();
	void commandSectorDisconnect();
	void commandVertexDisconnect();
	void commandVertexMerge();

	// M_EVENTS
	void Editor_Zoom(int delta, v2int_t mid);
	void EV_EnterWindow();
	void EV_LeaveWindow();
	void EV_MouseMotion(v2int_t pos, keycode_t mod, v2int_t dpos);
	int EV_RawButton(int event);
	int EV_RawKey(int event);
	int EV_RawMouse(int event);
	int EV_RawWheel(int event);
	void M_AddOperationMenu(const SString &context, Fl_Menu_Button *menu);
	bool M_ParseOperationFile();

	// M_KEYS
	void DoExecuteCommand(const editor_command_t *cmd);

	// M_LOADSAVE
	void FreshLevel();
	
	bool M_ExportMap(bool inhibit_node_build);
	void Navigate2D();
	void Project_ApplyChanges(const UI_ProjectSetup::Result &result) noexcept(false);
	tl::optional<fs::path> Project_AskFile() const;
	void SaveLevel(LoadingData &loading, const SString &level, Wad_file &wad, bool inhibit_node_build);
	void ConfirmLevelSaveSuccess(const LoadingData &loading, const Wad_file &wad);
	void SaveLevelAndUpdateWindow(LoadingData& loading, const SString& level, Wad_file &wad, bool inhibit_node_build);

	// M_NODES
	build_result_e BuildAllNodes(nodebuildinfo_t *info);

	// R_GRID

	// R_RENDER
	StringID GrabSelectedFlat();
	StringID GrabSelectedTexture();
	int GrabSelectedThing();
	StringID LD_GrabTex(const LineDef *L, int part) const;
	void Render3D_CB_Cut();
	void Render3D_CB_Paste();
	void Render3D_Navigate();
	void StoreDefaultedFlats();
	void StoreSelectedFlat(StringID new_tex);
	void StoreSelectedTexture(StringID new_tex);
	void StoreSelectedThing(int new_type);

public:	// will be private when we encapsulate everything
	Document level{*this};	// level data proper

	UI_MainWindow *main_win = nullptr;
	Editor_State_t edit = {};

	//
	// Wad settings
	//

	LoadingData loaded;

	//
	// Game-dependent (thus instance dependent) defaults
	//
	ConfigData conf;

	//
	// Panel stuff
	//
	bool changed_panel_obj = false;
	bool changed_recent_list = false;
	bool invalidated_last_sel = false;
	bool invalidated_panel_obj = false;
	bool invalidated_selection = false;
	bool invalidated_totals = false;

	//
	// Selection stuff
	//
	tl::optional<selection_c> last_Sel;

	//
	// Document stuff
	//
	int moved_vertex_count = 0;
	int new_vertex_minimum = 0;
	bool recalc_map_bounds = false;
	// the containers for the textures (etc)
	Recently_used recent_flats{ *this };
	Recently_used recent_textures{ *this };
	Recently_used recent_things{ *this };
	int last_given_file = 0;
	tl::optional<UI_NodeDialog> nodeialog;
	nodebuildinfo_t *nb_info = nullptr;
	
	int tagInMemory = 0;

	WadData wad;

	//
	// Path stuff
	//
	bool sound_propagation_invalid = false;
	std::vector<byte> sound_prop_vec;
	std::vector<byte> sound_temp1_vec;
	std::vector<byte> sound_temp2_vec;
	int sound_start_sec = 0;

	//
	// IO stuff
	//
	nav_active_key_t cur_action_key = {};
	bool in_operation_menu = false;
	v2int_t mouse_last_pos = {};
	nav_active_key_t nav_actives[MAX_NAV_ACTIVE_KEYS] = {};
	unsigned nav_time = 0;
	bool no_operation_cfg = false;
	std::unordered_map<SString, Fl_Menu_Button *> op_all_menus;
	// these are grabbed from FL_MOUSEWHEEL events
	v2int_t wheel_dpos = {};

	// key or mouse button pressed for command, 0 when none
	keycode_t EXEC_CurKey = {};
	// result from command function, 0 is OK
	int EXEC_Errno = 0;
	SString EXEC_Flags[MAX_EXEC_PARAM] = {};
	SString EXEC_Param[MAX_EXEC_PARAM] = {};

	//
	// Rendering
	//
	grid::State grid{ *this };
	Render_View_t r_view{ *this };
	sector_info_cache_c sector_info_cache{ *this };
};

extern Instance *gInstance;	// for now we run with one instance, will have more for the MDI.

#endif
