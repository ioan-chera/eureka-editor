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
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"


extern short loading_level;


void LoadLevel_UDMF()
{
	// FIXME LoadLevel_UDMF
}


//----------------------------------------------------------------------

static void WriteUDMF_Info(Lump_c *lump)
{
	lump->Printf("namespace = \"DOOM\";\n\n");
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
		lump->Printf("height = %d.000;\n", th->z);

		lump->Printf("angle = %d;\n", th->angle);
		lump->Printf("type = %d;\n", th->type);

		// FIXME thing options

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

		// FIXME linedef flags

		// FIXME args / tag

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
