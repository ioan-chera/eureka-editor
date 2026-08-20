//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2026 Ioan Chera
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

#include "Instance.h"
#include "w_rawdef.h"
#include "w_wad.h"
#include "gtest/gtest.h"


TEST(MUdmf, LoadUDMFThingsThenWrite)
{
	std::shared_ptr<Wad_file> wad = Wad_file::Open("wad.wad", WadOpenMode::write);
	ASSERT_TRUE(wad);
	wad->AddLevel("MAP01");
	wad->AddLump("TEXTMAP");
	wad->AddLump("ENDMAP");

	Lump_c *textmap = wad->GetLump(1);
	ASSERT_TRUE(textmap);

	textmap->Printf("namespace = \"Doom\";\n");
	textmap->Printf("thing { x = 0; y = 0; type = 100; skill1 = true; }\n");
	textmap->Printf("thing { x = 0; y = 0; angle = 0; type = 100; skill2 = true; }\n");
	textmap->Printf("thing { x = 0; y = 0; angle = 30; type = 100; skill3 = true; }\n");
	textmap->Printf("thing { x = 0; y = 0; type = 100; skill4 = true; }\n");
	textmap->Printf("thing { x = 0; y = 0; type = 100; skill5 = true; }\n");

	Instance inst;
	inst.loaded.levelFormat = MapFormat::udmf;
	BadCount bad = {};
	inst.UDMF_LoadLevel(0, wad.get(), inst.level, inst.loaded, bad);
	ASSERT_FALSE(bad.linedef_count);
	ASSERT_FALSE(bad.sector_refs);
	ASSERT_FALSE(bad.sidedef_refs);

	ASSERT_EQ(inst.level.numThings(), 5);
	ASSERT_EQ(inst.level.things[0]->udmfflags, MTF_UDMF_skill1);
	ASSERT_EQ(inst.level.things[0]->options, 0);
	ASSERT_EQ(inst.level.things[1]->udmfflags, 0);
	ASSERT_EQ(inst.level.things[1]->options, MTF_Easy);
	ASSERT_EQ(inst.level.things[2]->udmfflags, 0);
	ASSERT_EQ(inst.level.things[2]->options, MTF_Medium);
	ASSERT_EQ(inst.level.things[3]->udmfflags, 0);
	ASSERT_EQ(inst.level.things[3]->options, MTF_Hard);
	ASSERT_EQ(inst.level.things[4]->udmfflags, MTF_UDMF_skill5);
	ASSERT_EQ(inst.level.things[4]->options, 0);

	// Now write it
	inst.UDMF_SaveLevel(inst.loaded, *wad);
	ASSERT_EQ(wad->NumLumps(), 5);
	Lump_c *outTextmap = wad->GetLump(3);
	ASSERT_TRUE(outTextmap);

	std::string data(outTextmap->getData().begin(), outTextmap->getData().end());
	ASSERT_EQ(data, R"DATA(namespace = "Doom";

thing // 0
{
x = 0;
y = 0;
type = 100;
skill1 = true;
}

thing // 1
{
x = 0;
y = 0;
type = 100;
skill2 = true;
}

thing // 2
{
x = 0;
y = 0;
angle = 30;
type = 100;
skill3 = true;
}

thing // 3
{
x = 0;
y = 0;
type = 100;
skill4 = true;
}

thing // 4
{
x = 0;
y = 0;
type = 100;
skill5 = true;
}

)DATA");
}

