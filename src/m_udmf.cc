//------------------------------------------------------------------------
//  UDMF PARSING / WRITING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2019 Andrew Apted
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

#include "main.h"

#include "m_udmf.h"
#include "m_game.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"


extern short loading_level;


void LoadLevel_UDMF()
{
	// FIXME LoadLevel_UDMF
}


//----------------------------------------------------------------------

static inline void WrFlag(Lump_c *lump, int flags, const char *name, int mask)
{
	if ((flags & mask) != 0)
	{
		lump->Printf("%s = true;\n", name);
	}
}

static void WriteUDMF_Info(Lump_c *lump)
{
	// TODO other namespaces

	lump->Printf("namespace = \"doom\";\n\n");
}

static void WriteUDMF_Things(Lump_c *lump)
{
	for (int i = 0 ; i < NumThings ; i++)
	{
		lump->Printf("thing // %d\n", i);
		lump->Printf("{\n");

		const Thing *th = Things[i];

		lump->Printf("x = %d.000;\n", th->x);
		lump->Printf("y = %d.000;\n", th->y);

		if (th->z != 0)
			lump->Printf("height = %d.000;\n", th->z);

		lump->Printf("angle = %d;\n", th->angle);
		lump->Printf("type = %d;\n", th->type);

		// thing options
		WrFlag(lump, th->options, "skill1", MTF_Easy);
		WrFlag(lump, th->options, "skill2", MTF_Easy);
		WrFlag(lump, th->options, "skill3", MTF_Medium);
		WrFlag(lump, th->options, "skill4", MTF_Hard);
		WrFlag(lump, th->options, "skill5", MTF_Hard);

		WrFlag(lump, 0 ^ th->options, "single", MTF_Not_SP);
		WrFlag(lump, 0 ^ th->options, "coop",   MTF_Not_COOP);
		WrFlag(lump, 0 ^ th->options, "dm",     MTF_Not_DM);

		WrFlag(lump, th->options, "ambush", MTF_Ambush);

		if (game_info.friend_flag)
			WrFlag(lump, th->options, "friend", MTF_Friend);

		// TODO Hexen flags

		// TODO Strife flags

		// TODO Hexen special and args

		lump->Printf("}\n\n");
	}
}

static void WriteUDMF_Vertices(Lump_c *lump)
{
	for (int i = 0 ; i < NumVertices ; i++)
	{
		lump->Printf("vertex // %d\n", i);
		lump->Printf("{\n");

		const Vertex *vert = Vertices[i];

		lump->Printf("x = %d.000;\n", vert->x);
		lump->Printf("y = %d.000;\n", vert->y);

		lump->Printf("}\n\n");
	}
}

static void WriteUDMF_LineDefs(Lump_c *lump)
{
	for (int i = 0 ; i < NumLineDefs ; i++)
	{
		lump->Printf("linedef // %d\n", i);
		lump->Printf("{\n");

		const LineDef *ld = LineDefs[i];

		lump->Printf("v1 = %d;\n", ld->start);
		lump->Printf("v2 = %d;\n", ld->end);

		if (ld->right >= 0)
			lump->Printf("sidefront = %d;\n", ld->right);
		if (ld->left >= 0)
			lump->Printf("sideback = %d;\n", ld->left);

		if (ld->type != 0)
			lump->Printf("special = %d;\n", ld->type);

		if (ld->tag != 0)
			lump->Printf("arg0 = %d;\n", ld->tag);
		if (ld->arg2 != 0)
			lump->Printf("arg1 = %d;\n", ld->arg2);
		if (ld->arg3 != 0)
			lump->Printf("arg2 = %d;\n", ld->arg3);
		if (ld->arg4 != 0)
			lump->Printf("arg3 = %d;\n", ld->arg4);
		if (ld->arg5 != 0)
			lump->Printf("arg4 = %d;\n", ld->arg5);

		// linedef flags
		WrFlag(lump, ld->flags, "blocking",      MLF_Blocking);
		WrFlag(lump, ld->flags, "blockmonsters", MLF_BlockMonsters);
		WrFlag(lump, ld->flags, "twosided",      MLF_TwoSided);
		WrFlag(lump, ld->flags, "dontpegtop",    MLF_UpperUnpegged);
		WrFlag(lump, ld->flags, "dontpegbottom", MLF_LowerUnpegged);
		WrFlag(lump, ld->flags, "secret",        MLF_Secret);
		WrFlag(lump, ld->flags, "blocksound",    MLF_SoundBlock);
		WrFlag(lump, ld->flags, "dontdraw",      MLF_DontDraw);
		WrFlag(lump, ld->flags, "mapped",        MLF_Mapped);

		if (game_info.pass_through)
			WrFlag(lump, ld->flags, "passuse", MLF_Boom_PassThru);

		if (game_info.midtex_3d)
			WrFlag(lump, ld->flags, "midtex3d", MLF_Eternity_3DMidTex);

		// TODO : hexen stuff (SPAC flags, etc)

		// TODO : strife stuff

		// TODO : zdoom stuff

		lump->Printf("}\n\n");
	}
}

static void WriteUDMF_SideDefs(Lump_c *lump)
{
	for (int i = 0 ; i < NumSideDefs ; i++)
	{
		lump->Printf("sidedef // %d\n", i);
		lump->Printf("{\n");

		const SideDef *side = SideDefs[i];

		lump->Printf("sector = %d;\n", side->sector);

		if (side->x_offset != 0)
			lump->Printf("offsetx = %d;\n", side->x_offset);
		if (side->y_offset != 0)
			lump->Printf("offsety = %d;\n", side->y_offset);

		// use NormalizeTex to ensure no double quote

		if (strcmp(side->UpperTex(), "-") != 0)
			lump->Printf("texturetop = \"%s\";\n", NormalizeTex(side->UpperTex()));
		if (strcmp(side->LowerTex(), "-") != 0)
			lump->Printf("texturebottom = \"%s\";\n", NormalizeTex(side->LowerTex()));
		if (strcmp(side->MidTex(), "-") != 0)
			lump->Printf("texturemiddle = \"%s\";\n", NormalizeTex(side->MidTex()));

		lump->Printf("}\n\n");
	}
}

static void WriteUDMF_Sectors(Lump_c *lump)
{
	for (int i = 0 ; i < NumSectors ; i++)
	{
		lump->Printf("sector // %d\n", i);
		lump->Printf("{\n");

		const Sector *sec = Sectors[i];

		lump->Printf("heightfloor = %d;\n", sec->floorh);
		lump->Printf("heightceiling = %d;\n", sec->ceilh);

		// use NormalizeTex to ensure no double quote

		lump->Printf("texturefloor = \"%s\";\n", NormalizeTex(sec->FloorTex()));
		lump->Printf("textureceiling = \"%s\";\n", NormalizeTex(sec->CeilTex()));

		lump->Printf("lightlevel = %d;\n", sec->light);
		if (sec->type != 0)
			lump->Printf("special = %d;\n", sec->type);
		if (sec->tag != 0)
			lump->Printf("id = %d;\n", sec->tag);

		lump->Printf("}\n\n");
	}
}

void SaveLevel_UDMF()
{
	Lump_c *lump = edit_wad->AddLump("TEXTMAP");

	WriteUDMF_Info(lump);
	WriteUDMF_Things(lump);
	WriteUDMF_Vertices(lump);
	WriteUDMF_LineDefs(lump);
	WriteUDMF_SideDefs(lump);
	WriteUDMF_Sectors(lump);

	lump->Finish();

	lump = edit_wad->AddLump("ENDMAP");
	lump->Finish();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
