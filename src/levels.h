//------------------------------------------------------------------------
//  LEVEL MISC STUFF
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2015 Andrew Apted
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

#ifndef __EUREKA_LEVELS_H__
#define __EUREKA_LEVELS_H__

#include <string>

#include "e_things.h"


extern int Map_bound_x1;   /* minimum X value of map */
extern int Map_bound_y1;   /* minimum Y value of map */
extern int Map_bound_x2;   /* maximum X value of map */
extern int Map_bound_y2;   /* maximum Y value of map */

extern int MadeChanges;

void MarkChanges();

void CalculateLevelBounds();


void MapStuff_NotifyBegin();
void MapStuff_NotifyInsert(obj_type_e type, int objnum);
void MapStuff_NotifyDelete(obj_type_e type, int objnum);
void MapStuff_NotifyChange(obj_type_e type, int objnum, int field);
void MapStuff_NotifyEnd();


void ObjectBox_NotifyBegin();
void ObjectBox_NotifyInsert(obj_type_e type, int objnum);
void ObjectBox_NotifyDelete(obj_type_e type, int objnum);
void ObjectBox_NotifyChange(obj_type_e type, int objnum, int field);
void ObjectBox_NotifyEnd();


void Selection_NotifyBegin();
void Selection_NotifyInsert(obj_type_e type, int objnum);
void Selection_NotifyDelete(obj_type_e type, int objnum);
void Selection_NotifyChange(obj_type_e type, int objnum, int field);
void Selection_NotifyEnd();


void DumpSelection (selection_c * list);

void ConvertSelection(selection_c * src, selection_c * dest);

int Selection_FirstLine(selection_c *list);



// handling of recently used textures, flats and things

#define RECENTLY_USED_MAX	32

class Recently_used
{
private:
	int size;
	int keep_num;

	const char *name_set[RECENTLY_USED_MAX];

public:
	 Recently_used();
	~Recently_used();

	int find(const char *name); 
	int find_number(int val); 

	void insert(const char *name);
	void insert_number(int val);

	void clear();

	void WriteUser(FILE *fp, char letter);

private:
	void erase(int index);
	void push_front(const char *name);
};

extern Recently_used  recent_textures;
extern Recently_used  recent_flats;
extern Recently_used  recent_things;

void RecUsed_ClearAll();
void RecUsed_WriteUser(FILE *fp);
bool RecUsed_ParseUser(const char ** tokens, int num_tok);


#endif  /* __EUREKA_LEVELS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
