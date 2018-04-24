//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
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

#ifndef __EUREKA_E_CHECKS_H__
#define __EUREKA_E_CHECKS_H__

#include "ui_window.h"


void SideDefs_Unpack(bool is_after_load = false);

void Tags_ApplyNewValue(int new_tag);


void CMD_MapCheck();

void CMD_ApplyTag();


//------------------------------------------------------------------------

// the CHECK_xxx functions return the following values:
typedef enum
{
	CKR_OK = 0,            // no issues at all
	CKR_MinorProblem,      // only minor issues
	CKR_MajorProblem,      // some major problems
	CKR_Highlight,         // need to highlight stuff (skip further checks)
	CKR_TookAction         // [internal use : user took some action]

} check_result_e;


class UI_Check_base : public UI_Escapable_Window
{
protected:
	bool want_close;

	check_result_e  user_action;

	Fl_Group * line_group;

	int cy;
	int worst_severity;

private:
	static void close_callback(Fl_Widget *, void *);

public:
	UI_Check_base(int W, int H, bool all_mode, const char *L,
	              const char *header_txt);
	virtual ~UI_Check_base();

	void Reset();

	void AddGap(int H);

	void AddLine(const char *msg, int severity = 0, int W = -1,
	             const char *button1 = NULL, Fl_Callback *cb1 = NULL,
	             const char *button2 = NULL, Fl_Callback *cb2 = NULL,
	             const char *button3 = NULL, Fl_Callback *cb3 = NULL);

	check_result_e  Run();

	int WorstSeverity() const { return worst_severity; }
};


check_result_e CHECK_LineDefs(int min_severity = 0);
check_result_e CHECK_Textures(int min_severity = 0);
check_result_e CHECK_Tags    (int min_severity = 0);

#endif  /* __EUREKA_E_CHECKS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
