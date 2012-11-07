//------------------------------------------------------------------------
//  Flat / Texture / Thing Browser
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2007-2012 Andrew Apted
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
#include "m_game.h"
#include "e_things.h"
#include "w_rawdef.h"
#include "w_flats.h"
#include "w_texture.h"
#include "levels.h"


extern std::map<std::string, Img *> flats;
extern std::map<std::string, Img *> textures;

extern std::map<char, linegroup_t *>  line_groups;
extern std::map<char, thinggroup_t *> thing_groups;

extern std::map<int, linetype_t *>   line_types;
extern std::map<int, sectortype_t *> sector_types;
extern std::map<int, thingtype_t *>  thing_types;


Browser_Item::Browser_Item(int X, int Y, int W, int H,
	                       const char *_desc, char _category) :
	Fl_Group(X, Y, W, H, ""),
	desc(_desc), category(_category),
	button(NULL), pic(NULL)
{
	end();

	int button_W = W;
	int button_H = H - 4;

	button = new Fl_Repeat_Button(X, Y + 2, button_W, button_H, desc.c_str());

	button->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
  	button->labelfont(FL_COURIER);
	button->labelsize(12 + KF * 2);
	button->when(FL_WHEN_CHANGED);

	add(button);


	resizable(NULL);
}

Browser_Item::Browser_Item(int X, int Y, int W, int H,
						   const char * _desc, char _category,
						   int pic_w, int pic_h, UI_Pic *_pic) :
	Fl_Group(X, Y, W, H, ""),
	desc(_desc), category(_category),
	button(NULL), pic(_pic)
{
	end();


	add(pic);


	Fl_Box *box = new Fl_Box(FL_NO_BOX, X, Y + H - 32, W, 24, desc.c_str());
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

	int state = Fl::event_state();

	main_win->line_box->SetTexture(tex_name, state);
}


void Browser_Item::flat_callback(Fl_Widget *w, void *data)
{
	const char *flat_name = (const char *)data;
	SYS_ASSERT(flat_name);

	int state = Fl::event_state();

	main_win->sec_box->SetTexture(flat_name, state);
}


void Browser_Item::thing_callback(Fl_Widget *w, void *data)
{
	int new_type = w->argument();

	main_win->thing_box->SetThingType(new_type);
}


void Browser_Item::line_callback(Fl_Widget *w, void *data)
{
	int new_type = w->argument();

	main_win->line_box->SetLineType(new_type);
}


void Browser_Item::sector_callback(Fl_Widget *w, void *data)
{
	int new_type = w->argument();

	main_win->sec_box->SetSectorType(new_type);
}


//------------------------------------------------------------------------


UI_Browser_Box::UI_Browser_Box(int X, int Y, int W, int H, const char *label, char _kind) :
    Fl_Group(X, Y, W, H, NULL),
	kind(_kind)
{
	end(); // cancel begin() in Fl_Group constructor

	box(FL_FLAT_BOX);  // box(FL_DOWN_BOX);

	color(FL_DARK3, FL_DARK3);


	strcpy(cat_letters, "*");


	int cx = X + 80 + KF * 8;
	int cy = Y + 4;

	Fl_Box *title = new Fl_Box(X + 40, cy, W - 62, 22+KF*4, label);
	title->labelsize(20+KF*4);
	add(title);

	Fl_Button *hide_button = new Fl_Button(X + 8, cy+2, 22, 22, "X"); // "@>>"
	hide_button->callback(hide_callback, this);
	hide_button->labelsize(14);
//	hide_button->labelcolor(FL_DARK3);
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

	cy += category->h() + 6;


	sortm = NULL;

	if (strchr("OSL", kind))
	{
		sortm = new Fl_Choice(cx, cy, 160, 22, "Sort:");
		sortm->align(FL_ALIGN_LEFT);
		sortm->add("Alphabetical|Numeric");
		sortm->value((kind == 'O') ? 0 : 1);
		sortm->labelsize(KF_fonth);
		sortm->textsize(KF_fonth);
		sortm->callback(sort_callback, this);

		add(sortm);

		cy += sortm->h() + 6;
	}


	search = new Fl_Input(cx, cy, 100, 22, "Match:");
	search->align(FL_ALIGN_LEFT);
	search->callback(filter_callback, this);
	search->when(FL_WHEN_CHANGED);

	add(search);


	pics = NULL;

	if (strchr("FTO", kind))
	{
		pics = new Fl_Check_Button(X+W-78, cy, 20, 22, "Pics");
		pics->align(FL_ALIGN_RIGHT);
		pics->value(1);
///???		pics->callback(filter_callback, this);

		add(pics);
	}

	cy += search->h() + 12;


	pack = new Fl_Scroll(X, cy, W, H-3 - (cy - Y));
	pack->end();
	pack->type(Fl_Scroll::VERTICAL_ALWAYS);

	if (kind == 'T' || kind == 'F')
		pack->color(FL_BLACK, FL_BLACK);
	else
		pack->color(FL_DARK3, FL_DARK3);

	pack->scrollbar.align(FL_ALIGN_LEFT);
	pack->scrollbar.color(FL_DARK2, FL_DARK2);
	pack->scrollbar.selection_color(FL_GRAY0);

	add(pack);


	// resize box

	Fl_Box * rs_box = new Fl_Box(FL_NO_BOX, X + W - 18, Y, 16, H, NULL);
	rs_box->clear_visible();

	add(rs_box);

	resizable(rs_box);
}


UI_Browser_Box::~UI_Browser_Box()
{
}


void UI_Browser_Box::resize_buttons()
{
	fix_scrollbar_order();

	// subtract 2 to ignore the scrollbars
	int total = pack->children() - 2;

	int W = pack->w() - 24;

	for (int i = 0 ; i < total ; i++)
	{
		Browser_Item *item = (Browser_Item *)pack->child(i);

		Fl_Repeat_Button * button = item->button;

		if (button)
		{
			button->size(W, button->h());
		}
	}
}


void UI_Browser_Box::resize(int X, int Y, int W, int H)
{
	Fl_Group::resize(X, Y, W, H);

	// rearrange images
	if (kind == 'T' || kind == 'F')
	{
		Filter();
	}
	else  // change button sizes
	{
		resize_buttons();
	}
}


void UI_Browser_Box::filter_callback(Fl_Widget *w, void *data)
{
	UI_Browser_Box *that = (UI_Browser_Box *)data;

	that->Filter();
}


void UI_Browser_Box::hide_callback(Fl_Widget *w, void *data)
{
	CMD_SetBrowser(0);
}


void UI_Browser_Box::sort_callback(Fl_Widget *w, void *data)
{
	UI_Browser_Box *that = (UI_Browser_Box *)data;

	that->Sort();
}


bool UI_Browser_Box::Filter(bool force_update)
{
	pack->scroll_to(0, 0);

	fix_scrollbar_order();

	// subtract 2 to ignore the scrollbars
	int total = pack->children() - 2;

	bool side_by_side = (kind == 'F' || kind == 'T');

	bool changes = false;

	int left_X  = pack->x() + 20;
	int right_X = left_X + pack->w() - 20;

	// current position
	int cx = left_X;
	int cy = pack->y();

	// the highest visible widget on the current line
	int highest = 0;

	for (int i = 0 ; i < total ; i++)
	{
		Browser_Item *item = (Browser_Item *)pack->child(i);

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
		if (side_by_side && (cx <= left_X || (cx + item->w()) <= right_X))
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

	pack->init_sizes();
	pack->redraw();

	return changes;
}


bool UI_Browser_Box::SearchMatch(Browser_Item *item) const
{
	if (category->value() > 0)
	{
		char cat = cat_letters[category->value()];

		if (cat != item->category)
			return false;
	}

	if (search->size() == 0)
		return true;

	// add '*' to the start and end of the pattern
	// (unless it uses the ^ or $ anchors)

	const char *search_pat = search->value();

	bool negated = false;
	if (search_pat[0] == '!')
	{
		search_pat++;
		negated = true;
	}

	char pattern[256];
	pattern[0] = 0;

	if (search_pat[0] == '^')
		search_pat++;
	else
		strcpy(pattern, "*");
	
	strcat(pattern, search_pat);

	int len = strlen(pattern);

	if (len == 0)
		return true;

	if (pattern[len-1] == '$')
		pattern[len-1] = 0;
	else
		strcat(pattern, "*");

	bool result = fl_filename_match(item->desc.c_str(), pattern) ? true : false;

	return negated ? !result : result;
}


static int SortCmp(const Browser_Item *A, const Browser_Item *B, int method)
{
	const char *sa = A->desc.c_str();
	const char *sb = B->desc.c_str();

	if (method != 1)  // 1 = Numeric
	{
		if (strchr(sa, '/')) sa = strchr(sa, '/') + 1;
		if (strchr(sb, '/')) sb = strchr(sb, '/') + 1;

		// 2 = Alphabetical in LINEDEF mode, so skip trigger type (SR etc)
		if (method == 2)
		{
			while (isspace(*sa)) sa++;
			while (isspace(*sb)) sb++;

			while (! isspace(*sa)) sa++;
			while (! isspace(*sb)) sb++;
		}
	}

	return strcmp(sa, sb);
}

static void SortPass(std::vector< Browser_Item * >& ARR, int gap, int total, int method)
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
	fix_scrollbar_order();

	// subtract 2 to ignore the scrollbars
	int total = pack->children() - 2;

	// transfer widgets to a local vector
	std::vector< Browser_Item * > ARR;

	for (int i = 0 ; i < total ; i++)
	{
		ARR.push_back( (Browser_Item *) pack->child(0));

		pack->remove(0);
	}

	int method = sortm ? sortm->value() : 0;

	if (method == 0 && kind == 'L')
		method = 2;

	// shell sort
	SortPass(ARR, 9, total, method);
	SortPass(ARR, 4, total, method);
	SortPass(ARR, 1, total, method);

	// transfer them back to the pack widget
	for (int i = 0 ; i < total ; i++)
		pack->add(ARR[i]);

	fix_scrollbar_order();

	// reposition them all
	Filter(true);
}


void UI_Browser_Box::fix_scrollbar_order()
{
	// make sure the scrollbars are the last two children.
	// (ideally we'd call fix_scrollbar_order() method, but it is private)
	pack->resize(pack->x(), pack->y(), pack->w(), pack->h());

	pack->redraw();
}


char * UI_Browser_Box::TidySearch() const
{
	static char buffer[FL_PATH_MAX];

	char *dest = buffer;

	for (const char *src = search->value() ; *src ; src++)
		if (isprint(*src) && *src != '"')
			*dest++ = *src;

	*dest = 0;

	return buffer;
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


void UI_Browser_Box::Populate_Images(std::map<std::string, Img *> & img_list)
{
	/* Note: the side-by-side packing is done in Filter() method */

	std::map<std::string, Img *>::iterator TI;

	int cx = pack->x() + 20;
	int cy = pack->y();

	char full_desc[256];

	for (TI = img_list.begin() ; TI != img_list.end() ; TI++)
	{
		const char *name = TI->first.c_str();

		Img *image = TI->second;

		if (false) /* NO PICS */
			snprintf(full_desc, sizeof(full_desc), "%-8s : %3dx%d", name,
					 image->width(), image->height());
		else
			snprintf(full_desc, sizeof(full_desc), "%-8s", name);

		int pic_w = (kind == 'F' || image->width() <= 64) ? 64 : 128; // MIN(128, MAX(4, image->width()));
		int pic_h = (kind == 'F') ? 64 : MIN(128, MAX(4, image->height()));

		if (image->width() >= 256 && image->height() == 128)
			pic_h = 64;

		int item_w = MAX(pic_w, 64) + 8;
		int item_h = MAX(pic_h, 16) + 6 + 28;

		char category = 0;

		UI_Pic *pic = new UI_Pic(cx, cy, pic_w, pic_h);

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
		                                      full_desc, category,
		                                      pic_w, pic_h, pic);
		pack->add(item);

		cy += item->h();
	}
}


void UI_Browser_Box::Populate_ThingTypes()
{
	std::map<int, thingtype_t *>::iterator TI;

	int y = pack->y();

	int mx = pack->x() + 20;
	int mw = pack->w() - 24;

	char full_desc[256];

	for (TI = thing_types.begin() ; TI != thing_types.end() ; TI++)
	{
		thingtype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%4d/ %s", TI->first, info->desc);

		Browser_Item *item = new Browser_Item(mx, y, mw, 28, full_desc, info->group);

		item->button->callback(Browser_Item::thing_callback, NULL);
		item->button->argument(TI->first);

		pack->add(item);

		y += item->h() + 3;
	}
}


void UI_Browser_Box::Populate_LineTypes()
{
	std::map<int, linetype_t *>::iterator TI;

	int y = pack->y();

	int mx = pack->x() + 20;
	int mw = pack->w() - 24;

	char full_desc[256];

	for (TI = line_types.begin() ; TI != line_types.end() ; TI++)
	{
		linetype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%3d/ %s", TI->first,
		         TidyLineDesc(info->desc));

		Browser_Item *item = new Browser_Item(mx, y, mw, 28, full_desc, info->group);

		item->button->callback(Browser_Item::line_callback, NULL);
		item->button->argument(TI->first);

		pack->add(item);

		y += item->h() + 3;
	}
}


void UI_Browser_Box::Populate_SectorTypes()
{
	std::map<int, sectortype_t *>::iterator TI;

	int y = pack->y();

	int mx = pack->x() + 20;
	int mw = pack->w() - 24;

	char full_desc[256];

	for (TI = sector_types.begin() ; TI != sector_types.end() ; TI++)
	{
		sectortype_t *info = TI->second;

		snprintf(full_desc, sizeof(full_desc), "%3d/ %s", TI->first, info->desc);

		Browser_Item *item = new Browser_Item(mx, y, mw, 28, full_desc, 0);

		item->button->callback(Browser_Item::sector_callback, NULL);
		item->button->argument(TI->first);

		pack->add(item);

		y += item->h() + 3;
	}
}


void UI_Browser_Box::Populate()
{
	// delete existing ones
	pack->clear();

	switch (kind)
	{
		case 'T':
			Populate_Images(textures);
			break;

		case 'F':
			Populate_Images(flats);
			break;

		case 'O':
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

	// this calls Filter to reposition the widgets
	Sort();
}


void UI_Browser_Box::SetCategories(const char *cats, const char *letters)
{
	// FIXME: possible buffer overflow
	strcpy(cat_letters, letters);

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

	int new_cat = 0;

	if (dir > 0)
	{
		new_cat = (category->value() + 1) % total_cats;
	}
	else if (dir < 0)
	{
		new_cat = (category->value() + total_cats - 1) % total_cats;
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
	// get the scrollbar (OMG this is hacky shit)
	int index = pack->children() - 1;

	Fl_Scrollbar *bar = (Fl_Scrollbar *)pack->child(index);

	SYS_ASSERT(bar);

	// this logic is copied from FLTK
	// (would be nice if we could just resend the event to the widget)

	float ssz = bar->slider_size();

	if (ssz >= 1.0)
		return;

	int v  = bar->value();
	int ls = (kind == 'F' || kind == 'T') ? 100 : 40;

	if (delta < 0)
	{
		// PAGE-UP
		v -= int((bar->maximum() - bar->minimum()) * ssz / (1.0 - ssz));
		v += ls;
	}
	else
	{
		// PAGE-DOWN
		v += int((bar->maximum() - bar->minimum()) * ssz / (1.0 - ssz));
		v -= ls;
	}

    v = int(bar->clamp(v));

	bar->value(v);
	bar->damage(FL_DAMAGE_ALL);
	bar->set_changed();
	bar->do_callback();
}


//------------------------------------------------------------------------


UI_Browser::UI_Browser(int X, int Y, int W, int H, const char *label) :
    Fl_Group(X, Y, W, H, label),
	active(2)
{
	end(); // cancel begin() in Fl_Group constructor


	// create each browser box

	const char *mode_letters = "TFOLS";

	const char *mode_titles[5] =
	{
		"Texture List",
		"Flat List",
		"Thing Types",
		"Linedef Types",
		"Sector Types"
	};

	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i] = new UI_Browser_Box(X, Y, W, H, mode_titles[i], mode_letters[i]);

		add(browsers[i]);

		if (i != active)
			browsers[i]->hide();
	}
}


UI_Browser::~UI_Browser()
{
}


void UI_Browser::Populate()
{
	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i]->Populate();
	}

	// setup the categories

	char letters[64];

	const char *tex_cats = M_TextureCategoryString(letters);

	browsers[0]->SetCategories(tex_cats, letters);
	browsers[1]->SetCategories(tex_cats, letters);

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

	browsers[active]->hide();

	active = new_active;

	browsers[active]->show();
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

		default:
			return;
	}
}


void UI_Browser::NewEditMode(char edit_mode)
{
	switch (edit_mode)
	{
		case 'l':
			// if on LINE TYPES, stay there
			// otherwise go to TEXTURES
			if (active != 3)
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
	browsers[active]->CycleCategory(dir);
}


void UI_Browser::ClearSearchBox()
{
	browsers[active]->ClearSearchBox();
}


void UI_Browser::Scroll(int delta)
{
	browsers[active]->Scroll(delta);
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

	if (strcmp(tokens[0], "sort") == 0 && num_tok >= 2 && sortm)
	{
		sortm->value(atoi(tokens[1]) ? 1 : 0);
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

	if (sortm)
		fprintf(fp, "browser %c sort %d\n", kind, sortm->value());

	fprintf(fp, "browser %c search \"%s\"\n", kind, TidySearch());

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

	if (visible())
	{
		fprintf(fp, "open_browser %c\n", browsers[active]->GetKind());
	}

	for (int i = 0 ; i < 5 ; i++)
	{
		browsers[i]->WriteUser(fp);
	}
}


bool Browser_ParseUser(const char ** tokens, int num_tok)
{
	if (main_win)
		return main_win->browser->ParseUser(tokens, num_tok);
	
	return false;
}


void Browser_WriteUser(FILE *fp)
{
	if (main_win)
		return main_win->browser->WriteUser(fp);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
