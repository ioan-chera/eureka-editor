//------------------------------------------------------------------------
//  STRING TABLE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#include "m_strings.h"


#define CHARS_PER_BLOCK  4096

class string_block_c
{
public:
	char data[CHARS_PER_BLOCK];

	int used;

public:
	 string_block_c() : used(0) { }
	~string_block_c() { }

	bool fits(int len) const
	{
		return (used + len + 1) < CHARS_PER_BLOCK;
	}

	int add(const char *str, int len)
	{
		SYS_ASSERT(fits(len));

		int offset = used;  used += (len+1);

		memcpy(data + offset, str, len+1);

		return offset;
	}

	int find(const char *str, int len) const
	{
		int pos = 0;

		while (pos < used)
		{
			int p_len = (int)strlen(data + pos);

			// we allow matching the end of existing strings
			if (len <= p_len)
			{
				if (memcmp(data + pos + (p_len-len), str, len) == 0)
					return pos + (p_len-len);
			}

			pos += (p_len+1);
		}

		return -1;  // NOT FOUND
	}
};


//------------------------------------------------------------------------


string_table_c::string_table_c() : blocks(), huge_ones()
{
	// nothing needed
}

string_table_c::~string_table_c()
{
	clear();
}

int string_table_c::add(const char *str)
{
	if (!str || !str[0])
		return 0;

	int len = (int)strlen(str);

	if (len > CHARS_PER_BLOCK/5)
	{
		int offset = find_huge(str, len);

		return offset ? offset : add_huge(str, len);
	}

	int offset = find_normal(str, len);

	return offset ? offset : add_normal(str, len);
}

const char * string_table_c::get(int offset)
{
	if (offset == 0)
		return "";

	if (offset < 0)
	{
		offset = -(offset + 1);

		SYS_ASSERT(offset < (int)huge_ones.size());

		return huge_ones[offset];
	}

	offset--;

	int blk_num = offset / CHARS_PER_BLOCK;
	offset      = offset % CHARS_PER_BLOCK;

	SYS_ASSERT(blk_num < (int)blocks.size());

	return & blocks[blk_num]->data[offset];
}

void string_table_c::clear()
{
	for (int i = 0; i < (int)blocks.size(); i++)
		delete blocks[i];

	for (int i = 0; i < (int)huge_ones.size(); i++)
		delete[] huge_ones[i];

	blocks.clear();
	huge_ones.clear();
}

int string_table_c::find_normal(const char *str, int len)
{
	for (int blk_num = 0; blk_num < (int)blocks.size(); blk_num++)
	{
		int offset = blocks[blk_num]->find(str, len);

		if (offset != -1)
			return (blk_num * CHARS_PER_BLOCK) + offset + 1;
	}

	return 0; // not found
}

int string_table_c::add_normal(const char *str, int len)
{
	if (blocks.empty())
		blocks.push_back(new string_block_c);

	int blk_num = (int)blocks.size() - 1;

	if (! blocks[blk_num]->fits(len))
	{
		// try some earlier blocks
		if (blk_num >= 1 && blocks[blk_num-1]->fits(len))
			blk_num -= 1;
		else if (blk_num >= 2 && blocks[blk_num-2]->fits(len))
			blk_num -= 2;
		else if (blk_num >= 3 && blocks[blk_num-3]->fits(len))
			blk_num -= 3;
		else if (blk_num >= 4 && blocks[blk_num-4]->fits(len))
			blk_num -= 4;
		else
		{
			// need a new block
			blocks.push_back(new string_block_c);
			blk_num++;
		}
	}

	int offset = blocks[blk_num]->add(str, len);

	return (blk_num * CHARS_PER_BLOCK) + offset + 1;
}

int string_table_c::find_huge(const char *str, int len)
{
	for (int i = 0; i < (int)huge_ones.size(); i++)
	{
		if (strcmp(huge_ones[i], str) == 0)
			return -(i+1);
	}

	return 0;
}

int string_table_c::add_huge(const char *str, int len)
{
	char *buf = new char[len+2];
	strcpy(buf, str);

	huge_ones.push_back(buf);

	return -(int)huge_ones.size();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
