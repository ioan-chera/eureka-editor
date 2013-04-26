
#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_Progress.H>
#include <FL/Fl_Int_Input.H>
#include <FL/Fl_Double_Window.H>

#include <FL/fl_ask.H>


#define WINDOW_BG  fl_gray_ramp(3)
#define  PANEL_BG  fl_gray_ramp(3)  // UP_BOX makes it a lot lighter


class UI_MainWin : public Fl_Double_Window
{
public:
	// main settings

	Fl_Choice *game;
	Fl_Choice *mode;
	Fl_Choice *length;
	Fl_Int_Input *seed;
	Fl_Output *config;

	// user options

	Fl_Check_Button *big_mon;
	Fl_Check_Button *big_weap;
	Fl_Int_Input *macho;
	Fl_Int_Input *rooms;

	// bottom panel

	Fl_Button *about;
	Fl_Button *build;
	Fl_Button *quit;
	Fl_Progress *progress;

	char prog_label[100];

public:
	 UI_MainWin(const char *title);
	~UI_MainWin();

	void Prog_Clear();
	void Prog_Set(int perc);
};


extern UI_MainWin * main_win;

extern bool want_quit;
extern bool want_build;

#endif  /* __WINDOW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
