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

#include "Instance.h"
#include "main.h"

#include "LineDef.h"
#include "m_game.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_window.h"

class Udmf_Token
{
private:
	// empty means EOF
	SString text;

	Instance &inst;

public:
	Udmf_Token(Instance &inst, const char *str) : text(str), inst(inst)
	{ }

	Udmf_Token(Instance &inst, const char *str, int len) : text(str, len), inst(inst)
	{ }

	const char *c_str()
	{
		return text.c_str();
	}

	bool IsEOF() const
	{
		return text.empty();
	}

	bool IsIdentifier() const
	{
		if (text.size() == 0)
			return false;

		char ch = text[0];

		return isalpha(ch) || ch == '_';
	}

	bool IsString() const
	{
		return text.size() > 0 && text[0] == '"';
	}

	bool Match(const char *name) const
	{
		return y_stricmp(text.c_str(), name) == 0;
	}

	int DecodeInt() const
	{
		return atoi(text);
	}

	double DecodeFloat() const
	{
		return atof(text);
	}

	SString DecodeString() const
	{
		if (! IsString() || text.size() < 2)
		{
			// TODO warning
			return SString();
		}

		SString string = text;
		string.erase(0, 1);
		string.pop_back();
		return string;
	}

	FFixedPoint DecodeCoord() const
	{
		return MakeValidCoord(inst.loaded.levelFormat, DecodeFloat());
	}

	StringID DecodeTexture() const
	{
		SString buffer;

		if (! IsString())
		{
			// TODO warning
			buffer = "-";
		}
		else
		{
			int use_len = 8;

			if (text.size() < 10)
				use_len = (int)text.size() - 2;
			buffer = text;
			buffer.erase(0, 1);
			buffer.erase(use_len, SString::npos);
		}

		return BA_InternaliseString(NormalizeTex(buffer));
	}
};


// since UDMF lumps can be very large, we read chunks of it
// as-needed instead of loading the whole thing into memory.
// the buffer size should be over 2x maximum token length.
#define U_BUF_SIZE  16384

class Udmf_Parser
{
private:
	Lump_c *lump;

	// reached EOF or a file read error
	bool done = false;

	// we have seen a "/*" but not the closing "*/"
	bool in_comment = false;

	// number of remaining bytes
	int remaining;

	// read buffer
	char buffer[U_BUF_SIZE];

	// position in buffer and used size of buffer
	int b_pos = 0;
	int b_size = 0;

	Instance &inst;

public:
	Udmf_Parser(Instance &inst, Lump_c *_lump) : lump(_lump), inst(inst)
	{
		remaining = lump->Length();
	}

	Udmf_Token Next()
	{
		for (;;)
		{
			if (done)
				return Udmf_Token(inst, "");

			// when position reaches half-way point, shift buffer down
			if (b_pos >= U_BUF_SIZE/2)
			{
				memmove(buffer, buffer + U_BUF_SIZE/2, U_BUF_SIZE/2);

				b_pos  -= U_BUF_SIZE/2;
				b_size -= U_BUF_SIZE/2;
			}

			// top up the buffer
			if (remaining > 0 && b_size < U_BUF_SIZE)
			{
				int want = U_BUF_SIZE - b_size;
				if (want > remaining)
					want = remaining;

				if (! lump->Read(buffer + b_size, want))
				{
					// TODO mark error somewhere, show dialog later
					done = true;
					continue;
				}

				remaining -= want;
				b_size    += want;
			}

			// end of file?
			if (remaining <= 0 && b_pos >= b_size)
			{
				done = true;
				continue;
			}

			if (in_comment)
			{
				// end of multi-line comment?
				if (b_pos+2 <= b_size &&
					buffer[b_pos] == '*' &&
					buffer[b_pos+1] == '/')
				{
					in_comment = false;
					b_pos += 2;
					continue;
				}

				b_pos++;
				continue;
			}

			// check for multi-line comment
			if (b_pos+2 <= b_size &&
				buffer[b_pos] == '/' &&
				buffer[b_pos+1] == '*')
			{
				in_comment = true;
				b_pos += 2;
				continue;
			}

			// check for single-line comment
			if (b_pos+2 <= b_size &&
				buffer[b_pos] == '/' &&
				buffer[b_pos+1] == '/')
			{
				SkipToEOLN();
				continue;
			}

			// skip whitespace (assumes ASCII)
			int start = b_pos;
			unsigned char ch = buffer[b_pos];

			if ((ch <= 32) || (ch >= 127 && ch <= 160))
			{
				b_pos++;
				continue;
			}

			// an actual token, yay!

			// is it a string?
			if (ch == '"')
			{
				b_pos++;

				while (b_pos < b_size)
				{
					// skip escapes
					if (buffer[b_pos] == '\\' && b_pos+1 < b_size)
					{
						b_pos += 2;
						continue;
					}

					if (buffer[b_pos] == '"')
					{
						// include trailing double quote
						b_pos++;
						break;
					}

					b_pos++;
				}

				return Udmf_Token(inst, buffer+start, b_pos - start);
			}

			// is it a identifier or number?
			if (isalnum(ch) || ch == '_' || ch == '-' || ch == '+')
			{
				b_pos++;

				while (b_pos < b_size)
				{
					char ch = buffer[b_pos];
					if (isalnum(ch) || ch == '_' || ch == '-' || ch == '+' || ch == '.')
					{
						b_pos++;
						continue;
					}
					break;
				}

				return Udmf_Token(inst, buffer+start, b_pos - start);
			}

			// it must be a symbol, such as '{' or '}'
			b_pos++;

			return Udmf_Token(inst, buffer+start, 1);
		}
	}

	bool Expect(const char *name)
	{
		Udmf_Token tok = Next();
		return tok.Match(name);
	}

	void SkipToEOLN()
	{
		while (b_pos < b_size && buffer[b_pos] != '\n')
			b_pos++;
	}
};


static void UDMF_ParseGlobalVar(Instance &inst, Udmf_Parser& parser, Udmf_Token& name)
{
	Udmf_Token value = parser.Next();
	if (value.IsEOF())
	{
		// TODO mark error
		return;
	}
	if (!parser.Expect(";"))
	{
		// TODO mark error
		parser.SkipToEOLN();
		return;
	}

	if (name.Match("namespace"))
	{
		// TODO : check if namespace is supported by current port
		//        [ if not, show a dialog with some options ]

		inst.loaded.udmfNamespace = value.DecodeString();
	}
	else if (name.Match("ee_compat"))
	{
		// odd Eternity thing, ignore it
	}
	else
	{
		gLog.printf("skipping unknown global '%s' in UDMF\n", name.c_str());
	}
}


static void UDMF_ParseThingField(const Document &doc, Thing *T, Udmf_Token& field, Udmf_Token& value)
{
	// just ignore any setting with the "false" keyword
	if (value.Match("false"))
		return;

	// TODO hexen options

	// TODO strife options

	if (field.Match("x"))
		T->raw_x = value.DecodeCoord();
	else if (field.Match("y"))
		T->raw_y = value.DecodeCoord();
	else if (field.Match("height"))
		T->raw_h = value.DecodeCoord();
	else if (field.Match("type"))
		T->type = value.DecodeInt();
	else if (field.Match("angle"))
		T->angle = value.DecodeInt();

	else if (field.Match("id"))
		T->tid = value.DecodeInt();
	else if (field.Match("special"))
		T->special = value.DecodeInt();
	else if (field.Match("arg0"))
		T->arg1 = value.DecodeInt();
	else if (field.Match("arg1"))
		T->arg2 = value.DecodeInt();
	else if (field.Match("arg2"))
		T->arg3 = value.DecodeInt();
	else if (field.Match("arg3"))
		T->arg4 = value.DecodeInt();
	else if (field.Match("arg4"))
		T->arg5 = value.DecodeInt();

	else if (field.Match("skill2"))
		T->options |= MTF_Easy;
	else if (field.Match("skill3"))
		T->options |= MTF_Medium;
	else if (field.Match("skill4"))
		T->options |= MTF_Hard;
	else if (field.Match("ambush"))
		T->options |= MTF_Ambush;
	else if (field.Match("friend"))
		T->options |= MTF_Friend;
	else if (field.Match("single"))
		T->options &= ~MTF_Not_SP;
	else if (field.Match("coop"))
		T->options &= ~MTF_Not_COOP;
	else if (field.Match("dm"))
		T->options &= ~MTF_Not_DM;

	else
	{
		gLog.debugPrintf("thing #%d: unknown field '%s'\n", doc.numThings()-1, field.c_str());
	}
}

static void UDMF_ParseVertexField(const Document &doc, Vertex *V, Udmf_Token& field, Udmf_Token& value)
{
	if (field.Match("x"))
		V->raw_x = value.DecodeCoord();
	else if (field.Match("y"))
		V->raw_y = value.DecodeCoord();
	else
	{
		gLog.debugPrintf("vertex #%d: unknown field '%s'\n", doc.numVertices()-1, field.c_str());
	}
}

static void UDMF_ParseLinedefField(const Document &doc, LineDef *LD, Udmf_Token& field, Udmf_Token& value)
{
	// Note: vertex and sidedef numbers are validated later on

	// just ignore any setting with the "false" keyword
	if (value.Match("false"))
		return;

	// TODO hexen flags

	// TODO strife flags

	if (field.Match("v1"))
		LD->start = value.DecodeInt();
	else if (field.Match("v2"))
		LD->end = value.DecodeInt();
	else if (field.Match("sidefront"))
		LD->right = value.DecodeInt();
	else if (field.Match("sideback"))
		LD->left = value.DecodeInt();
	else if (field.Match("special"))
		LD->type = value.DecodeInt();

	else if (field.Match("arg0"))
		LD->tag = value.DecodeInt();
	else if (field.Match("arg1"))
		LD->arg2 = value.DecodeInt();
	else if (field.Match("arg2"))
		LD->arg3 = value.DecodeInt();
	else if (field.Match("arg3"))
		LD->arg4 = value.DecodeInt();
	else if (field.Match("arg4"))
		LD->arg5 = value.DecodeInt();

	else if (field.Match("blocking"))
		LD->flags |= MLF_Blocking;
	else if (field.Match("blockmonsters"))
		LD->flags |= MLF_BlockMonsters;
	else if (field.Match("twosided"))
		LD->flags |= MLF_TwoSided;
	else if (field.Match("dontpegtop"))
		LD->flags |= MLF_UpperUnpegged;
	else if (field.Match("dontpegbottom"))
		LD->flags |= MLF_LowerUnpegged;
	else if (field.Match("secret"))
		LD->flags |= MLF_Secret;
	else if (field.Match("blocksound"))
		LD->flags |= MLF_SoundBlock;
	else if (field.Match("dontdraw"))
		LD->flags |= MLF_DontDraw;
	else if (field.Match("mapped"))
		LD->flags |= MLF_Mapped;

	else if (field.Match("passuse"))
		LD->flags |= MLF_Boom_PassThru;

	else
	{
		gLog.debugPrintf("linedef #%d: unknown field '%s'\n", doc.numVertices() -1, field.c_str());
	}
}

static void UDMF_ParseSidedefField(const Document &doc, SideDef *SD, Udmf_Token& field, Udmf_Token& value)
{
	// Note: sector numbers are validated later on

	// TODO: consider how to handle "offsetx_top" (etc), if at all

	if (field.Match("sector"))
		SD->sector = value.DecodeInt();
	else if (field.Match("texturetop"))
		SD->upper_tex = value.DecodeTexture();
	else if (field.Match("texturebottom"))
		SD->lower_tex = value.DecodeTexture();
	else if (field.Match("texturemiddle"))
		SD->mid_tex = value.DecodeTexture();
	else if (field.Match("offsetx"))
		SD->x_offset = value.DecodeInt();
	else if (field.Match("offsety"))
		SD->y_offset = value.DecodeInt();
	else
	{
		gLog.debugPrintf("sidedef #%d: unknown field '%s'\n", doc.numVertices() -1, field.c_str());
	}
}

static void UDMF_ParseSectorField(const Document &doc, Sector *S, Udmf_Token& field, Udmf_Token& value)
{
	if (field.Match("heightfloor"))
		S->floorh = value.DecodeInt();
	else if (field.Match("heightceiling"))
		S->ceilh = value.DecodeInt();
	else if (field.Match("texturefloor"))
		S->floor_tex = value.DecodeTexture();
	else if (field.Match("textureceiling"))
		S->ceil_tex = value.DecodeTexture();
	else if (field.Match("lightlevel"))
		S->light = value.DecodeInt();
	else if (field.Match("special"))
		S->type = value.DecodeInt();
	else if (field.Match("id"))
		S->tag = value.DecodeInt();
	else
	{
		gLog.debugPrintf("sector #%d: unknown field '%s'\n", doc.numVertices() -1, field.c_str());
	}
}

static void UDMF_ParseObject(Document &doc, Udmf_Parser& parser, Udmf_Token& name)
{
	// create a new object of the specified type
	Objid kind;

	Thing   *new_T  = NULL;
	Vertex  *new_V  = NULL;
	LineDef *new_LD = NULL;
	SideDef *new_SD = NULL;
	Sector  *new_S  = NULL;

	if (name.Match("thing"))
	{
		kind = Objid(ObjType::things, 1);
		auto addedThing = std::make_unique<Thing>();
		addedThing->options = MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM;
		doc.things.push_back(std::move(addedThing));
		new_T = doc.things.back().get();
	}
	else if (name.Match("vertex"))
	{
		kind = Objid(ObjType::vertices, 1);
		auto addedVertex = std::make_unique<Vertex>();
		doc.vertices.push_back(std::move(addedVertex));
		new_V = doc.vertices.back().get();
	}
	else if (name.Match("linedef"))
	{
		kind = Objid(ObjType::linedefs, 1);
		auto addedLine = std::make_unique<LineDef>();
		doc.linedefs.push_back(std::move(addedLine));
		new_LD = doc.linedefs.back().get();
	}
	else if (name.Match("sidedef"))
	{
		kind = Objid(ObjType::sidedefs, 1);
		auto addedSide = std::make_unique<SideDef>();
		addedSide->mid_tex = BA_InternaliseString("-");
		addedSide->lower_tex = addedSide->mid_tex;
		addedSide->upper_tex = addedSide->mid_tex;
		doc.sidedefs.push_back(std::move(addedSide));
		new_SD = doc.sidedefs.back().get();
	}
	else if (name.Match("sector"))
	{
		kind = Objid(ObjType::sectors, 1);
		auto addedSector = std::make_unique<Sector>();
		addedSector->light = 160;
		doc.sectors.push_back(std::move(addedSector));
		new_S = doc.sectors.back().get();
	}

	if (!kind.valid())
	{
		// unknown object kind
		gLog.printf("skipping unknown block '%s' in UDMF\n", name.c_str());
	}

	for (;;)
	{
		Udmf_Token tok = parser.Next();
		if (tok.IsEOF())
			break;

		if (tok.Match("}"))
			break;

		if (! parser.Expect("="))
		{
			// TODO mark error
			parser.SkipToEOLN();
			continue;
		}

		Udmf_Token value = parser.Next();
		if (value.IsEOF())
			break;

		if (! parser.Expect(";"))
		{
			// TODO mark error
			parser.SkipToEOLN();
			continue;
		}

		if (new_T)
			UDMF_ParseThingField(doc, new_T, tok, value);

		if (new_V)
			UDMF_ParseVertexField(doc, new_V, tok, value);

		if (new_LD)
			UDMF_ParseLinedefField(doc, new_LD, tok, value);

		if (new_SD)
			UDMF_ParseSidedefField(doc, new_SD, tok, value);

		if (new_S)
			UDMF_ParseSectorField(doc, new_S, tok, value);
	}
}


void Instance::ValidateLevel_UDMF()
{
	for (int n = 0 ; n < level.numSidedefs() ; n++)
	{
		ValidateSectorRef(level.sidedefs[n].get(), n);
	}

	for (int n = 0 ; n < level.numLinedefs(); n++)
	{
		auto &L = level.linedefs[n];

		ValidateVertexRefs(L.get(), n);
		ValidateSidedefRefs(L.get(), n);
	}
}


void Instance::UDMF_LoadLevel(const Wad_file *load_wad)
{
	Lump_c *lump = Load_LookupAndSeek(load_wad, "TEXTMAP");
	// we assume this cannot happen
	if (! lump)
		return;

	// TODO: reduce stack!!
	Udmf_Parser parser(*this, lump);

	for (;;)
	{
		Udmf_Token tok = parser.Next();
		if (tok.IsEOF())
			break;

		if (! tok.IsIdentifier())
		{
			// something has gone wrong
			// TODO mark the error somehow, pop-up dialog later
			parser.SkipToEOLN();
			continue;
		}

		Udmf_Token tok2 = parser.Next();
		if (tok2.IsEOF())
			break;

		if (tok2.Match("="))
		{
			UDMF_ParseGlobalVar(*this, parser, tok);
			continue;
		}
		if (tok2.Match("{"))
		{
			UDMF_ParseObject(level, parser, tok);
			continue;
		}

		// unexpected symbol
		// TODO mark the error somehow, show dialog later
		parser.SkipToEOLN();
	}

	ValidateLevel_UDMF();
}


//----------------------------------------------------------------------

static inline void WrFlag(Lump_c *lump, int flags, const char *name, int mask)
{
	if ((flags & mask) != 0)
	{
		lump->Printf("%s = true;\n", name);
	}
}

static void UDMF_WriteInfo(const Instance &inst, Lump_c *lump)
{
	lump->Printf("namespace = \"%s\";\n\n", inst.loaded.udmfNamespace.c_str());
}

static void UDMF_WriteThings(const Instance &inst, Lump_c *lump)
{
	for (int i = 0 ; i < inst.level.numThings() ; i++)
	{
		lump->Printf("thing // %d\n", i);
		lump->Printf("{\n");

		const auto &th = inst.level.things[i];

		lump->Printf("x = %1.3f;\n", th->x());
		lump->Printf("y = %1.3f;\n", th->y());

		if (th->raw_h != FFixedPoint{})
			lump->Printf("height = %1.3f;\n", th->h());

		lump->Printf("angle = %d;\n", th->angle);
		lump->Printf("type = %d;\n", th->type);

		// thing options
		WrFlag(lump, th->options, "skill1", MTF_Easy);
		WrFlag(lump, th->options, "skill2", MTF_Easy);
		WrFlag(lump, th->options, "skill3", MTF_Medium);
		WrFlag(lump, th->options, "skill4", MTF_Hard);
		WrFlag(lump, th->options, "skill5", MTF_Hard);

		WrFlag(lump, ~ th->options, "single", MTF_Not_SP);
		WrFlag(lump, ~ th->options, "coop",   MTF_Not_COOP);
		WrFlag(lump, ~ th->options, "dm",     MTF_Not_DM);

		WrFlag(lump, th->options, "ambush", MTF_Ambush);

		if (inst.conf.features.friend_flag)
			WrFlag(lump, th->options, "friend", MTF_Friend);

		// TODO Hexen flags

		// TODO Strife flags

		// TODO Hexen special and args

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteVertices(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numVertices(); i++)
	{
		lump->Printf("vertex // %d\n", i);
		lump->Printf("{\n");

		const auto &vert = doc.vertices[i];

		lump->Printf("x = %1.3f;\n", vert->x());
		lump->Printf("y = %1.3f;\n", vert->y());

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteLineDefs(const Instance &inst, Lump_c *lump)
{
	for (int i = 0 ; i < inst.level.numLinedefs(); i++)
	{
		lump->Printf("linedef // %d\n", i);
		lump->Printf("{\n");

		const auto &ld = inst.level.linedefs[i];

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

		if (inst.conf.features.pass_through)
			WrFlag(lump, ld->flags, "passuse", MLF_Boom_PassThru);

		if (inst.conf.features.midtex_3d)
			WrFlag(lump, ld->flags, "midtex3d", MLF_Eternity_3DMidTex);

		// TODO : hexen stuff (SPAC flags, etc)

		// TODO : strife stuff

		// TODO : zdoom stuff

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteSideDefs(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numSidedefs(); i++)
	{
		lump->Printf("sidedef // %d\n", i);
		lump->Printf("{\n");

		const auto &side = doc.sidedefs[i];

		lump->Printf("sector = %d;\n", side->sector);

		if (side->x_offset != 0)
			lump->Printf("offsetx = %d;\n", side->x_offset);
		if (side->y_offset != 0)
			lump->Printf("offsety = %d;\n", side->y_offset);

		// use NormalizeTex to ensure no double quote

		if (side->UpperTex() != "-")
			lump->Printf("texturetop = \"%s\";\n", NormalizeTex(side->UpperTex()).c_str());
		if (side->LowerTex() != "-")
			lump->Printf("texturebottom = \"%s\";\n", NormalizeTex(side->LowerTex()).c_str());
		if (side->MidTex() != "-")
			lump->Printf("texturemiddle = \"%s\";\n", NormalizeTex(side->MidTex()).c_str());

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteSectors(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numSectors(); i++)
	{
		lump->Printf("sector // %d\n", i);
		lump->Printf("{\n");

		const auto &sec = doc.sectors[i];

		lump->Printf("heightfloor = %d;\n", sec->floorh);
		lump->Printf("heightceiling = %d;\n", sec->ceilh);

		// use NormalizeTex to ensure no double quote

		lump->Printf("texturefloor = \"%s\";\n", NormalizeTex(sec->FloorTex()).c_str());
		lump->Printf("textureceiling = \"%s\";\n", NormalizeTex(sec->CeilTex()).c_str());

		lump->Printf("lightlevel = %d;\n", sec->light);
		if (sec->type != 0)
			lump->Printf("special = %d;\n", sec->type);
		if (sec->tag != 0)
			lump->Printf("id = %d;\n", sec->tag);

		lump->Printf("}\n\n");
	}
}

void Instance::UDMF_SaveLevel() const
{
	Lump_c &lump = wad.master.edit_wad->AddLump("TEXTMAP");

	UDMF_WriteInfo(*this, &lump);
	UDMF_WriteThings(*this, &lump);
	UDMF_WriteVertices(level, &lump);
	UDMF_WriteLineDefs(*this, &lump);
	UDMF_WriteSideDefs(level, &lump);
	UDMF_WriteSectors(level, &lump);

	wad.master.edit_wad->AddLump("ENDMAP");
}


//----------------------------------------------------------------------

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
