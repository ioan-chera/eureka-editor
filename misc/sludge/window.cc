
#include "main.h"

#include <time.h>


#define MAIN_WINDOW_W  525
#define MAIN_WINDOW_H  410

UI_MainWin * main_win;

bool want_quit;
bool want_build;


#define PROGRESS_BG  FL_DARK1  //  fl_gray_ramp(16)
#define PROGRESS_FG  fl_color_cube(1,4,2)


#define MIN_ROOMS  2
#define MAX_ROOMS  50

#define MAX_MACHO  160


static void set_default_seed(Fl_Int_Input *w)
{
	char buffer[100];

	int value = (time(NULL) & 0x3FFFFF0) / 7;

	sprintf(buffer, "%07d", value);

	w->value(buffer);
}


const char * Int_TempStr(int value)
{
	static char buffer[100];

	sprintf(buffer, "%d", value);

	return buffer;
}


static void close_callback(Fl_Widget *w, void *data)
{
	want_quit = true;
}


static void about_callback(Fl_Button *w, void *data)
{
	// FIXME

	fl_message("Sludge = SLIGE + GUI");
}


static void build_callback(Fl_Button *w, void *data)
{
	want_build = true;
}


static void load_callback(Fl_Button *w, void *data)
{
	// TODO
}


static void minus_callback(Fl_Button *w, void *data)
{
	Fl_Widget * d = (Fl_Widget *)data;

	if (d == main_win->seed)
	{
		int val = atoi(main_win->seed->value());
		val = val - 1;
		main_win->seed->value(Int_TempStr(val));
	}
	else if (d == main_win->macho)
	{
		int val = atoi(main_win->macho->value());
		val = CLAMP(0, val - 10, MAX_MACHO);
		main_win->macho->value(Int_TempStr(val));
	}
	else if (d == main_win->rooms)
	{
		int val = atoi(main_win->rooms->value());
		if (val & 1) val--;
		val = CLAMP(MIN_ROOMS, val - 2, MAX_ROOMS);
		main_win->rooms->value(Int_TempStr(val));
	}
}


static void plus_callback(Fl_Button *w, void *data)
{
	Fl_Widget * d = (Fl_Widget *)data;

	if (d == main_win->seed)
	{
		int val = atoi(main_win->seed->value());
		val = val + 1;
		main_win->seed->value(Int_TempStr(val));
	}
	else if (d == main_win->macho)
	{
		int val = atoi(main_win->macho->value());
		val = CLAMP(0, val + 10, MAX_MACHO);
		main_win->macho->value(Int_TempStr(val));
	}
	else if (d == main_win->rooms)
	{
		int val = atoi(main_win->rooms->value());
		if (val & 1) val--;
		val = CLAMP(MIN_ROOMS, val + 2, MAX_ROOMS);
		main_win->rooms->value(Int_TempStr(val));
	}
}


static void validate_callback(Fl_Int_Input *w, void *data)
{
	if (w == main_win->macho)
	{
		int val = atoi(main_win->macho->value());
		val = CLAMP(0, val, MAX_MACHO);
		main_win->macho->value(Int_TempStr(val));
	}
	else if (w == main_win->rooms)
	{
		int val = atoi(main_win->rooms->value());
		val = CLAMP(MIN_ROOMS, val, MAX_ROOMS);
		main_win->rooms->value(Int_TempStr(val));
	}
}


UI_MainWin::UI_MainWin(const char *title) :
  Fl_Double_Window(MAIN_WINDOW_W, MAIN_WINDOW_H, title)
{
	color(WINDOW_BG);

	callback((Fl_Callback*)close_callback);

	/* main settings */

	{	Fl_Group *grp = new Fl_Group(0, 0, 260, 340);
		grp->box(FL_THIN_UP_BOX);
		grp->color(PANEL_BG);

		{ Fl_Box* o = new Fl_Box(10, 5, 195, 45, "Main Settings");
			o->labelfont(1);
			o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		game = new Fl_Choice(80, 50, 140, 25, "Game:");
		game->down_box(FL_BORDER_BOX);
		game->add("Doom 1|Doom 2");
		game->value(1);

		mode = new Fl_Choice(80, 90, 140, 25, "Mode:");
		mode->down_box(FL_BORDER_BOX);
		mode->add("Single Player|DeathMatch");
		mode->value(0);

		length = new Fl_Choice(80, 130, 140, 25, "Length:");
		length->down_box(FL_BORDER_BOX);
		length->add("Single|A Few Maps|Episode|Full Game");
		length->value(3);

		seed = new Fl_Int_Input(75, 245, 85, 30, "Seed:");
		set_default_seed(seed);

		{ Fl_Button* o = new Fl_Button(170, 245, 30, 30, "-");
			o->labelsize(20);
			o->callback((Fl_Callback*)minus_callback, seed);
		}
		{ Fl_Button* o = new Fl_Button(210, 245, 30, 30, "+");
			o->labelsize(20);
			o->callback((Fl_Callback*)plus_callback, seed);
		}

		config = new Fl_Output(75, 195, 105, 30, "Config:");
		config->value("DEFAULT");

		{ Fl_Button* o = new Fl_Button(190, 195, 55, 30, "Load");
			o->labelsize(12);
			o->callback((Fl_Callback*)load_callback);
			o->hide();  // FIXME: not implemented yet
		}

		grp->end();
	}

	/* user options */

	{	Fl_Group *grp = new Fl_Group(265, 0, 260, 340);
		grp->box(FL_THIN_UP_BOX);
		grp->color(PANEL_BG);

		{ Fl_Box* o = new Fl_Box(275, 8, 160, 34, "Playing Options");
			o->labelfont(1);
			o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
		}

		big_mon = new Fl_Check_Button(305, 50, 180, 30, " Big monsters");
		big_mon->down_box(FL_DOWN_BOX);

		big_weap = new Fl_Check_Button(305, 90, 180, 30, " Big weapons");
		big_weap->down_box(FL_DOWN_BOX);

		macho = new Fl_Int_Input(340, 195, 80, 30, "Macho:");
		macho->value("0");
		macho->callback((Fl_Callback*)validate_callback);
		/// macho->when(FL_WHEN_CHANGED);

		{ Fl_Button* o = new Fl_Button(430, 195, 30, 30, "-");
			o->labelsize(20);
			o->callback((Fl_Callback*)minus_callback, macho);
		}
		{ Fl_Button* o = new Fl_Button(470, 195, 30, 30, "+");
			o->labelsize(20);
			o->callback((Fl_Callback*)plus_callback, macho);
		}

		rooms = new Fl_Int_Input(340, 245, 80, 30, "Rooms:");
		rooms->value("18");
		rooms->callback((Fl_Callback*)validate_callback);
		/// rooms->when(FL_WHEN_CHANGED);

		{ Fl_Button* o = new Fl_Button(430, 245, 30, 30, "-");
			o->labelsize(20);
			o->callback((Fl_Callback*)minus_callback, rooms);
		}
		{ Fl_Button* o = new Fl_Button(470, 245, 30, 30, "+");
			o->labelsize(20);
			o->callback((Fl_Callback*)plus_callback, rooms);
		}

		grp->end();
	}

	/* bottom panel */

	about = new Fl_Button(220, 355, 80, 36, "About");
	about->labelsize(16);
	about->callback((Fl_Callback*)about_callback);

	build = new Fl_Button(325, 355, 80, 36, "Build");
	build->labelfont(1);
	build->labelsize(16);
	build->callback((Fl_Callback*)build_callback);

	quit = new Fl_Button(430, 355, 75, 36, "Quit");
	quit->labelsize(16);
	quit->callback((Fl_Callback*)close_callback);

	progress = new Fl_Progress(10, 357, 185, 30, "Ready!");
	progress->box(FL_FLAT_BOX);
	progress->color(PROGRESS_BG, PROGRESS_FG);
	progress->minimum(0.0);
	progress->maximum(100.0);

	end();
}


UI_MainWin::~UI_MainWin()
{
	// nothing needed
}


void UI_MainWin::Prog_Clear()
{
	progress->value(0.0);
}

void UI_MainWin::Prog_Set(int perc)
{
	sprintf(prog_label, "%d%%", perc);

	progress->value((float)perc);
	progress->label(prog_label);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
