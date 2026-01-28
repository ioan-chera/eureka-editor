//------------------------------------------------------------------------
//  UDMF PARSING / WRITING
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2019 Andrew Apted
//  Copyright (C) 2025-2026 Ioan Chera
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
#include "m_files.h"
#include "m_udmf.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "w_wad.h"

#include "ui_file.h"
#include "ui_window.h"

#include <array>
#include <initializer_list>

#define UDMF_LINE_MAPPING(name) UDMFMapping{ #name, UDMFMapping::Category::line, 1, static_cast<unsigned>(MLF_UDMF_ ## name) }
#define UDMF_LINE_MAPPING2(name) UDMFMapping{#name, UDMFMapping::Category::line, 2, static_cast<unsigned>(MLF2_UDMF_ ## name) }
#define UDMF_THING_MAPPING(name) UDMFMapping{#name, UDMFMapping::Category::thing, 1, static_cast<unsigned>(MTF_UDMF_ ## name) }

static constexpr UDMFMapping kUDMFMapping[] = {
	UDMFMapping{ "blocking",      UDMFMapping::Category::line, 1, (unsigned)MLF_Blocking },
	UDMFMapping{ "blockmonsters", UDMFMapping::Category::line, 1, (unsigned)MLF_BlockMonsters },
	UDMFMapping{ "twosided",      UDMFMapping::Category::line, 1, (unsigned)MLF_TwoSided },
	UDMFMapping{ "dontpegtop",    UDMFMapping::Category::line, 1, (unsigned)MLF_UpperUnpegged },
	UDMFMapping{ "dontpegbottom", UDMFMapping::Category::line, 1, (unsigned)MLF_LowerUnpegged },
	UDMFMapping{ "secret",        UDMFMapping::Category::line, 1, (unsigned)MLF_Secret },
	UDMFMapping{ "blocksound",    UDMFMapping::Category::line, 1, (unsigned)MLF_SoundBlock },
	UDMFMapping{ "dontdraw",      UDMFMapping::Category::line, 1, (unsigned)MLF_DontDraw },
	UDMFMapping{ "mapped",        UDMFMapping::Category::line, 1, (unsigned)MLF_Mapped },
	UDMFMapping{ "passuse",       UDMFMapping::Category::line, 1, (unsigned)MLF_Boom_PassThru },
	UDMF_LINE_MAPPING(Translucent),
	UDMF_LINE_MAPPING(Translucent  ),
	UDMF_LINE_MAPPING(Transparent  ),
	UDMF_LINE_MAPPING(JumpOver     ),
	UDMF_LINE_MAPPING(BlockFloaters),
	UDMF_LINE_MAPPING(PlayerCross  ),
	UDMF_LINE_MAPPING(PlayerUse    ),
	UDMF_LINE_MAPPING(MonsterCross ),
	UDMF_LINE_MAPPING(MonsterUse   ),
	UDMF_LINE_MAPPING(Impact       ),
	UDMF_LINE_MAPPING(PlayerPush   ),
	UDMF_LINE_MAPPING(MonsterPush  ),
	UDMF_LINE_MAPPING(MissileCross ),
	UDMF_LINE_MAPPING(RepeatSpecial),
	UDMF_LINE_MAPPING(BlockPlayers     ),
	UDMF_LINE_MAPPING(BlockLandMonsters),
	UDMF_LINE_MAPPING(BlockProjectiles ),
	UDMF_LINE_MAPPING(BlockHitScan     ),
	UDMF_LINE_MAPPING(BlockUse         ),
	UDMF_LINE_MAPPING(BlockSight       ),
	UDMF_LINE_MAPPING2(BlockEverything   ),
	UDMF_LINE_MAPPING2(Revealed          ),
	UDMF_LINE_MAPPING2(AnyCross          ),
	UDMF_LINE_MAPPING2(MonsterActivate   ),
	UDMF_LINE_MAPPING2(PlayerUseBack     ),
	UDMF_LINE_MAPPING2(FirstSideOnly     ),
	UDMF_LINE_MAPPING2(CheckSwitchRange  ),
	UDMF_LINE_MAPPING2(ClipMidTex        ),
	UDMF_LINE_MAPPING2(WrapMidTex        ),
	UDMFMapping{"midtex3d", UDMFMapping::Category::line, 1, (unsigned)MLF_Eternity_3DMidTex},
	UDMF_LINE_MAPPING2(MidTex3DImpassible),
	UDMF_LINE_MAPPING2(DamageSpecial     ),
	UDMF_LINE_MAPPING2(DeathSpecial      ),

// Things
	{ "skill1",      UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Easiest },
	{ "skill2",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Easy },
	{ "skill3",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Medium },
	{ "skill4",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Hard },
	{ "skill5",      UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Hardest },
	{ "ambush",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Ambush },
	{ "friend",      UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Friend },
	{ "dormant",     UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_Dormant },
	{ "single",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_SP },
	{ "dm",          UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_DM },
	{ "coop",        UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_COOP },
	{ "class1",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_Fighter },
	{ "class2",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_Cleric },
	{ "class3",      UDMFMapping::Category::thing, 1, (unsigned)MTF_Hexen_Mage },
	{ "standing",    UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Standing },
	{ "strifeally",  UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_StrifeAlly },
	{ "translucent", UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Translucent },
	{ "invisible",   UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_Invisible },
	{ "countsecret", UDMFMapping::Category::thing, 1, (unsigned)MTF_UDMF_CountSecret },
};

// Returns false on invalid string
bool UDMF_AddLineFeature(ConfigData &config, const char *featureName)
{
	struct FeatureMap
	{
		const char *name;
		UDMF_LineFeature flagIndex;
	};

#define ENTRY(a) { #a, UDMF_LineFeature::a }
	static const FeatureMap map[] =
	{
		ENTRY(anycross),
		ENTRY(blockeverything),
		ENTRY(blockfloaters),
		ENTRY(blockhitscan),
		ENTRY(blocking),
		ENTRY(blocklandmonsters),
		ENTRY(blockmonsters),
		ENTRY(blockplayers),
		ENTRY(blockprojectiles),
		ENTRY(blocksight),
		ENTRY(blocksound),
		ENTRY(blockuse),
		ENTRY(checkswitchrange),
		ENTRY(damagespecial),
		ENTRY(deathspecial),
		ENTRY(dontpegbottom),
		ENTRY(dontpegtop),
		ENTRY(firstsideonly),
		ENTRY(impact),
		ENTRY(jumpover),
		ENTRY(midtex3d),
		ENTRY(midtex3dimpassible),
		ENTRY(missilecross),
		ENTRY(monsteractivate),
		ENTRY(monstercross),
		ENTRY(monsterpush),
		ENTRY(monsteruse),
		ENTRY(passuse),
		ENTRY(playercross),
		ENTRY(playerpush),
		ENTRY(playeruse),
		ENTRY(playeruseback),
		ENTRY(repeatspecial),
		ENTRY(twosided),
	};
#undef ENTRY

	for(const FeatureMap &entry : map)
	{
		if (y_stricmp(entry.name, featureName) == 0)
		{
			config.udmfLineFeatures |= (1ULL << (uint64_t)entry.flagIndex);
			return true;
		}
	}
	return false;
}

bool UDMF_HasLineFeature(const ConfigData &config, UDMF_LineFeature feature)
{
	return (config.udmfLineFeatures & (1ULL << (uint64_t)feature)) != 0;
}

class Udmf_Token
{
private:
	// empty means EOF
	SString text;

public:
	Udmf_Token(const char *str) : text(str)
	{ }

	Udmf_Token(const char *str, int len) : text(str, len)
	{ }

	const char *c_str() const
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

		return safe_isalpha(ch) || ch == '_';
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

		for(size_t pos = string.find('\\'); pos != std::string::npos;
			pos = string.find('\\', pos + 1))
		{
			string.erase(pos, 1);
		}

		return string;
	}

	FFixedPoint DecodeCoord() const
	{
		return MakeValidCoord(MapFormat::udmf, DecodeFloat());
	}

	StringID DecodeTexture() const
	{
		SString buffer = DecodeString();

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
	LumpInputStream stream;

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

	int line = 0;

public:
	void throwException(const char *format, ...) const
	{
		SString message;
		va_list ap;
		va_start(ap, format);
		message = SString::vprintf(format, ap);
		va_end(ap);
		throw std::runtime_error(SString::printf("UDMF error at line %d: %s", line,
												 message.c_str()).get());
	}

	int getLine() const
	{
		return line;
	}

	Udmf_Parser(const Lump_c &_lump) : stream(_lump)
	{
		remaining = _lump.Length();
	}

	Udmf_Token Next()
	{
		for (;;)
		{
			if (done)
				return Udmf_Token("");

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

				if (! stream.read(buffer + b_size, want))
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
				if(b_pos == '\n')
					line++;
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
				if(ch == '\n')
					line++;
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
						if(buffer[b_pos+1] == '\n')
							line++;
						b_pos += 2;
						continue;
					}

					if (buffer[b_pos] == '"')
					{
						// include trailing double quote
						b_pos++;
						break;
					}

					if(buffer[b_pos] == '\n')
						line++;

					b_pos++;
				}

				return Udmf_Token(buffer+start, b_pos - start);
			}

			// is it a identifier or number?
			if (safe_isalnum(ch) || ch == '_' || ch == '-' || ch == '+')
			{
				b_pos++;

				while (b_pos < b_size)
				{
					char ch = buffer[b_pos];
					if (safe_isalnum(ch) || ch == '_' || ch == '-' || ch == '+' || ch == '.')
					{
						b_pos++;
						continue;
					}
					break;
				}

				return Udmf_Token(buffer+start, b_pos - start);
			}

			// it must be a symbol, such as '{' or '}'
			assert(buffer[b_pos] != '\n');
			b_pos++;

			return Udmf_Token(buffer+start, 1);
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
		if(b_pos < b_size)	// so it's '\n'
			line++;
	}
};

const UDMFMapping *UDMFMapping::getForName(const char *name, Category category)
{
	for(const UDMFMapping &mapping : kUDMFMapping)
		if(!y_stricmp(name, mapping.name) && mapping.category == category)
			return &mapping;
	return nullptr;
}

static std::optional<ConfigData> UDMF_ParseGlobalVar(const Instance &inst, LoadingData &loading, Udmf_Parser& parser, const Udmf_Token& name)
{
	Udmf_Token value = parser.Next();
	if (value.IsEOF())
	{
		parser.throwException("expected value for global setting %s, reached end of text map",
							  name.c_str());
		return std::nullopt;
	}
	if (!parser.Expect(";"))
	{
		parser.throwException("expected semicolon");
		return std::nullopt;
	}

	if (name.Match("namespace"))
	{
		loading.udmfNamespace = value.DecodeString();

		// Check if the namespace matches the current port+game configuration
		const std::vector<PortGamePair> &pairs = M_FindPortGameForUDMFNamespace(loading.udmfNamespace);
		if (!pairs.empty())
		{
			// Check if current port+game matches any registered pair for this namespace
			bool foundMatch = false;
			for (const PortGamePair &pair : pairs)
			{
				if (pair.portName == loading.portName && pair.gameName == loading.gameName)
				{
					foundMatch = true;
					break;
				}
			}

			// If no match found, show dialog for user to select port/game pair
			if (!foundMatch)
			{
				gLog.printf("UDMF namespace '%s' doesn't match current port '%s' / game '%s'\n",
							loading.udmfNamespace.c_str(), loading.portName.c_str(), loading.gameName.c_str());

				std::optional<PortGamePair> selectedPair;
				if(pairs.size() > 1)
				{
					UI_UDMFSetup dialog(loading.udmfNamespace, loading.levelName, pairs);
					selectedPair = dialog.Run();

					if(selectedPair.has_value())
					{
						gLog.printf("User selected: port '%s' / game '%s'\n",
							selectedPair->portName.c_str(), selectedPair->gameName.c_str());
					}
					else
					{
						gLog.printf("No selection made, keeping current port '%s' / game '%s'\n",
							loading.portName.c_str(), loading.gameName.c_str());
						return std::nullopt;
					}
				}
				else
					selectedPair = pairs[0];

				loading.portName = selectedPair->portName;
				SString oldGameName = loading.gameName;
				loading.gameName = selectedPair->gameName;

				if (loading.gameName != oldGameName)
				{
					const fs::path *iwadPath = global::recent.queryIWAD(loading.gameName);
					if (iwadPath)
						loading.iwadName = *iwadPath;
					else
						gLog.printf("Warning: could not find IWAD for game '%s'\n", loading.gameName.c_str());
				}

				// Load the new ConfigData for the changed port/game
				return loadConfigOnly(loading);
			}
		}
	}
	else if (name.Match("ee_compat"))
	{
		// odd Eternity thing, ignore it
	}
	else
	{
		gLog.printf("skipping unknown global '%s' in UDMF\n", name.c_str());
	}

	return std::nullopt;
}


static void UDMF_ParseThingField(const Document &doc, Thing *T, const Udmf_Token& field, const Udmf_Token& value)
{
	// just ignore any setting with the "false" keyword
	if (value.Match("false"))
		return;

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

	else
	{
		for(const UDMFMapping &mapping : kUDMFMapping)
			if(field.Match(mapping.name) && mapping.category == UDMFMapping::Category::thing)
			{
				T->options |= mapping.value;
				return;
			}
		gLog.debugPrintf("thing #%d: unknown field '%s'\n", doc.numThings()-1, field.c_str());
	}
}

static void UDMF_ParseVertexField(const Document &doc, Vertex *V, const Udmf_Token& field, const Udmf_Token& value)
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

static std::set<int> parseMoreIDs(const char *string)
{
	long number;
	char *endptr;
	std::set<int> result;

	bool afterNumber = false;
	for(const char *c = string; *c; ++c)
	{
		if(safe_isspace(*c))
		{
			afterNumber = false;
			continue;
		}
		if(afterNumber)
			continue;
		number = strtol(c, &endptr, 10);
		if(c == endptr)
		{
			afterNumber = true;	// no longer valid
			continue;
		}
		c = endptr - 1;
		result.insert((int)number);
		afterNumber = true;
	}

	return result;
}

static void UDMF_ParseLinedefField(const Document &doc, LineDef *LD, const Udmf_Token& field,
	const Udmf_Token& value)
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

	// TODO: separate id from arg0 for UDMF only
	else if (field.Match("id"))
		LD->tag = value.DecodeInt();
	else if (field.Match("arg0"))
		LD->arg1 = value.DecodeInt();
	else if (field.Match("arg1"))
		LD->arg2 = value.DecodeInt();
	else if (field.Match("arg2"))
		LD->arg3 = value.DecodeInt();
	else if (field.Match("arg3"))
		LD->arg4 = value.DecodeInt();
	else if (field.Match("arg4"))
		LD->arg5 = value.DecodeInt();
	else if (field.Match("arg0str"))
		LD->arg1str = BA_InternaliseString(value.DecodeString());
	else if (field.Match("locknumber"))
		LD->locknumber = value.DecodeInt();
	else if (field.Match("automapstyle"))
		LD->automapstyle = value.DecodeInt();
	else if (field.Match("health"))
		LD->health = value.DecodeInt();
	else if (field.Match("healthgroup"))
		LD->healthgroup = value.DecodeInt();
	else if (field.Match("alpha"))
		LD->alpha = value.DecodeFloat();
	else if (field.Match("moreids"))
		LD->moreIDs = parseMoreIDs(value.DecodeString().c_str());
	else
	{
		// Flags
		for(const UDMFMapping &mapping : kUDMFMapping)
			if(field.Match(mapping.name) && mapping.category == UDMFMapping::Category::line)
			{
				(mapping.flagSet == 1 ? LD->flags : LD->flags2) |= mapping.value;
				return;
			}
		gLog.debugPrintf("linedef #%d: unknown field '%s'\n", doc.numVertices() -1, field.c_str());
	}
}

static void UDMF_ParseSidedefField(const Document &doc, SideDef *SD, const Udmf_Token& field, const Udmf_Token& value)
{
	// Note: sector numbers are validated later on

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

	// Per-part offsets
	else if (field.Match("offsetx_top"))
		SD->offsetx_top = value.DecodeFloat();
	else if (field.Match("offsety_top"))
		SD->offsety_top = value.DecodeFloat();
	else if (field.Match("offsetx_mid"))
		SD->offsetx_mid = value.DecodeFloat();
	else if (field.Match("offsety_mid"))
		SD->offsety_mid = value.DecodeFloat();
	else if (field.Match("offsetx_bottom"))
		SD->offsetx_bottom = value.DecodeFloat();
	else if (field.Match("offsety_bottom"))
		SD->offsety_bottom = value.DecodeFloat();

	// Per-part scales
	else if (field.Match("scalex_top"))
		SD->scalex_top = value.DecodeFloat();
	else if (field.Match("scaley_top"))
		SD->scaley_top = value.DecodeFloat();
	else if (field.Match("scalex_mid"))
		SD->scalex_mid = value.DecodeFloat();
	else if (field.Match("scaley_mid"))
		SD->scaley_mid = value.DecodeFloat();
	else if (field.Match("scalex_bottom"))
		SD->scalex_bottom = value.DecodeFloat();
	else if (field.Match("scaley_bottom"))
		SD->scaley_bottom = value.DecodeFloat();

	else if(field.Match("light_top"))
		SD->light_top = value.DecodeInt();
	else if(field.Match("light_mid"))
		SD->light_mid = value.DecodeInt();
	else if(field.Match("light_bottom"))
		SD->light_bottom = value.DecodeInt();
	else if(field.Match("lightabsolute_top") && !value.Match("false"))
		SD->flags |= SideDef::FLAG_LIGHT_ABSOLUTE_TOP;
	else if(field.Match("lightabsolute_mid") && !value.Match("false"))
		SD->flags |= SideDef::FLAG_LIGHT_ABSOLUTE_MID;
	else if(field.Match("lightabsolute_bottom") && !value.Match("false"))
		SD->flags |= SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM;

	else
	{
		gLog.debugPrintf("sidedef #%d: unknown field '%s'\n", doc.numSidedefs() - 1, field.c_str());
	}
}

static void UDMF_ParseSectorField(const Document &doc, Sector *S, const Udmf_Token& field, const Udmf_Token& value)
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

static void UDMF_ParseObject(Document &doc, Udmf_Parser& parser, const Udmf_Token& name)
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
		auto addedLine = std::make_shared<LineDef>();
		doc.linedefs.push_back(std::move(addedLine));
		new_LD = doc.linedefs.back().get();
	}
	else if (name.Match("sidedef"))
	{
		kind = Objid(ObjType::sidedefs, 1);
		auto addedSide = std::make_shared<SideDef>();
		addedSide->mid_tex = BA_InternaliseString("-");
		addedSide->lower_tex = addedSide->mid_tex;
		addedSide->upper_tex = addedSide->mid_tex;
		doc.sidedefs.push_back(std::move(addedSide));
		new_SD = doc.sidedefs.back().get();
	}
	else if (name.Match("sector"))
	{
		kind = Objid(ObjType::sectors, 1);
		auto addedSector = std::make_shared<Sector>();
		addedSector->light = 160;
		doc.sectors.push_back(std::move(addedSector));
		new_S = doc.sectors.back().get();
	}

	if (!kind.valid())
	{
		// unknown object kind
		gLog.printf("skipping unknown block '%s' in UDMF at line %d\n", name.c_str(),
					parser.getLine());
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
			parser.throwException("expected equal sign");
			continue;
		}

		Udmf_Token value = parser.Next();
		if (value.IsEOF())
			break;

		if (! parser.Expect(";"))
		{
			parser.throwException("expected semicolon");
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


void Document::ValidateLevel_UDMF(const ConfigData &config, BadCount &bad)
{
	for (int n = 0 ; n < numSidedefs() ; n++)
	{
		ValidateSectorRef(*sidedefs[n], n, config, bad);
	}

	for (int n = 0 ; n < numLinedefs(); n++)
	{
		auto L = linedefs[n];

		ValidateVertexRefs(*L, n, bad);
		ValidateSidedefRefs(*L, n, config, bad);
	}
}


void Document::UDMF_LoadLevel(int loading_level, const Wad_file *load_wad, LoadingData &loading,
							  const ConfigData &config, BadCount &bad)
{
	const Lump_c *lump = Load_LookupAndSeek(loading_level, load_wad, "TEXTMAP");
	if (! lump)
		ThrowLogicException("%s: couldn't find TEXTMAP lump!", __func__);

	// NOTE: this must be a pointer to heap, due to stack size.
	auto parser = std::make_unique<Udmf_Parser>(*lump);

	// Config to use for parsing - may be overridden if namespace changes port/game
	std::optional<ConfigData> overrideConfig;
	const ConfigData *activeConfig = &config;

	for (;;)
	{
		Udmf_Token tok = parser->Next();
		if (tok.IsEOF())
			break;

		if (! tok.IsIdentifier())
		{
			// something has gone wrong
			parser->throwException("expected identifier, got %s instead", tok.c_str());
			continue;
		}

		Udmf_Token tok2 = parser->Next();
		if (tok2.IsEOF())
			break;

		if (tok2.Match("="))
		{
			std::optional<ConfigData> newConfig = UDMF_ParseGlobalVar(inst, loading, *parser, tok);
			if (newConfig.has_value())
			{
				overrideConfig = std::move(newConfig);
				activeConfig = &overrideConfig.value();
			}
			continue;
		}
		if (tok2.Match("{"))
		{
			UDMF_ParseObject(*this, *parser, tok);
			continue;
		}

		// unexpected symbol
		parser->throwException("unexpected symbol %s", tok2.c_str());
	}

	ValidateLevel_UDMF(*activeConfig, bad);
}


//----------------------------------------------------------------------

static inline void WrFlag(Lump_c *lump, int flags, const char *name, int mask)
{
	if ((flags & mask) != 0)
	{
		lump->Printf("%s = true;\n", name);
	}
}

static void UDMF_WriteInfo(const LoadingData &loading, Lump_c *lump)
{
	lump->Printf("namespace = \"%s\";\n\n", loading.udmfNamespace.c_str());
}

static void UDMF_WriteThings(const Instance &inst, Lump_c *lump)
{
	for (int i = 0 ; i < inst.level.numThings() ; i++)
	{
		lump->Printf("thing // %d\n", i);
		lump->Printf("{\n");

		const auto th = inst.level.things[i];

		lump->Printf("x = %0.16g;\n", th->x());
		lump->Printf("y = %0.16g;\n", th->y());

		if (th->raw_h != FFixedPoint{})
			lump->Printf("height = %0.16g;\n", th->h());

		lump->Printf("angle = %d;\n", th->angle);
		lump->Printf("type = %d;\n", th->type);

		// thing options
		for(const UDMFMapping& mapping : kUDMFMapping)
		{
			if (mapping.category != UDMFMapping::Category::thing)
				continue;
			WrFlag(lump, th->options, mapping.name, mapping.value);
		}

		if(inst.conf.features.udmf_thingspecials)
		{
			lump->Printf("special = %d;\n", th->special);
			lump->Printf("arg0 = %d;\n", th->arg1);
			lump->Printf("arg1 = %d;\n", th->arg2);
			lump->Printf("arg2 = %d;\n", th->arg3);
			lump->Printf("arg3 = %d;\n", th->arg4);
			lump->Printf("arg4 = %d;\n", th->arg5);
		}

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteVertices(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numVertices(); i++)
	{
		lump->Printf("vertex // %d\n", i);
		lump->Printf("{\n");

		const auto vert = doc.vertices[i];

		lump->Printf("x = %0.16g;\n", vert->x());
		lump->Printf("y = %0.16g;\n", vert->y());

		lump->Printf("}\n\n");
	}
}

static SString EncodeString(const SString &raw)
{
	SString result = raw;
	// Escape all backslashes and quotes
	size_t pos = std::string::npos;
	while((pos = result.find('\\', pos == std::string::npos ? 0 : pos + 2)) != std::string::npos)
	{
		result.insert(pos, "\\");
	}
	pos = std::string::npos;
	while((pos = result.find('"', pos == std::string::npos ? 0 : pos + 2)) != std::string::npos)
	{
		result.insert(pos, "\\");
	}
	return "\"" + result + "\"";
}

static void UDMF_WriteLineDefs(const Instance &inst, Lump_c *lump)
{
	for (int i = 0 ; i < inst.level.numLinedefs(); i++)
	{
		lump->Printf("linedef // %d\n", i);
		lump->Printf("{\n");

		const auto ld = inst.level.linedefs[i];

		lump->Printf("v1 = %d;\n", ld->start);
		lump->Printf("v2 = %d;\n", ld->end);

		if (ld->right >= 0)
			lump->Printf("sidefront = %d;\n", ld->right);
		if (ld->left >= 0)
			lump->Printf("sideback = %d;\n", ld->left);

		if (ld->type != 0)
			lump->Printf("special = %d;\n", ld->type);

		if (ld->tag != 0)
			lump->Printf("id = %d;\n", ld->tag);
		if (ld->arg1 != 0)
			lump->Printf("arg0 = %d;\n", ld->arg1);
		if (ld->arg2 != 0)
			lump->Printf("arg1 = %d;\n", ld->arg2);
		if (ld->arg3 != 0)
			lump->Printf("arg2 = %d;\n", ld->arg3);
		if (ld->arg4 != 0)
			lump->Printf("arg3 = %d;\n", ld->arg4);
		if (ld->arg5 != 0)
			lump->Printf("arg4 = %d;\n", ld->arg5);
		if (ld->arg1str.get())
			lump->Printf("arg0str = %s\n", EncodeString(BA_GetString(ld->arg1str)).c_str());

		if (ld->locknumber != 0)
			lump->Printf("locknumber = %d;\n", ld->locknumber);

		if (ld->automapstyle != 0)
			lump->Printf("automapstyle = %d;\n", ld->automapstyle);

		if (ld->health != 0)
			lump->Printf("health = %d;\n", ld->health);

		if (ld->healthgroup != 0)
			lump->Printf("healthgroup = %d;\n", ld->healthgroup);

		if (ld->alpha != 1.0)
			lump->Printf("alpha = %0.16g;\n", ld->alpha);

		if (!ld->moreIDs.empty())
		{
			lump->Printf("moreids = \"");
			bool first = true;
			for(int id : ld->moreIDs)
			{
				if (!first)
					lump->Printf(" ");
				lump->Printf("%d", id);
				first = false;
			}
			lump->Printf("\";\n");
		}

		// linedef flags
		for(const UDMFMapping& mapping : kUDMFMapping)
		{
			if (mapping.category != UDMFMapping::Category::line)
				continue;
			WrFlag(lump, mapping.flagSet == 1 ? ld->flags : ld->flags2, mapping.name, mapping.value);
		}

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteSideDefs(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numSidedefs(); i++)
	{
		lump->Printf("sidedef // %d\n", i);
		lump->Printf("{\n");

		const auto side = doc.sidedefs[i];

		lump->Printf("sector = %d;\n", side->sector);

		if (side->x_offset != 0)
			lump->Printf("offsetx = %d;\n", side->x_offset);
		if (side->y_offset != 0)
			lump->Printf("offsety = %d;\n", side->y_offset);

		if (side->UpperTex() != "-")
			lump->Printf("texturetop = %s;\n", EncodeString(NormalizeTex(side->UpperTex())).c_str());
		if (side->LowerTex() != "-")
			lump->Printf("texturebottom = %s;\n", EncodeString(NormalizeTex(side->LowerTex())).c_str());
		if (side->MidTex() != "-")
			lump->Printf("texturemiddle = %s;\n", EncodeString(NormalizeTex(side->MidTex())).c_str());

		if (side->offsetx_top)
			lump->Printf("offsetx_top = %.16g;\n", side->offsetx_top);
		if (side->offsetx_mid)
			lump->Printf("offsetx_mid = %.16g;\n", side->offsetx_mid);
		if (side->offsetx_bottom)
			lump->Printf("offsetx_bottom = %.16g;\n", side->offsetx_bottom);
		if (side->offsety_top)
			lump->Printf("offsety_top = %.16g;\n", side->offsety_top);
		if (side->offsety_mid)
			lump->Printf("offsety_mid = %.16g;\n", side->offsety_mid);
		if (side->offsety_bottom)
			lump->Printf("offsety_bottom = %.16g;\n", side->offsety_bottom);
		if (side->scalex_top != 1)
			lump->Printf("scalex_top = %.16g;\n", side->scalex_top);
		if (side->scalex_mid != 1)
			lump->Printf("scalex_mid = %.16g;\n", side->scalex_mid);
		if (side->scalex_bottom != 1)
			lump->Printf("scalex_bottom = %.16g;\n", side->scalex_bottom);
		if (side->scaley_top != 1)
			lump->Printf("scaley_top = %.16g;\n", side->scaley_top);
		if (side->scaley_mid != 1)
			lump->Printf("scaley_mid = %.16g;\n", side->scaley_mid);
		if (side->scaley_bottom != 1)
			lump->Printf("scaley_bottom = %.16g;\n", side->scaley_bottom);
		if (side->light_top)
			lump->Printf("light_top = %d;\n", side->light_top);
		if (side->light_mid)
			lump->Printf("light_mid = %d;\n", side->light_mid);
		if (side->light_bottom)
			lump->Printf("light_bottom = %d;\n", side->light_bottom);
		if (side->flags & SideDef::FLAG_LIGHT_ABSOLUTE_TOP)
			lump->Printf("lightabsolute_top = true;\n");
		if (side->flags & SideDef::FLAG_LIGHT_ABSOLUTE_MID)
			lump->Printf("lightabsolute_mid = true;\n");
		if (side->flags & SideDef::FLAG_LIGHT_ABSOLUTE_BOTTOM)
			lump->Printf("lightabsolute_bottom = true;\n");

		lump->Printf("}\n\n");
	}
}

static void UDMF_WriteSectors(const Document &doc, Lump_c *lump)
{
	for (int i = 0 ; i < doc.numSectors(); i++)
	{
		lump->Printf("sector // %d\n", i);
		lump->Printf("{\n");

		const auto sec = doc.sectors[i];

		lump->Printf("heightfloor = %d;\n", sec->floorh);
		lump->Printf("heightceiling = %d;\n", sec->ceilh);

		lump->Printf("texturefloor = %s;\n", EncodeString(NormalizeTex(sec->FloorTex())).c_str());
		lump->Printf("textureceiling = %s;\n", EncodeString(NormalizeTex(sec->CeilTex())).c_str());

		lump->Printf("lightlevel = %d;\n", sec->light);
		if (sec->type != 0)
			lump->Printf("special = %d;\n", sec->type);
		if (sec->tag != 0)
			lump->Printf("id = %d;\n", sec->tag);

		lump->Printf("}\n\n");
	}
}

void Instance::UDMF_SaveLevel(const LoadingData& loading, Wad_file& wad) const
{
	Lump_c &lump = wad.AddLump("TEXTMAP");

	UDMF_WriteInfo(loading, &lump);
	UDMF_WriteThings(*this, &lump);
	UDMF_WriteVertices(level, &lump);
	UDMF_WriteLineDefs(*this, &lump);
	UDMF_WriteSideDefs(level, &lump);
	UDMF_WriteSectors(level, &lump);

	wad.AddLump("ENDMAP");
}



//----------------------------------------------------------------------

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
