//------------------------------------------------------------------------
//  BROWSER for TEXTURES / FLATS / THINGS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2016 Andrew Apted
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

#include <map>
#include <string>

#include "ui_window.h"
#include "ui_browser.h"

#include "im_img.h"
#include "im_color.h"
#include "m_config.h"
#include "m_game.h"
#include "e_things.h"
#include "w_rawdef.h"
#include "w_flats.h"
#include "w_texture.h"
#include "levels.h"


extern std::map<std::string, Img_c *> flats;
extern std::map<std::string, Img_c *> textures;

extern std::map<char, linegroup_t *>  line_groups;
extern std::map<char, thinggroup_t *> thing_groups;

extern std::map<int, linetype_t *>   line_types;
extern std::map<int, sectortype_t *> sector_types;
extern std::map<int, thingtype_t *>  thing_types;


#define  BROWBACK_COL  (gui_scheme == 2 ? FL_DARK3 : FL_DARK2)


// config items
bool browser_small_tex = false;


// sort methods
typedef enum
{
	SOM_Numeric = 0,
	SOM_Alpha,
	SOM_AlphaSkip,  // skip the S1, WR (etc) of linedef descriptions
	SOM_Recent

} sort_method_e;


/* text item */

Browser_Item::Browser_Item(int X, int Y, int W, int H,
	                       const char *_desc, const char *_realname,
						   int _num, char _category) :
	Fl_Group(X, Y, W, H, ""),
	desc(_desc), real_name(_realname),
	number(_num), category(_category),
	recent_idx(-2),
	button(NULL), pic(NULL)
{
	end();

	button = new Fl_Repeat_Button(X + 4, Y + 1, W - 8, H - 2, desc.c_str());

	button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
  	button->labelfont(FL_COURIER);
	button->labelsize(12 + KF * 2);
	button->when(FL_WHEN_CHANGED);

	add(button);
}


/* image item */

Browser_Item::Browser_Item(int X, int Y, int W, int H,
	                       const char *_desc, const char *_realname,
						   int _num, char _category,
						   int pic_w, int pic_h, UI_Pic *_pic) :
	Fl_Group(X, Y, W, H, ""),
	desc(_desc), real_name(_realname),
	number(_num), category(_category),
	recent_idx(-2),
	button(NULL), pic(_pic)
{
	end();

	add(pic);

	Fl_Box *box = new Fl_Box(FL_NO_BOX, X + 4, Y + H - 28, W - 4, 24, desc.c_str());
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelcolor(FL_WHITE);
	box->labelsize(12);

	add(box);

	resizable(NULL);
}


Browser_Item::~Browser_Item()
{
	// TODO
}


void Browser_Item::texture_callback(Fl_Widget *w, void *data)
{
	const char *tex_name = (const char *)data;
	SYS_ASSERT(tex_name);

	main_win->BrowsedItem('T', 0, tex_name, Fl::event_state());
}


void Browser_Item::flat_callback(Fl_Widget *w, void *data)
{
	const char *flat_name = (const char *)data;
	SYS_ASSERT(flat_name);

	main_win->BrowsedItem('F', 0, flat_name, Fl::event_state());
}


void Browser_Item::thing_callback(Fl_Widget *w, void *data)
{
	Browser_Item * item = (Browser_Item *) data;

	main_win->BrowsedItem('O', item->number, "", Fl::event_state());
}


void Browser_Item::line_callback(Fl_Widget *w, void *data)
{
	Browser_Item * item = (Browser_Item *) data;

	main_win->BrowsedItem('L', item->number, "", Fl::event_state());
}


void Browser_Item::sector_callback(Fl_Widget *w, void *data)
{
	Browser_Item * item = (Browser_Item *) data;

	main_win->BrowsedItem('S', item->number, "", Fl::event_state());
}


//------------------------------------------------------------------------


UI_Browser_Box::UI_Browser_Box(int X, int Y, int W, int H, const char *label, char _kind) :
    Fl_Group(X, Y, W, H, NULL),
	kind(_kind), pic_mode(false)
{
	end(); // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);

	color(BROWBACK_COL, BROWBACK_COL);


	strcpy(cat_letters, "*");


	int cx = X + 80 + KF * 8;
	int cy = Y + 4;

	Fl_Box *title = new Fl_Box(X + 34, cy, W - 90, 22+KF*4, label);
	title->labelsize(20+KF*4);
	add(title);

	Fl_Button *hide_button = new Fl_Button(X + 8, cy+2, 22, 22, "X");
	hide_button->callback(hide_callback, this);
	hide_button->labelsize(14);
	add(hide_button);

	cy += title->h() + 6;


	category = new Fl_Choice(cx, cy, 160, 22, "Category:");
	category->align(FL_ALIGN_LEFT);
	category->add("ALL");
	category->value(0);
	category->labelsize(KF_fonth);
	category->textsize(KF_fonth);
	category->callback(filter_callback, this);

	add(category);

	cy += category->h() + 9;


	search = new Fl_Input(cx, cy, 120, 22, "Match:");
	search->align(FL_ALIGN_LEFT);
	search->callback(filter_callback, this);
	search->when(FL_WHEN_CHANGED);

	add(search);

	cy += search->h() + 6;


	alpha = NULL;

	if (strchr("OSL", kind))
	{
		alpha = new Fl_Check_Button(cx, cy, 75, 22, " Alpha");

		// things need to repopulate (in picture mode anyway)
		if (kind == 'O')
			alpha->callback(repop_callback, this);
		else
			alpha->callback(sort_callback, this);
 
		// things usually show pics (with sprite name), so want alpha
		if (kind == 'O')
			alpha->value(1);

		add(alpha);
	}


	pics = NULL;

	if (strchr("O", kind))
	{
		pics = new Fl_Check_Button(X+172, cy, 64, 22, " Pics");
		pics->value(1);
		pics->callback(repop_callback, this);

		add(pics);
	}


	if (strchr("OSL", kind))
	{
		cy += 30;
	}


	int top_H = cy - Y;

	scroll = new UI_Scroll(X, cy, W, H-3 - top_H);

	scroll->box(FL_FLAT_BOX);

	add(scroll);


	// resize box

	Fl_Box * rs_box = new Fl_Box(FL_NO_BOX, X + W - 10, Y + top_H, 8, H - top_H, NULL);

	resizable(rs_box);
}


UI_Browser_Box::~UI_Browser_Box()
{
	// nothing needed
}


void UI_Browser_Box::resize(int X, int Y, int W, int H)
{
	Fl_Group::resize(X, Y, W, H);

	Fl_Widget * rs_box = resizable();

	rs_box->resize(X + W - 10, Y + rs_box->h(), 8, H - rs_box->h());

	// rearrange images
	if (pic_mode)
	{
		Filter();
	}
}


void UI_Browser_Box::filter_callback(Fl_Widget *w, void *data)
{
	UI_Browser_Box *that = (UI_Browser_Box *)data;

	that->Filter();
}


void UI_Browser_Box::hide_callback(Fl_Widget *w, void *data)
{
	main_win->ShowBrowser(0);
}


void UI_Browser_Box::repop_callback(Fl_Widget *w, void *data)
{
	UI_Browser_Box *that = (UI_Browser_Box *)data;

	that->Populate();
}


void UI_Browser_Box::sort_callback(Fl_Widget *w, void *data)
{
	UI_Browser_Box *that = (UI_Browser_Box *)data;

	that->Sort();
}


bool UI_Browser_Box::Filter(bool force_update)
{
	bool changes = false;

	int left_X  = scroll->x() + SBAR_W;
	int right_X = left_X + scroll->w() - SBAR_W;

	// current position
	int cx = left_X;
	int cy = scroll->y();

	// the highest visible widget on the current line
	int highest = 0;

	for (int i = 0 ; i < scroll->Children() ; i++)
	{
		Browser_Item *item = (Browser_Item *)scroll->Child(i);

		item->redraw();

		bool keep = SearchMatch(item);

		if (keep != (item->visible() ? true : false))
		{
			if (keep)
				item->show();
			else
				item->hide();

			changes = true;
		}

		if (! keep)
		{
			item->position(cx, cy);
			continue;
		}

		// can it fit on the current row?
		if (pic_mode && (cx <= left_X || (cx + item->w()) <= right_X))
		{
			// Yes
		}
		else
		{
			// No, move down to the next row
			cx = left_X;

			cy += highest;

			highest = 0;
		}

		// update position
		item->position(cx, cy);

		cx += item->w();

		highest = MAX(highest, item->h());
	}

	scroll->Init_sizes();
	scroll->redraw();

	return changes;
}


bool UI_Browser_Box::SearchMatch(Browser_Item *item) const
{
	if (category->value() > 0)
	{
		char cat = cat_letters[category->value()];

		// special logic for RECENT category  [ignore search box]
		if (cat == '^')
			return (item->recent_idx >= 0);

		if (! (cat == tolower(item->category) ||
			   (cat == 'X' && isupper(item->category))))
			return false;
	}

	// here an empty pattern matches EVERYTHING
	// [ different to Texture_MatchPattern semantics ]
	if (search->size() == 0)
		return true;

	const char *pattern = search->value();

	if (kind == 'T' || kind == 'F')
		return Texture_MatchPattern(item->real_name.c_str(), pattern);

	return Texture_MatchPattern(item->desc.c_str(), pattern);
}


bool UI_Browser_Box::Recent_UpdateItem(Browser_Item *item)
{
	// returns true if the index changed

	int new_idx = -1;

	switch (kind)
	{
		case 'T':
			new_idx = recent_textures.find(item->real_name.c_str());
			break;

		case 'F':
			new_idx = recent_flats.find(item->real_name.c_str());
			break;

		case 'O':
			new_idx = recent_things.find_number(item->number);
			break;

		default:
			return false;
	}

	if (item->recent_idx == new_idx)
		return false;

	item->recent_idx = new_idx;
	return true;
}


static int SortCmp(const Browser_Item *A, const Browser_Item *B, sort_method_e method)
{
	const char *sa = A->desc.c_str();
	const char *sb = B->desc.c_str();

	if (method == SOM_Numeric)
	{
		return (A->number - B->number);
	}
	else if (method == SOM_Recent)
	{
		return (A->recent_idx - B->recent_idx);
	}

	if (strchr(sa, '/')) sa = strchr(sa, '/') + 1;
	if (strchr(sb, '/')) sb = strchr(sb, '/') + 1;

	// Alphabetical in LINEDEF mode, skip trigger type (SR etc)
	if (method == SOM_AlphaSkip)
	{
		while (isspace(*sa)) sa++;
		while (isspace(*sb)) sb++;

		if (sa[0] && sa[1] && sa[2] == ' ')
			while (! isspace(*sa)) sa++;

		if (sb[0] && sb[1] && sb[2] == ' ')
			while (! isspace(*sb)) sb++;
	}

	return strcmp(sa, sb);
}

static void SortPass(std::vector< Browser_Item * >& ARR, int gap, int total, sort_method_e method)
{
	int i, k;

	for (i = gap ; i < total ; i++)
	{
		Browser_Item * temp = ARR[i];

		for (k = i ; k >= gap && SortCmp(ARR[k - gap], temp, method) > 0 ; k -= gap)
			ARR[k] = ARR[k - gap];

		ARR[k] = temp;
	}
}

void UI_Browser_Box::Sort()
{
	int total = scroll->Children();

	// transfer widgets to a local vector
	std::vector< Browser_Item * > ARR;

	for (int i = 0 ; i < total ; i++)
	{
		ARR.push_back( (Browser_Item *) scroll->Child(0));

		scroll->Remove_first();
	}

	char cat = cat_letters[category->value()];

	sort_method_e method = SOM_Alpha;

	if (cat == '^')
		method = SOM_Recent;
	else if (alpha && ! alpha->value())
		method = SOM_Numeric;
	else if (kind == 'L')
		method = SOM_AlphaSkip;

	// shell sort
	SortPass(ARR, 9, total, method);
	SortPass(ARR, 4, total, method);
	SortPass(ARR, 1, total, method);

	// transfer them back to the scroll widget
	for (int i = 0 ; i < total ; i++)
		scroll->Add(ARR[i]);

	// reposition them all
	Filter(true);
}


const char * TidyLineDesc(const char *name)
{
	// escapes any '&' characters for FLTK

	if (! strchr(name, '&'))
		return name;
	
	static char buffer[FL_PATH_MAX];

	char *dest = buffer;

	for (const char *src = name ; *src ; src++)
	{
		if (*src == '&')
		{
			*dest++ = '&';
			*dest++ = '&';
			continue;
		}

		*dest++ = *src;
	}

	*dest = 0;

	return buffer;
}


void UI_Browser_Box::Populate_Images(std::map<std::string, Img_c *> & img_list)
{
	/* Note: the side-by-side packing is done in Filter() method */

	pic_mode = true;

	scroll->color(FL_BLACK, FL_BLACK);
	scroll->resize_horiz(false);
	scroll->Line_size(98);

	std::map<std::string, Img_c *>::iterator TI;

	int cx = scroll->x() + SBAR_W;
	int cy = scroll->y();

	char full_desc[256];

	for (TI = img_list.begin() ; TI != img_list.end() ; TI++)
	{
		const char *name = TI->first.c_str();

		Img_c *image = TI->second;

		if (false) /* NO PICS */
			snprintf(full_desc, sizeof(full_desc), "%-8s : %3dx%d", name,
					 image->width(), image->height());
		else
			snprintf(full_desc, sizeof(full_desc), "%-8s", name);

		int pic_w = (kind == 'F' || image->width() <= 64) ? 64 : 128; // MIN(128, MAX(4, image->width()));
		int pic_h = (kind == 'F') ? 64 : MIN(128, MAX(4, image->height()));

		if (browser_small_tex && kind == 'T')
		{
			pic_w = 64;
			pic_h = MIN(64, MAX(4, image->height()));
		}

		if (image->width() >= 256 && image->height() == 128)
		{
			pic_w = 128;
			pic_h = 64;
		}

		int item_w = 8 + MAX(pic_w, 64) + 2;
		int item_h = 4 + MAX(pic_h, 16) + 2 + 24 + 4;

		char category = 0;

		UI_Pic *pic = new UI_Pic(cx + 8, cy + 4, pic_w, pic_h);

		if (kind == 'F')
		{
			pic->GetFlat(name);
			pic->callback(Browser_Item::flat_callback, (void *)name);

			category = M_GetFlatType(name);
		}
		else if (kind == 'T')
		{
			pic->GetTex(name);
			pic->callback(Browser_Item::texture_callback, (void *)name);

			category = M_GetTextureType(name);
		}

		Browser_Item *item = new Browser_Item(cx, cy, item_w, item_h,
		                                      full_desc, name,
											  0 /* num */, category,
		                                      pic_w, pic_h, pic);
		scroll->Add(item);
	}
}


void UI_Browser_Box::Populate_Sprites()
{
	/* Note: the side-by-side packing is done in Filter() method */

	pic_mode = true;

	scroll->color(FL_BLACK, FL_BLACK);
	scroll->resize_horiz(false);
	scroll->Line_size(98);

	std::map<int, thingtype_t *>::iterator TI;

	int cx = scroll->x() + SBAR_W;
	int cy = scroll->y();

	char full_desc[256];

	for (TI = thing_types.begin() ; TI != thing_types.end() ; TI++)
	{
		thingtype_t *info = TI->second;

		// ignore sprite-less things
		if (y_stricmp(info->sprite, "NULL") == 0)
			continue;

		if (alpha->value() == 0)
			sprintf(full_desc, "%d", TI->first);
		else
			snprintf(full_desc, sizeof(full_desc), "%s", info->sprite);

		int pic_w = 64;
		int pic_h = 72;

		int item_w = 8 + MAX(pic_w, 64) + 2;
		int item_h = 4 + MAX(pic_h, 16) + 2 + 24 + 4;

		UI_Pic *pic = new UI_Pic(cx + 8, cy + 4, pic_w, pic_h);

		pic->GetSprite(TI->first, FL_BLACK);

		Browser_Item *item = new Browser_Item(cx, cy, item_w, item_h,
		                                      full_desc, "", TI->first, info->group,
		                                      pic_w, pic_h, pic);

		pic->callback(Browser_Item::thing_callback, item);

		scroll->Add(item);
	}
}


void UI_Browser_Box::Populate_ThingTypes()
{
	std::map<int, thingtype_t *>::iterator TI;

	int y = scroll->y();

	int mx = scroll->x() + SBAR_W;
	int mw = scroll->w() - SBAR_W;

	char full_desc[256];

	for (TI = thing_types.begin() ; TI != thing_types.end() ; TI++)
	{
		thingtype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%4d/ %s", TI->first, info->desc);

		Browser_Item *item = new Browser_Item(mx, y, mw, 24, full_desc, "", TI->first, info->group);

		item->button->callback(Browser_Item::thing_callback, item);

		scroll->Add(item);
	}
}


void UI_Browser_Box::Populate_LineTypes()
{
	std::map<int, linetype_t *>::iterator TI;

	int y = scroll->y();

	int mx = scroll->x() + SBAR_W;
	int mw = scroll->w() - SBAR_W;

	char full_desc[256];

	for (TI = line_types.begin() ; TI != line_types.end() ; TI++)
	{
		linetype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%3d/ %s", TI->first,
		         TidyLineDesc(info->desc));

		Browser_Item *item = new Browser_Item(mx, y, mw, 24, full_desc, "", TI->first, info->group);

		item->button->callback(Browser_Item::line_callback, item);

		scroll->Add(item);
	}
}


void UI_Browser_Box::Populate_SectorTypes()
{
	std::map<int, sectortype_t *>::iterator TI;

	int y = scroll->y();

	int mx = scroll->x() + SBAR_W;
	int mw = scroll->w() - SBAR_W;

	char full_desc[256];

	for (TI = sector_types.begin() ; TI != sector_types.end() ; TI++)
	{
		sectortype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%3d/ %s", TI->first, info->desc);

		Browser_Item *item = new Browser_Item(mx, y, mw, 24, full_desc, "", TI->first, 0 /* cat */);

		item->button->callback(Browser_Item::sector_callback, item);

		scroll->Add(item);
	}
}


void UI_Browser_Box::Populate()
{
	// delete existing ones
	scroll->Remove_all();

	// default background and scroll rate
	scroll->color(WINDOW_BG, WINDOW_BG);
	scroll->resize_horiz(true);
	scroll->Line_size(24 * 2);

	pic_mode = false;

	switch (kind)
	{
		case 'T':
			Populate_Images(textures);
			break;

		case 'F':
			Populate_Images(flats);
			break;

		case 'O':
			if (pics->value())
				Populate_Sprites();
			else
				Populate_ThingTypes();
			break;

		case 'L':
			Populate_LineTypes();
			break;

		case 'S':
			Populate_SectorTypes();
			break;

		default:
			break;
	}

	RecentUpdate();

	// this calls Filter to reposition the widgets
	Sort();
}


void UI_Browser_Box::SetCategories(const char *cats, const char *letters)
{
	strncpy(cat_letters, letters, sizeof(cat_letters));
	cat_letters[sizeof(cat_letters) - 1] = 0;

	category->clear();
	category->add(cats);
	category->value(0);

	redraw();
}


void UI_Browser_Box::CycleCategory(int dir)
{
	// need '- 1' since the size() includes the empty terminator
	int total_cats = category->size() - 1;

	if (total_cats <= 1)
		return;

	int new_cat = category->value();

	for (int loop = 0 ; loop < 2 ; loop++)
	{
		if (dir > 0)
		{
			new_cat = (new_cat + 1) % total_cats;
		}
		else if (dir < 0)
		{
			new_cat = (new_cat + total_cats - 1) % total_cats;
		}

		// skip the RECENT category
		// TODO : controllable via a /xxx flag
		if (new_cat != 1)
			break;
	}

	if (category->value(new_cat))
	{
		Filter();
	}
}


bool UI_Browser_Box::CategoryByLetter(char letter)
{
	// need '- 1' since the size() includes the empty terminator
	int total_cats = category->size() - 1;

	for (int i = 0 ; i < total_cats ; i++)
	{
		if (cat_letters[i] == letter)
		{
			category->value(i);
			Filter();
			return true;
		}
	}

	return false;
}


void UI_Browser_Box::ClearSearchBox()
{
	if (search->size() > 0)
	{
		search->value("");

		Filter();
	}
}


void UI_Browser_Box::Scroll(int delta)
{
	scroll->Scroll(delta);
}


void UI_Browser_Box::RecentUpdate()
{
	bool changes = false;

	for (int i = 0 ; i < scroll->Children() ; i++)
	{
		Browser_Item *item = (Browser_Item *)scroll->Child(i);

		if (Recent_UpdateItem(item))
			changes = true;
	}

	if (changes)
		Sort();
}


void UI_Browser_Box::ToggleRecent(bool force_recent)
{
	char cat = cat_letters[category->value()];

	if (cat == '^' && force_recent)
	{
		Filter();
		return;
	}

	// this logic assumes first category is ALL, second is RECENT

	if (cat_letters[1] != '^')
		return;

	int new_cat = (cat == '^') ? 0 : 1;

	category->value(new_cat);

	Filter();
}


//------------------------------------------------------------------------


class UI_Generalized_Item : public Fl_Choice
{
public:
	const generalized_field_t * field;

public:
	UI_Generalized_Item(int X, int Y, int W, int H,
						const generalized_field_t *_field) :
		Fl_Choice(X, Y, W, H, ""),
		field(_field)
	{
		char label_buf[256];

		snprintf(label_buf, sizeof(label_buf), "%s: ", field->name);

		copy_label(label_buf);

		for (int i = 0 ; i < field->num_keywords ; i++)
		{
			add(field->keywords[i]);
		}

		Reset();
	}

	~UI_Generalized_Item()
	{ }

	int Compute() const
	{
		return (value() << field->shift) & field->mask;
	}

	void Decode(int line_type)
	{
		value((line_type & field->mask) >> field->shift);
	}

	void Reset()
	{
		int def_val = CLAMP(0, field->default_val, field->num_keywords - 1);

		value(def_val);
	}
};


class UI_Generalized_Page : public Fl_Group
{
public:
	int t_base;
	int t_length;

	UI_Generalized_Item * items[MAX_GEN_NUM_FIELDS];
	int num_items;

	// index for the "Change", "Model", "Monster" triplet, usually -1
	int change_index;

private:
	static void change_callback(Fl_Widget *w, void *data)
	{
		UI_Generalized_Page *page = (UI_Generalized_Page *)data;

		page->UpdateChange();
	}

public:
	UI_Generalized_Page(int X, int Y, int W, int H,
						const generalized_linetype_t *info) :
		Fl_Group(X, Y, W, H),
		t_base(info->base), t_length(info->length),
		num_items(0),
		change_index(-1)
	{
#if 0
		box(FL_FLAT_BOX);
		color(FL_BLUE, FL_BLUE);
#endif

		memset(items, 0, sizeof(items));

		num_items = info->num_fields;

		Y += 5;

		for (int i = 0 ; i < num_items ; i++)
		{
			bool is_change = (y_stricmp(info->fields[i].name, "Change") == 0);

			if (is_change)
				Y += 10;

			items[i] = new UI_Generalized_Item(X + 100, Y, 110, 22, &info->fields[i]);

			if (is_change && (i+2) < num_items)
			{
				change_index = i;

				items[i]->callback(change_callback, this);
			}

			if (change_index >= 0 && i == change_index+1)
				items[i]->deactivate();

			Y += 30;
		}

		end();
	}

	~UI_Generalized_Page()
	{ }

	void UpdateChange()
	{
		if (change_index < 0)
			return;

		if (items[change_index]->value() == 0)
		{
			items[change_index+1]->deactivate();
			items[change_index+2]->  activate();
		}
		else
		{
			items[change_index+1]->  activate();
			items[change_index+2]->deactivate();
		}
	}

	int ComputeType() const
	{
		int value = 0;
		
		for (int i = 0 ; i < num_items ; i++)
		{
			if (items[i]->visible())
				value = value | items[i]->Compute();
		}

		return t_base + value;
	}

	void DecodeType(int line_type)
	{
		line_type -= t_base;

		for (int i = 0 ; i < num_items ; i++)
		{
			items[i]->Decode(line_type);
		}

		UpdateChange();
	}

	void ResetFields()
	{
		for (int i = 0 ; i < num_items ; i++)
		{
			items[i]->Reset();
		}
	}
};


UI_Generalized_Box::UI_Generalized_Box(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, NULL),
	num_pages(0),
	in_update(false)
{
	box(FL_FLAT_BOX);

	color(BROWBACK_COL, BROWBACK_COL);


	memset(pages, 0, sizeof(pages));


	int orig_X = X;

///  X = X + (W - MIN_BROWSER_W);


	Y += 10;

	Fl_Box *title = new Fl_Box(X + 30, Y, W - 114, 22+KF*4, label);
	title->labelsize(20+KF*4);


	Fl_Button *hide_button = new Fl_Button(X + 8, Y+2, 22, 22, "X");
	hide_button->callback(hide_callback, this);
	hide_button->labelsize(14);

	Y += title->h() + 6;


	no_boom = new Fl_Box(FL_NO_BOX, X + 2, Y + 40, W - 60, 60,
						 "This requires BOOM\n(or a compatible port)");
	no_boom->labelsize(18);
	no_boom->labelcolor(FL_BLUE);
	no_boom->align(FL_ALIGN_INSIDE);


	Y += 10;

	category = new Fl_Choice(X + 40, Y, 180, 30);
	category->callback(cat_callback, this);
	category->textsize(16);


	end();


	// resize box

	Fl_Box * rs_box = new Fl_Box(FL_NO_BOX, orig_X + W - 10, Y + H - 10, 8, 8, NULL);

	resizable(rs_box);
}


UI_Generalized_Box::~UI_Generalized_Box()
{
	// nothing needed
}


void UI_Generalized_Box::Populate()
{
	if (! game_info.gen_types)
	{
		no_boom->show();

		category->hide();

		for (int i = 0 ; i < num_pages ; i++)
			pages[i]->hide();
	}
	else
	{
		// we only create the pages once
		// [ not strictly correct, but the generalized types never change ]

		if (! pages[0])
			CreatePages();

		no_boom->hide();

		category->show();

		for (int i = 0 ; i < num_pages ; i++)
		{
			if (i == category->value() - 1)
				pages[i]->show();
			else
				pages[i]->hide();
		}
	}

	redraw();
}


void UI_Generalized_Box::CycleCategory(int dir)
{
#if 0  // NOT SURE IF THIS MAKES SENSE

	if (no_boom->visible() || num_pages < 2)
		return;

	int new_page = category->value() + dir;

	if (new_page < 0)
		new_page = num_pages - 1;
	else if (new_page >= num_pages)
		new_page = 0;

	category->value(new_page);
	category->do_callback();

#endif
}


void UI_Generalized_Box::CreatePages()
{
	memset(pages, 0, sizeof(pages));

	num_pages = 0;

	category->clear();

	category->add("NONE");

	int X = x();  /// + (w() - MIN_BROWSER_W);

	for (int i = 0 ; i < num_gen_linetypes ; i++)
	{
		const generalized_linetype_t *info = &gen_linetypes[i];

		category->add(info->name);

		pages[i] = new UI_Generalized_Page(X + 10, y() + 100, 230, 300, info);

		add(pages[i]);

		num_pages += 1;
	}

	category->value(0);
}


int UI_Generalized_Box::ComputeType() const
{
	int cur_page = category->value();

	if (cur_page == 0)
		return 0;

	return pages[cur_page - 1]->ComputeType();
}


void UI_Generalized_Box::UpdateGenType(int line_type)
{
	if (in_update)
		return;

	if (no_boom->visible() || num_pages == 0)
		return;

	int new_page = -1;

	for (int i = 0 ; i < num_pages ; i++)
	{
		if (pages[i]->t_base <= line_type && line_type < pages[i]->t_base + pages[i]->t_length)
		{
			new_page = i;
			break;
		}
	}

	if (new_page < 0)
	{
		for (int k = 0 ; k < num_pages ; k++)
			pages[k]->ResetFields();

		if (category->value() != 0)
		{
			category->value(0);
			Populate();
		}

		return;
	}

	if (category->value() != new_page + 1)
	{
		category->value(new_page + 1);
		Populate();
	}

	pages[new_page]->DecodeType(line_type);
}


void UI_Generalized_Box::hide_callback(Fl_Widget *w, void *data)
{
	main_win->ShowBrowser(0);
}

void UI_Generalized_Box::cat_callback(Fl_Widget *w, void *data)
{
	UI_Generalized_Box *box = (UI_Generalized_Box *)data;

	int new_page = box->category->value() - 1;

	for (int i = 0 ; i < box->num_pages ; i++)
	{
		if (i == new_page)
			box->pages[i]->show();
		else
			box->pages[i]->hide();
	}

	value_callback(w, (void *)box);

	box->redraw();
}

void UI_Generalized_Box::value_callback(Fl_Widget *w, void *data)
{
	UI_Generalized_Box *box = (UI_Generalized_Box *)data;

	if (box->no_boom->visible() || box->num_pages == 0)
		return;

	if (box->category->value() == 0)
		return;

	box->in_update = true;  // prevent some useless work
	{
		int line_type = box->ComputeType();

		main_win->BrowsedItem('L', line_type, "", 0);
	}
	box->in_update = false;
}


//------------------------------------------------------------------------


UI_Browser::UI_Browser(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label),
	active(2)
{
	// create each browser box

	const char *mode_letters = "TFOLS";

	const char *mode_titles[5] =
	{
		"Textures",
		"Flats",
		"Things",
		"Line Types",
		"Sector Types"
	};

	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i] = new UI_Browser_Box(X, Y, W, H, mode_titles[i], mode_letters[i]);

		if (i != active)
			browsers[i]->hide();
	}

	gen_box = new UI_Generalized_Box(X, Y, W, H, "Generalized");
	gen_box->hide();


	end();
}


UI_Browser::~UI_Browser()
{
	// nothing needed
}


void UI_Browser::Populate()
{
	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i]->Populate();
	}

	gen_box->Populate();

	// setup the categories

	char letters[64];

	const char *tex_cats = M_TextureCategoryString(letters, false);
	browsers[0]->SetCategories(tex_cats, letters);

	const char *flat_cats = M_TextureCategoryString(letters, true);
	browsers[1]->SetCategories(flat_cats, letters);

	const char *thing_cats = M_ThingCategoryString(letters);
	browsers[2]->SetCategories(thing_cats, letters);

	const char *line_cats = M_LineCategoryString(letters);
	browsers[3]->SetCategories(line_cats, letters);

	// TODO: sector_cats

	// no ceiling_cats, fortunately :)
}


void UI_Browser::SetActive(int new_active)
{
	if (new_active == active)
		return;

	if (active < ACTIVE_GENERALIZED)
		browsers[active]->hide();
	else
		gen_box->hide();

	active = new_active;

	if (active < ACTIVE_GENERALIZED)
	{
		browsers[active]->show();
		browsers[active]->RecentUpdate();
	}
	else
	{
		gen_box->show();
	}

	if (new_active == ACTIVE_GENERALIZED)
		main_win->tile->MinimiseRight();
}


void UI_Browser::ChangeMode(char new_mode)
{
	switch (new_mode)
	{
		case 'T': SetActive(0); break;  // TEXTURES
		case 'F': SetActive(1); break;  // FLATS
		case 'O': SetActive(2); break;  // THINGS (Objects)
		case 'L': SetActive(3); break;  // LINE TYPES
		case 'S': SetActive(4); break;  // SECTOR TYPES

		case 'G': SetActive(ACTIVE_GENERALIZED); break;

		default: break;
	}
}


void UI_Browser::NewEditMode(char edit_mode)
{
	switch (edit_mode)
	{
		case 'l':
			// if on LINE TYPES, stay there
			// otherwise go to TEXTURES
			if (! (active == 3 || active == ACTIVE_GENERALIZED))
				SetActive(0);
			break;

		case 's':
			// if on SECTOR TYPES, stay there
			// otherwise go to FLATS
			if (active != 4)
				SetActive(1);
			break;

		case 't':
			SetActive(2);
			break;

		default:
			/* no change */
			break;
	}
}


void UI_Browser::CycleCategory(int dir)
{
	if (active < ACTIVE_GENERALIZED)
	{
		browsers[active]->CycleCategory(dir);
	}
	else
	{
		gen_box->CycleCategory(dir);
	}
}


void UI_Browser::ClearSearchBox()
{
	if (active < ACTIVE_GENERALIZED)
	{
		browsers[active]->ClearSearchBox();
	}

	// idea : reset generalized info
}


void UI_Browser::Scroll(int delta)
{
	if (active < ACTIVE_GENERALIZED)
	{
		browsers[active]->Scroll(delta);
	}
}


void UI_Browser::RecentUpdate()
{
	if (active < ACTIVE_GENERALIZED)
	{
		UI_Browser_Box *box = browsers[active];

		box->RecentUpdate();
	}
}


void UI_Browser::ToggleRecent(bool force_recent)
{
	// show browser if hidden [ and then force the RECENT category ]
	if (! visible())
	{
		main_win->ShowBrowser('/');

		force_recent = true;
	}

	if (active < ACTIVE_GENERALIZED)
	{
		browsers[active]->ToggleRecent(force_recent);
	}
}


void UI_Browser::UpdateGenType(int line_type)
{
	gen_box->UpdateGenType(line_type);
}


//------------------------------------------------------------------------


bool UI_Browser_Box::ParseUser(const char ** tokens, int num_tok)
{
	// syntax is: browser <kind> <keyword> <args...>

	if (num_tok < 3)
		return false;
	
	if (strcmp(tokens[0], "browser") != 0)
		return false;

	if (tokens[1][0] != kind)
		return false;

	tokens  += 2;
	num_tok -= 2;

	if (strcmp(tokens[0], "cat") == 0 && num_tok >= 2)
	{
		CategoryByLetter(tokens[1][0]);
		return true;
	}

	if (strcmp(tokens[0], "sort") == 0 && num_tok >= 2 && alpha)
	{
		alpha->value(atoi(tokens[1]) ? 0 : 1);
		Sort();
		return true;
	}

	if (strcmp(tokens[0], "search") == 0 && num_tok >= 2)
	{
		search->value(tokens[1]);
		Filter();
		return true;
	}

	if (strcmp(tokens[0], "pics") == 0 && num_tok >= 2 && pics)
	{
		pics->value(atoi(tokens[1]) ? 1 : 0);
		// pics_callback();
		return true;
	}

	return true;
}


void UI_Browser_Box::WriteUser(FILE *fp)
{
	char cat = cat_letters[category->value()];

	if (isprint(cat))
		fprintf(fp, "browser %c cat %c\n", kind, cat);

	if (alpha)
		fprintf(fp, "browser %c sort %d\n", kind, 1 - alpha->value());

	fprintf(fp, "browser %c search \"%s\"\n", kind, StringTidy(search->value(), "\""));

	if (pics)
		fprintf(fp, "browser %c pics %d\n", kind, pics->value());
}


bool UI_Browser::ParseUser(const char ** tokens, int num_tok)
{
	if (strcmp(tokens[0], "open_browser") == 0 && num_tok >= 2)
	{
		main_win->ShowBrowser(tokens[1][0]);
		return true;
	}

	for (int i = 0 ; i < 5 ; i++)
	{
		if (browsers[i]->ParseUser(tokens, num_tok))
			return true;
	}

	return false;
}


void UI_Browser::WriteUser(FILE *fp)
{
	fprintf(fp, "\n");

	fprintf(fp, "open_browser %c\n",
		(! visible()) ? '-' :
		(active >= ACTIVE_GENERALIZED) ? 'G' :
		browsers[active]->GetKind());

	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i]->WriteUser(fp);
	}

	// generalized box is not saved (not needed)
}


bool Browser_ParseUser(const char ** tokens, int num_tok)
{
	if (main_win)
	{
		if (main_win->tile->ParseUser(tokens, num_tok))
			return true;

		if (main_win->browser->ParseUser(tokens, num_tok))
			return true;
	}

	return false;
}


void Browser_WriteUser(FILE *fp)
{
	if (main_win)
	{
		main_win->tile   ->WriteUser(fp);
		main_win->browser->WriteUser(fp);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
