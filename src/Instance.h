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

#include "Document.h"
#include "e_main.h"
#include "im_img.h"
#include "main.h"

class Fl_RGB_Image;
class Lump_c;

//
// An instance with a document, holding all other associated data, such as the window reference, the
// wad list.
//
class Instance
{
public:
	// E_BASIS
	fixcoord_t MakeValidCoord(double x) const;

	// E_MAIN
	void CalculateLevelBounds();
	void Editor_ChangeMode(char mode_char);
	void Editor_ClearAction();
	void Editor_DefaultState();
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
	void Selection_Add(Objid &obj) const;
	void Selection_InvalidateLast();
	void Selection_NotifyBegin();
	void Selection_NotifyDelete(ObjType type, int objnum);
	void Selection_NotifyEnd();
	void Selection_NotifyInsert(ObjType type, int objnum);
	void Selection_Push();
	void Selection_Toggle(Objid &obj) const;
	SelectHighlight SelectionOrHighlight();
	void UpdateHighlight();
	void ZoomWholeMap();

	const byte *SoundPropagation(int start_sec);

	// IM_COLOR
	byte W_FindPaletteColor(int r, int g, int b) const;
	void W_LoadColormap();
	void W_LoadPalette();
	void W_UpdateGamma();

	// IM_IMG
	Img_c *IM_ConvertRGBImage(Fl_RGB_Image *src) const;
	Img_c *IM_ConvertTGAImage(const rgba_color_t *data, int W, int H) const;
	Img_c *IM_CreateDogSprite() const;
	Img_c *IM_CreateLightSprite() const;
	Img_c *IM_CreateMapSpotSprite(int base_r, int base_g, int base_b) const;
	Img_c *IM_DigitFont_11x14();
	Img_c *IM_DigitFont_14x19();
	Img_c *IM_MissingTex();
	void IM_ResetDummyTextures();
	Img_c *IM_SpecialTex();
	Img_c *IM_UnknownFlat();
	Img_c *IM_UnknownSprite();
	Img_c *IM_UnknownTex();
	void W_UnloadAllTextures() const;
	void IM_UnloadDummyTextures() const;

	// this one applies the current gamma.
	// for rendering the 3D view or the 2D sectors and sprites.
	inline void IM_DecodePixel(img_pixel_t p, byte &r, byte &g, byte &b) const
	{
		if(p & IS_RGB_PIXEL)
		{
			r = rgb555_gamma[IMG_PIXEL_RED(p)];
			g = rgb555_gamma[IMG_PIXEL_GREEN(p)];
			b = rgb555_gamma[IMG_PIXEL_BLUE(p)];
		}
		else
		{
			const rgb_color_t col = palette[p];

			r = RGB_RED(col);
			g = RGB_GREEN(col);
			b = RGB_BLUE(col);
		}
	}

	// this applies a constant gamma.
	// for textures/flats/things in the browser and panels.
	inline void IM_DecodePixel_medium(img_pixel_t p, byte &r, byte &g, byte &b) const
	{
		if(p & IS_RGB_PIXEL)
		{
			r = rgb555_medium[IMG_PIXEL_RED(p)];
			g = rgb555_medium[IMG_PIXEL_GREEN(p)];
			b = rgb555_medium[IMG_PIXEL_BLUE(p)];
		}
		else
		{
			const rgb_color_t col = palette_medium[p];

			r = RGB_RED(col);
			g = RGB_GREEN(col);
			b = RGB_BLUE(col);
		}
	}

	// M_CONFIG
	void M_DefaultUserState();
	bool M_LoadUserState();
	bool M_SaveUserState() const;

	// M_EVENTS
	void Editor_ClearNav();
	void Editor_ScrollMap(int mode, int dx = 0, int dy = 0, keycode_t mod = 0);
	bool Nav_ActionKey(keycode_t key, nav_release_func_t func);
	void Nav_Clear();

	// M_FILES
	bool M_ParseEurekaLump(Wad_file *wad, bool keep_cmd_line_args = false);
	SString M_PickDefaultIWAD() const;
	bool M_TryOpenMostRecent();
	void M_WriteEurekaLump(Wad_file *wad) const;

	// M_GAME
	SString M_GetBaseGame(const SString &game);
	void M_PrepareConfigVariables();

	// M_KEYS
	void Beep(EUR_FORMAT_STRING(const char *fmt), ...) const EUR_PRINTF(2, 3);

	// M_LOADSAVE
	void LoadLevel(Wad_file *wad, const SString &level);
	bool MissingIWAD_Dialog();
	void RemoveEditWad();

	// MAIN
	bool Main_ConfirmQuit(const char *action) const;
	void Main_LoadResources();

	// R_RENDER
	void Render3D_MouseMotion(int x, int y, keycode_t mod, int dx, int dy);
	bool Render3D_ParseUser(const std::vector<SString> &tokens);
	void Render3D_Setup();
	void Render3D_UpdateHighlight();

	// R_SOFTWARE
	bool SW_QueryPoint(Objid &hl, int qx, int qy);
	void SW_RenderWorld(int ox, int oy, int ow, int oh);

	// UI_BROWSER
	void Browser_WriteUser(std::ostream &os) const;

	// UI_DEFAULT
	void Props_WriteUser(std::ostream &os) const;

	// UI_INFOBAR
	void Status_Set(EUR_FORMAT_STRING(const char *fmt), ...) const EUR_PRINTF(2, 3);
	void Status_Clear() const;

	// W_LOADPIC
	Img_c *LoadImage_JPEG(Lump_c *lump, const SString &name) const;
	Img_c *LoadImage_PNG(Lump_c *lump, const SString &name) const;
	Img_c *LoadImage_TGA(Lump_c *lump, const SString &name) const;
	bool LoadPicture(Img_c &dest, Lump_c *lump, const SString &pic_name, int pic_x_offset, int pic_y_offset, int *pic_width = nullptr, int *pic_height = nullptr) const;

	// W_TEXTURE
	Img_c *W_GetSprite(int type) const;
	void W_LoadFlats() const;
	void W_LoadTextures() const;
	void W_LoadTextures_TX_START(Wad_file *wf) const;

	// W_WAD
	void MasterDir_Add(Wad_file *wad);
	void MasterDir_CloseAll();
	bool MasterDir_HaveFilename(const SString &chk_path) const;
	void MasterDir_Remove(Wad_file *wad);
	Lump_c *W_FindGlobalLump(const SString &name) const;
	Lump_c *W_FindSpriteLump(const SString &name) const;

public:	// will be private when we encapsulate everything
	Document level{*this};	// level data proper

	UI_MainWindow *main_win = nullptr;
	Editor_State_t edit = {};

	//
	// Wad settings
	//

	// the current PWAD, or NULL for none.
	// when present it is also at master_dir.back()
	Wad_file *edit_wad = nullptr;
	Wad_file *game_wad = nullptr;
	std::vector<Wad_file *> master_dir;	// the IWAD, never NULL, always at master_dir.front()
	MapFormat Level_format = {};	// format of current map
	SString Level_name;	// Name of map lump we are editing
	SString Port_name;	// Name of source port "vanilla", "boom", ...
	SString Iwad_name;	// Filename of the iwad
	SString Game_name;	// Name of game "doom", "doom2", "heretic", ...
	SString Udmf_namespace;	// for UDMF, the current namespace
	std::vector<SString> Resource_list;

	//
	// Game-dependent (thus instance dependent) defaults
	//
	int default_thing = 2001;
	SString default_wall_tex = "GRAY1";
	SString default_floor_tex = "FLAT1";
	SString default_ceil_tex = "FLAT1";

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
	selection_c *last_Sel = nullptr;

	//
	// Document stuff
	//
	bool MadeChanges = false;
	double Map_bound_x1 = 32767;   /* minimum X value of map */
	double Map_bound_y1 = 32767;   /* minimum Y value of map */
	double Map_bound_x2 = -32767;   /* maximum X value of map */
	double Map_bound_y2 = -32767;   /* maximum Y value of map */
	int moved_vertex_count = 0;
	int new_vertex_minimum = 0;
	bool recalc_map_bounds = false;
	// the containers for the textures (etc)
	Recently_used recent_flats{ *this };
	Recently_used recent_textures{ *this };
	Recently_used recent_things{ *this };

	//
	// Path stuff
	//
	bool sound_propagation_invalid = false;
	std::vector<byte> sound_prop_vec;
	std::vector<byte> sound_temp1_vec;
	std::vector<byte> sound_temp2_vec;
	int sound_start_sec = 0;

	//
	// Color stuff
	//
	byte bright_map[256] = {};
	// this palette has the gamma setting applied
	rgb_color_t palette[256] = {};
	rgb_color_t palette_medium[256] = {};
	byte raw_colormap[32][256] = {};
	byte raw_palette[256][3] = {};
	byte rgb555_gamma[32];
	byte rgb555_medium[32];
	// the palette color closest to what TRANS_PIXEL really is
	int trans_replace = 0;
	int missing_tex_color = 0;
	int special_tex_color = 0;
	int unknown_flat_color = 0;
	int unknown_sprite_color = 0;
	int unknown_tex_color = 0;

	//
	// Image stuff
	//
	Img_c *digit_font_11x14 = nullptr;
	Img_c *digit_font_14x19 = nullptr;
	Img_c *missing_tex_image = nullptr;
	Img_c *special_tex_image = nullptr;
	Img_c *unknown_flat_image = nullptr;
	Img_c *unknown_sprite_image = nullptr;
	Img_c *unknown_tex_image = nullptr;
#ifndef NO_OPENGL
	bool use_npot_textures = false;
#endif

	// IO stuff
	nav_active_key_t cur_action_key;
};

extern Instance gInstance;	// for now we run with one instance, will have more for the MDI.

#endif
