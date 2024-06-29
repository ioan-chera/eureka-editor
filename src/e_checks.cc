//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2018 Andrew Apted
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

#include "Errors.h"
#include "Instance.h"
#include "main.h"

#include <algorithm>

#include "e_checks.h"
#include "e_cutpaste.h"
#include "e_hover.h"
#include "e_main.h"
#include "e_path.h"
#include "e_vertex.h"
#include "LineDef.h"
#include "m_game.h"
#include "e_objects.h"
#include "Sector.h"
#include "SideDef.h"
#include "Thing.h"
#include "Vertex.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "ui_window.h"


#define   ERROR_MSG_COLOR	FL_RED
#define WARNING_MSG_COLOR	FL_BLUE


#define CAMERA_PEST  32000

//------------------------------------------------------------------------

class UI_Check_base : public UI_Escapable_Window
{
protected:
	bool want_close;

	CheckResult  user_action;

	Fl_Group *line_group;

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

	void AddLine(const SString &msg, int severity = 0, int W = -1,
		const char *button1 = NULL, Fl_Callback *cb1 = NULL,
		const char *button2 = NULL, Fl_Callback *cb2 = NULL,
		const char *button3 = NULL, Fl_Callback *cb3 = NULL);

	CheckResult  Run();

	int WorstSeverity() const { return worst_severity; }
};


//------------------------------------------------------------------------
//  BASE CLASS
//------------------------------------------------------------------------

void UI_Check_base::close_callback(Fl_Widget *w, void *data)
{
	UI_Check_base *dialog = (UI_Check_base *)data;

	dialog->want_close = true;
}


UI_Check_base::UI_Check_base(int W, int H, bool all_mode,
                             const char *L, const char *header_txt) :
	UI_Escapable_Window(W, H, L),
	want_close(false), user_action(CheckResult::ok),
	worst_severity(0)
{
	cy = 10;

	callback(close_callback, this);

	int ey = h() - 66;

	Fl_Box *title = new Fl_Box(FL_NO_BOX, 10, cy, w() - 20, 30, header_txt);
	title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	title->labelfont(FL_HELVETICA_BOLD);
	title->labelsize(FL_NORMAL_SIZE + 2);

	cy = 45;

	line_group = new Fl_Group(0, 0, w(), ey);
	line_group->end();

	{ Fl_Group *o = new Fl_Group(0, ey, w(), 66);

	  o->box(FL_FLAT_BOX);
	  o->color(WINDOW_BG, WINDOW_BG);

	  int but_W = all_mode ? 110 : 70;

	  { Fl_Button *ok_but;

	    ok_but = new Fl_Button(w()/2 - but_W/2, ey + 18, but_W, 34,
							   all_mode ? "Continue" : "OK");
		ok_but->labelfont(1);
		ok_but->callback(close_callback, this);
	  }
	  o->end();
	}

	end();
}


UI_Check_base::~UI_Check_base()
{ }


void UI_Check_base::Reset()
{
	want_close = false;
	user_action = CheckResult::ok;

	cy = 45;

	line_group->clear();

	redraw();
}


void UI_Check_base::AddGap(int H)
{
	cy += H;
}


void UI_Check_base::AddLine(
		const SString &msg, int severity, int W,
		 const char *button1, Fl_Callback *cb1,
		 const char *button2, Fl_Callback *cb2,
		 const char *button3, Fl_Callback *cb3)
{
	int cx = 30;

	if (W < 0)
		W = w() - 40;

	Fl_Box *box = new Fl_Box(FL_NO_BOX, cx, cy, W, 25, NULL);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
	box->copy_label(msg.c_str());

	if (severity == 2)
	{
		box->labelcolor(ERROR_MSG_COLOR);
		box->labelfont(FL_HELVETICA_BOLD);
	}
	else if (severity == 1)
	{
		box->labelcolor(WARNING_MSG_COLOR);
		box->labelfont(FL_HELVETICA_BOLD);
	}

	line_group->add(box);

	cx += W;

	if (button1)
	{
		Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button1);
		but->callback(cb1, this);

		line_group->add(but);

		cx += but->w() + 10;
	}

	if (button2)
	{
		Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button2);
		but->callback(cb2, this);

		line_group->add(but);

		cx += but->w() + 10;
	}

	if (button3)
	{
		Fl_Button *but = new Fl_Button(cx, cy, 80, 25, button3);
		but->callback(cb3, this);

		line_group->add(but);
	}

	cy = cy + 30;

	if (severity > worst_severity)
		worst_severity = severity;
}


CheckResult UI_Check_base::Run()
{
	set_modal();

	show();

	while (! (want_close || user_action != CheckResult::ok))
		Fl::wait(0.2);

	if (user_action != CheckResult::ok)
		return user_action;

	switch (worst_severity)
	{
		case 0:  return CheckResult::ok;
		case 1:  return CheckResult::minorProblem;
		default: return CheckResult::majorProblem;
	}
}


//------------------------------------------------------------------------

static void Vertex_FindDanglers(selection_c& sel, const Document &doc)
{
	sel.change_type(ObjType::vertices);

	if (doc.numVertices() == 0 || doc.numLinedefs() == 0)
		return;

	byte * line_counts = new byte[doc.numVertices()];

	memset(line_counts, 0, doc.numVertices());

	for (const auto &L : doc.linedefs)
	{
		int v1 = L->start;
		int v2 = L->end;

		// dangling vertices are fine for lines setting inside a sector
		// (i.e. with same sector on both sides)
		if (L->TwoSided() && (doc.getSectorID(*L, Side::left) == doc.getSectorID(*L, Side::right)))
		{
			line_counts[v1] = line_counts[v2] = 2;
			continue;
		}

		if (line_counts[v1] < 2) line_counts[v1] += 1;
		if (line_counts[v2] < 2) line_counts[v2] += 1;
	}

	for (int k = 0 ; k < doc.numVertices(); k++)
	{
		if (line_counts[k] == 1)
			sel.set(k);
	}

	delete[] line_counts;
}


static void Vertex_ShowDanglers(Instance &inst)
{
	if (inst.edit.mode != ObjType::vertices)
		inst.Editor_ChangeMode('v');

	Vertex_FindDanglers(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


struct vertex_X_CMP_pred
{
	const Document &doc;
	explicit vertex_X_CMP_pred(const Document &doc) : doc(doc)
	{
	}

	inline bool operator() (int A, int B) const
	{
		const auto V1 = doc.vertices[A];
		const auto V2 = doc.vertices[B];

		return V1->raw_x < V2->raw_x;
	}
};


void Vertex_FindOverlaps(selection_c& sel, const Document &doc)
{
	// NOTE: when two or more vertices share the same coordinates,
	//       only the second and subsequent ones are stored in 'sel'.

	sel.change_type(ObjType::vertices);

	if (doc.numVertices() < 2)
		return;

	// sort the vertices into order of the 'X' value.
	// hence any overlapping vertices will be near each other.

	std::vector<int> sorted_list(doc.numVertices(), 0);

	for (int i = 0 ; i < doc.numVertices(); i++)
		sorted_list[i] = i;

	std::sort(sorted_list.begin(), sorted_list.end(), vertex_X_CMP_pred(doc));

#define VERT_K  doc.vertices[sorted_list[k]]
#define VERT_N  doc.vertices[sorted_list[n]]

	for (int k = 0 ; k < doc.numVertices(); k++)
	{
		for (int n = k + 1 ; n < doc.numVertices() && VERT_N->raw_x == VERT_K->raw_x ; n++)
		{
			if (VERT_N->raw_y == VERT_K->raw_y)
			{
				sel.set(sorted_list[k]);
			}
		}
	}

#undef VERT_K
#undef VERT_N
}


static void Vertex_MergeOne(EditOperation &op, int idx, selection_c& merge_verts, Document &doc)
{
	const auto V = doc.vertices[idx];

	// find the base vertex (the one V is sitting on)
	for (int n = 0 ; n < doc.numVertices(); n++)
	{
		if (n == idx)
			continue;

		// skip any in the merge list
		if (merge_verts.get(n))
			continue;

		const auto N = doc.vertices[n];

		if (*N != *V)
			continue;

		// Ok, found it, so update linedefs

		for (int ld = 0 ; ld < doc.numLinedefs(); ld++)
		{
			const auto L = doc.linedefs[ld];

			if (L->start == idx)
				op.changeLinedef(ld, LineDef::F_START, n);

			if (L->end == idx)
				op.changeLinedef(ld, LineDef::F_END, n);
		}

		return;
	}

	// SHOULD NOT GET HERE
	gLog.printf("VERTEX MERGE FAILURE.\n");
}


static void Vertex_MergeOverlaps(Instance &inst)
{
	selection_c verts;
	Vertex_FindOverlaps(verts, inst.level);

	{
		EditOperation op(inst.level.basis);
		op.setMessage("merged overlapping vertices");

		for (sel_iter_c it(verts) ; !it.done() ; it.next())
		{
			Vertex_MergeOne(op, *it, verts, inst.level);
		}

		// nothing should reference these vertices now
		inst.level.objects.del(op, verts);
	}

	inst.RedrawMap();
}


static void Vertex_ShowOverlaps(Instance &inst)
{
	if (inst.edit.mode != ObjType::vertices)
		inst.Editor_ChangeMode('v');

	Vertex_FindOverlaps(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void Vertex_FindUnused(selection_c& sel, const Document &doc)
{
	sel.change_type(ObjType::vertices);

	if (doc.numVertices() == 0)
		return;

	for (const auto &linedef : doc.linedefs)
	{
		sel.set(linedef->start);
		sel.set(linedef->end);
	}

	sel.frob_range(0, doc.numVertices() - 1, BitOp::toggle);
}


static void Vertex_RemoveUnused(Document &doc)
{
	selection_c sel;

	Vertex_FindUnused(sel, doc);

	EditOperation op(doc.basis);
	op.setMessage("removed unused vertices");

	doc.objects.del(op, sel);
}


static void Vertex_ShowUnused(Instance &inst)
{
	if (inst.edit.mode != ObjType::vertices)
		inst.Editor_ChangeMode('v');

	Vertex_FindUnused(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Vertices : public UI_Check_base
{
public:
	UI_Check_Vertices(bool all_mode, Instance &inst) :
		UI_Check_base(520, 224, all_mode, "Check : Vertices",
				      "Vertex test results"), inst(inst)
	{ }

	static void action_merge(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_MergeOverlaps(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_highlight(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_ShowOverlaps(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_unused(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_ShowUnused(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_remove(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_RemoveUnused(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_show_danglers(Fl_Widget *w, void *data)
	{
		UI_Check_Vertices *dialog = (UI_Check_Vertices *)data;
		Vertex_ShowDanglers(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}
private:
	Instance &inst;
};


CheckResult ChecksModule::checkVertices(int min_severity) const
{
	UI_Check_Vertices *dialog = new UI_Check_Vertices(min_severity > 0, inst);

	selection_c  sel;

	SString check_message;

	for (;;)
	{
		Vertex_FindOverlaps(sel, doc);

		if (sel.empty())
			dialog->AddLine("No overlapping vertices");
		else
		{
			check_message = SString::printf("%d overlapping vertices", sel.count_obj());

			dialog->AddLine(check_message.c_str(), 2, 210,
			                "Show",  &UI_Check_Vertices::action_highlight,
			                "Merge", &UI_Check_Vertices::action_merge);
		}


		Vertex_FindDanglers(sel, doc);

		if (sel.empty())
			dialog->AddLine("No dangling vertices");
		else
		{
			check_message = SString::printf("%d dangling vertices", sel.count_obj());

			dialog->AddLine(check_message, 2, 210,
			                "Show",  &UI_Check_Vertices::action_show_danglers);
		}


		Vertex_FindUnused(sel, doc);

		if (sel.empty())
			dialog->AddLine("No unused vertices");
		else
		{
			check_message = SString::printf("%d unused vertices", sel.count_obj());

			dialog->AddLine(check_message, 1, 210,
			                "Show",   &UI_Check_Vertices::action_show_unused,
			                "Remove", &UI_Check_Vertices::action_remove);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CheckResult::ok;
		}

		CheckResult result = dialog->Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//------------------------------------------------------------------------

static void Sectors_FindUnclosed(selection_c& secs, selection_c& verts, const Document &doc)
{
	 secs.change_type(ObjType::sectors);
	verts.change_type(ObjType::vertices);

	if (doc.numVertices() == 0 || doc.numSectors() == 0)
		return;

	byte *ends = new byte[doc.numVertices()];
	int v;

	for (int s = 0 ; s < doc.numSectors(); s++)
	{
		// clear the "ends" array
		for (v = 0 ; v < doc.numVertices(); v++)
			ends[v] = 0;

		// for each sidedef bound to the Sector, store a "1" in the "ends"
		// array for its starting vertex, and a "2" for its ending vertex.
		for (const auto &L : doc.linedefs)
		{
			if (! doc.touchesSector(*L, s))
				continue;

			// ignore lines with same sector on both sides
			if (L->left >= 0 && L->right >= 0 &&
			    doc.getLeft(*L)->sector == doc.getRight(*L)->sector)
				continue;

			if (L->right >= 0 && doc.getRight(*L)->sector == s)
			{
				ends[L->start] |= 1;
				ends[L->end]   |= 2;
			}

			if (L->left >= 0 && doc.getLeft(*L)->sector == s)
			{
				ends[L->start] |= 2;
				ends[L->end]   |= 1;
			}
		}

		// every entry in the "ends" array should be 0 or 3

		for (v = 0 ; v < doc.numVertices(); v++)
		{
			if (ends[v] == 1 || ends[v] == 2)
			{
				 secs.set(s);
				verts.set(v);
			}
		}
	}

	delete[] ends;
}


static void Sectors_ShowUnclosed(ObjType what, Instance &inst)
{
	if (inst.edit.mode != what)
		inst.Editor_ChangeMode((what == ObjType::sectors) ? 's' : 'v');

	selection_c other;

	if (what == ObjType::sectors)
		Sectors_FindUnclosed(*inst.edit.Selected, other, inst.level);
	else
		Sectors_FindUnclosed(other, *inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void Sectors_FindMismatches(selection_c& secs, selection_c& lines, Instance &inst)
{
	//
	// Note from RQ:
	// This is a very simple idea, but it works!  The first test (above)
	// checks that all Sectors are closed.  But if a closed set of LineDefs
	// is moved out of a Sector and has all its "external" SideDefs pointing
	// to that Sector instead of the new one, then we need a second test.
	// That's why I check if the SideDefs facing each other are bound to
	// the same Sector.
	//
	// Other note from RQ:
	// Nowadays, what makes the power of a good editor is its automatic tests.
	// So, if you are writing another Doom editor, you will probably want
	// to do the same kind of tests in your program.  Fine, but if you use
	// these ideas, don't forget to credit DEU...  Just a reminder... :-)
	//

	 secs.change_type(ObjType::sectors);
	lines.change_type(ObjType::linedefs);
	
	Document &doc = inst.level;

	if (doc.numLinedefs() == 0 || doc.numSectors() == 0)
		return;

	FastOppositeTree tree(inst);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->right >= 0)
		{
			int s = doc.hover.getOppositeSector(n, Side::right, &tree);

			if (s < 0 || doc.getRight(*L)->sector != s)
			{
				 secs.set(doc.getRight(*L)->sector);
				lines.set(n);
			}
		}

		if (L->left >= 0)
		{
			int s = doc.hover.getOppositeSector(n, Side::left, &tree);

			if (s < 0 || doc.getLeft(*L)->sector != s)
			{
				 secs.set(doc.getLeft(*L)->sector);
				lines.set(n);
			}
		}
	}
}


static void Sectors_ShowMismatches(ObjType what, Instance &inst)
{
	if (inst.edit.mode != what)
		inst.Editor_ChangeMode((what == ObjType::sectors) ? 's' : 'l');

	selection_c other;

	if (what == ObjType::sectors)
		Sectors_FindMismatches(*inst.edit.Selected, other, inst);
	else
		Sectors_FindMismatches(other, *inst.edit.Selected, inst);

	inst.GoToErrors();
}


static void bump_unknown_type(std::map<int, int>& t_map, int type)
{
	int count = 0;

	if (t_map.find(type) != t_map.end())
		count = t_map[type];

	t_map[type] = count + 1;
}


static void Sectors_FindUnknown(selection_c& list, std::map<int, int>& types, const Instance &inst)
{
	types.clear();

	list.change_type(ObjType::sectors);

	int max_type = (inst.conf.features.gen_sectors == GenSectorFamily::zdoom) ? 8191 : 2047;

	for (int n = 0 ; n < inst.level.numSectors(); n++)
	{
		int type_num = inst.level.sectors[n]->type;

		// always ignore type #0
		if (type_num == 0)
			continue;

		if (type_num < 0 || type_num > max_type)
		{
			bump_unknown_type(types, type_num);
			list.set(n);
			continue;
		}

		// Boom and ZDoom generalized sectors
		if (inst.conf.features.gen_sectors == GenSectorFamily::zdoom)
			type_num &= 255;
		else if (inst.conf.features.gen_sectors != GenSectorFamily::none)
			type_num &= 31;

		const sectortype_t &info = inst.M_GetSectorType(type_num);

		if (info.desc.startsWith("UNKNOWN"))
		{
			bump_unknown_type(types, type_num);
			list.set(n);
		}
	}
}


static void Sectors_ShowUnknown(Instance &inst)
{
	if (inst.edit.mode != ObjType::sectors)
		inst.Editor_ChangeMode('s');

	std::map<int, int> types;

	Sectors_FindUnknown(*inst.edit.Selected, types, inst);

	inst.GoToErrors();
}


static void Sectors_LogUnknown(const Instance &inst)
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	Sectors_FindUnknown(sel, types, inst);

	gLog.printf("\n");
	gLog.printf("Unknown Sector Types:\n");
	gLog.printf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		gLog.printf("  %5d  x %d\n", IT->first, IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


static void Sectors_ClearUnknown(Instance &inst)
{
	selection_c sel;
	std::map<int, int> types;

	Sectors_FindUnknown(sel, types, inst);

	EditOperation op(inst.level.basis);
	op.setMessage("cleared unknown sector types");

	for (sel_iter_c it(sel) ; !it.done() ; it.next())
		op.changeSector(*it, Sector::F_TYPE, 0);
}


void Sectors_FindUnused(selection_c& sel, const Document &doc)
{
	sel.change_type(ObjType::sectors);

	if (doc.numSectors() == 0)
		return;

	for (const auto &L : doc.linedefs)
	{
		if (L->left >= 0)
			sel.set(doc.getLeft(*L)->sector);

		if (L->right >= 0)
			sel.set(doc.getRight(*L)->sector);
	}

	sel.frob_range(0, doc.numSectors() - 1, BitOp::toggle);
}


static void Sectors_RemoveUnused(Document &doc)
{
	selection_c sel;

	Sectors_FindUnused(sel, doc);

	EditOperation op(doc.basis);
	op.setMessage("removed unused sectors");

	doc.objects.del(op, sel);
}


static void Sectors_FindBadCeil(selection_c& sel, const Document &doc)
{
	sel.change_type(ObjType::sectors);

	if (doc.numSectors() == 0)
		return;

	for (int i = 0 ; i < doc.numSectors(); i++)
	{
		if (doc.sectors[i]->ceilh < doc.sectors[i]->floorh)
			sel.set(i);
	}
}


static void Sectors_FixBadCeil(Document &doc)
{
	selection_c sel;

	Sectors_FindBadCeil(sel, doc);

	EditOperation op(doc.basis);
	op.setMessage("fixed bad sector heights");

	for (int i = 0 ; i < doc.numSectors(); i++)
	{
		if (doc.sectors[i]->ceilh < doc.sectors[i]->floorh)
		{
			op.changeSector(i, Sector::F_CEILH, doc.sectors[i]->floorh);
		}
	}
}


static void Sectors_ShowBadCeil(Instance &inst)
{
	if (inst.edit.mode != ObjType::sectors)
		inst.Editor_ChangeMode('s');

	Sectors_FindBadCeil(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void SideDefs_FindUnused(selection_c& sel, const Document &doc)
{
	sel.change_type(ObjType::sidedefs);

	if (doc.numSidedefs() == 0)
		return;

	for (const auto &L : doc.linedefs)
	{
		if (L->left  >= 0) sel.set(L->left);
		if (L->right >= 0) sel.set(L->right);
	}

	sel.frob_range(0, doc.numSidedefs() - 1, BitOp::toggle);
}


static void SideDefs_RemoveUnused(Document &doc)
{
	selection_c sel;

	SideDefs_FindUnused(sel, doc);

	EditOperation op(doc.basis);
	op.setMessage("removed unused sidedefs");

	doc.objects.del(op, sel);
}


static void SideDefs_FindPacking(selection_c& sides, selection_c& lines, const Document &doc)
{
	sides.change_type(ObjType::sidedefs);
	lines.change_type(ObjType::linedefs);

	for (int i = 0 ; i < doc.numLinedefs(); i++)
	for (int k = 0 ; k < i ; k++)
	{
		const auto A = doc.linedefs[i];
		const auto B = doc.linedefs[k];

		bool AA = (A->left  >= 0 && A->left == A->right);

		bool AL = (A->left  >= 0 && (A->left  == B->left || A->left  == B->right));
		bool AR = (A->right >= 0 && (A->right == B->left || A->right == B->right));

		if (AL || AA) sides.set(A->left);
		if (AR)       sides.set(A->right);

		if (AL || AR)
		{
			lines.set(i);
			lines.set(k);
		}
		else if (AA)
		{
			lines.set(i);
		}
	}
}


static void SideDefs_ShowPacked(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	selection_c sides;

	SideDefs_FindPacking(sides, *inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


int ChecksModule::copySidedef(EditOperation &op, int num) const
{
	int sd = op.addNew(ObjType::sidedefs);

	*doc.sidedefs[sd] = *doc.sidedefs[num];

	return sd;
}


static const char *const unpack_confirm_message =
	"This map contains shared sidedefs.  It it recommended to unpack "
	"them, otherwise it may cause unexpected behavior during editing "
	"(such as random walls changing their texture).\n\n"
	"Unpack the sidedefs now?";


void ChecksModule::sidedefsUnpack(bool is_after_load) const
{
	selection_c sides;
	selection_c lines;

	SideDefs_FindPacking(sides, lines, doc);

	if (sides.empty())
		return;

	if ((false) /* confirm_it */)
	{
		if (DLG_Confirm({ "&No Change", "&Unpack" }, unpack_confirm_message) <= 0)
			return;
	}

	{
		EditOperation op(doc.basis);

		for (int sd = 0 ; sd < doc.numSidedefs(); sd++)
		{
			if (! sides.get(sd))
				continue;

			// find the first linedef which uses this sidedef
			int first;

			for (first = 0 ; first < doc.numLinedefs(); first++)
			{
				const auto F = doc.linedefs[first];

				if (F->left == sd || F->right == sd)
					break;
			}

			if (first >= doc.numLinedefs())
				continue;

			// handle it when first linedef uses sidedef on both sides
			if (doc.linedefs[first]->left == doc.linedefs[first]->right)
			{
				op.changeLinedef(first, LineDef::F_LEFT, copySidedef(op, sd));
			}

			// duplicate any remaining references
			for (int ld = first + 1 ; ld < doc.numLinedefs(); ld++)
			{
				if (doc.linedefs[ld]->left == sd)
					op.changeLinedef(ld, LineDef::F_LEFT, copySidedef(op, sd));

				if (doc.linedefs[ld]->right == sd)
					op.changeLinedef(ld, LineDef::F_RIGHT, copySidedef(op, sd));
			}
		}

		if (is_after_load)
		{
			op.setAbort(true /* keep changes */);
		}
		else
		{
			op.setMessage("unpacked all sidedefs");
		}
	}

	gLog.printf("Unpacked %d shared sidedefs --> %d\n", sides.count_obj(), doc.numSidedefs());
}


//------------------------------------------------------------------------

class UI_Check_Sectors : public UI_Check_base
{
public:
	UI_Check_Sectors(bool all_mode, Instance &inst) :
		UI_Check_base(530, 346, all_mode, "Check : Sectors",
				      "Sector test results"), inst(inst)
	{ }

	static void action_remove(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_RemoveUnused(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_remove_sidedefs(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		SideDefs_RemoveUnused(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_fix_ceil(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_FixBadCeil(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_show_ceil(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowBadCeil(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_unpack(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		dialog->inst.level.checks.sidedefsUnpack(false);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_packed(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		SideDefs_ShowPacked(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_unclosed(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnclosed(ObjType::sectors, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_un_verts(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnclosed(ObjType::vertices, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_mismatch(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowMismatches(ObjType::sectors, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_mis_lines(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowMismatches(ObjType::linedefs, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ShowUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_LogUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_clear_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Sectors *dialog = (UI_Check_Sectors *)data;
		Sectors_ClearUnknown(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}

private:
	Instance &inst;
};


CheckResult ChecksModule::checkSectors(int min_severity) const
{
	UI_Check_Sectors *dialog = new UI_Check_Sectors(min_severity > 0, inst);

	selection_c  sel, other;

	std::map<int, int> types;
	SString check_message;

	for (;;)
	{
		Sectors_FindUnclosed(sel, other, doc);

		if (sel.empty())
			dialog->AddLine("No unclosed sectors");
		else
		{
			check_message = SString::printf("%d unclosed sectors", sel.count_obj());

			dialog->AddLine(check_message, 2, 220,
			                "Show",  &UI_Check_Sectors::action_show_unclosed,
			                "Verts", &UI_Check_Sectors::action_show_un_verts);
		}


		Sectors_FindMismatches(sel, other, inst);

		if (sel.empty())
			dialog->AddLine("No mismatched sectors");
		else
		{
			check_message = SString::printf("%d mismatched sectors", sel.count_obj());

			dialog->AddLine(check_message, 2, 220,
			                "Show",  &UI_Check_Sectors::action_show_mismatch,
			                "Lines", &UI_Check_Sectors::action_show_mis_lines);
		}


		Sectors_FindBadCeil(sel, doc);

		if (sel.empty())
			dialog->AddLine("No sectors with ceil < floor");
		else
		{
			check_message = SString::printf("%d sectors with ceil < floor", sel.count_obj());

			dialog->AddLine(check_message, 2, 220,
			                "Show", &UI_Check_Sectors::action_show_ceil,
			                "Fix",  &UI_Check_Sectors::action_fix_ceil);
		}

		dialog->AddGap(10);


		Sectors_FindUnknown(sel, types, inst);

		if (sel.empty())
			dialog->AddLine("No unknown sector types");
		else
		{
			check_message = SString::printf("%d unknown sector types", (int)types.size());

			dialog->AddLine(check_message, 2, 220,
			                "Show",   &UI_Check_Sectors::action_show_unknown,
			                "Log",    &UI_Check_Sectors::action_log_unknown,
			                "Clear",  &UI_Check_Sectors::action_clear_unknown);
		}


		SideDefs_FindPacking(sel, other, doc);

		if (sel.empty())
			dialog->AddLine("No shared sidedefs");
		else
		{
			int approx_num = sel.count_obj();

			check_message = SString::printf("%d shared sidedefs", approx_num);

			dialog->AddLine(check_message, 1, 200,
			                "Show",   &UI_Check_Sectors::action_show_packed,
			                "Unpack", &UI_Check_Sectors::action_unpack);
		}


		Sectors_FindUnused(sel, doc);

		if (sel.empty())
			dialog->AddLine("No unused sectors");
		else
		{
			check_message = SString::printf("%d unused sectors", sel.count_obj());

			dialog->AddLine(check_message, 1, 170,
			                "Remove", &UI_Check_Sectors::action_remove);
		}


		SideDefs_FindUnused(sel, doc);

		if (sel.empty())
			dialog->AddLine("No unused sidedefs");
		else
		{
			check_message = SString::printf("%d unused sidedefs", sel.count_obj());

			dialog->AddLine(check_message, 1, 170,
			                "Remove", &UI_Check_Sectors::action_remove_sidedefs);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CheckResult::ok;
		}

		CheckResult result = dialog->Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//------------------------------------------------------------------------

void Things_FindUnknown(selection_c& list, std::map<int, int>& types, const Instance &inst)
{
	types.clear();

	list.change_type(ObjType::things);

	for (int n = 0 ; n < inst.level.numThings() ; n++)
	{
		const thingtype_t &info = inst.conf.getThingType(inst.level.things[n]->type);

		if (info.desc.startsWith("UNKNOWN"))
		{
			bump_unknown_type(types, inst.level.things[n]->type);

			list.set(n);
		}
	}
}


static void Things_ShowUnknown(Instance &inst)
{
	if (inst.edit.mode != ObjType::things)
		inst.Editor_ChangeMode('t');

	std::map<int, int> types;

	Things_FindUnknown(*inst.edit.Selected, types, inst);

	inst.GoToErrors();
}


static void Things_LogUnknown(const Instance &inst)
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	Things_FindUnknown(sel, types, inst);

	gLog.printf("\n");
	gLog.printf("Unknown Things:\n");
	gLog.printf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		gLog.printf("  %5d  x %d\n", IT->first, IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


void Things_RemoveUnknown(Instance &inst)
{
	selection_c sel;

	std::map<int, int> types;

	Things_FindUnknown(sel, types, inst);

	EditOperation op(inst.level.basis);
	op.setMessage("removed unknown things");

	inst.level.objects.del(op, sel);
}


// this returns a bitmask : bits 0..3 for players 1..4
static int Things_FindStarts(int *dm_num, const Document &doc)
{
	*dm_num = 0;

	int mask = 0;

	for(const auto &T : doc.things)
	{
		// ideally, these type numbers would not be hard-coded....

		switch (T->type)
		{
			case 1: mask |= (1 << 0); break;
			case 2: mask |= (1 << 1); break;
			case 3: mask |= (1 << 2); break;
			case 4: mask |= (1 << 3); break;

			case 11: *dm_num += 1; break;
		}
	}

	return mask;
}


static void Things_FindInVoid(selection_c& list, const Instance &inst)
{
	list.change_type(ObjType::things);

	for (int n = 0 ; n < inst.level.numThings() ; n++)
	{
		v2double_t pos = inst.level.things[n]->xy();

		Objid obj = hover::getNearestSector(inst.level, pos);

		if (! obj.is_nil())
			continue;

		// allow certain things in the void (Heretic sounds)
		const thingtype_t &info = inst.conf.getThingType(inst.level.things[n]->type);

		if (info.flags & THINGDEF_VOID)
			continue;

		// check more coords around the thing's centre, to be sure
		int out_count = 0;

		for (int corner = 0 ; corner < 4 ; corner++)
		{
			v2double_t pos2 = pos + v2double_t{ corner & 1 ? -4.0 : +4.0, corner & 2 ? -4.0 : +4.0 };

			obj = hover::getNearestSector(inst.level, pos2);

			if (obj.is_nil())
				out_count++;
		}

		if (out_count == 4)
			list.set(n);
	}
}


static void Things_ShowInVoid(Instance &inst)
{
	if (inst.edit.mode != ObjType::things)
		inst.Editor_ChangeMode('t');

	Things_FindInVoid(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


static void Things_RemoveInVoid(Instance &inst)
{
	selection_c sel;

	Things_FindInVoid(sel, inst);

	EditOperation op(inst.level.basis);
	op.setMessage("removed things in the void");

	inst.level.objects.del(op, sel);
}


// returns true if the game engine ALWAYS spawns this thing
// (i.e. the skill-flags and mode-flags are ignored).
static bool TH_always_spawned(const Instance &inst, int type)
{
	const thingtype_t &info = inst.conf.getThingType(type);

	// a player?
	if (1 <= type && type <= 4)
		return true;

	// a deathmatch start?
	if (type == 11)
		return true;

	// Polyobject things
	if (info.desc.find("Polyobj") != SString::npos ||
		info.desc.find("PolyObj") != SString::npos)
	{
		return true;
	}

	// ambient sounds in Heretic and Hexen
	if (info.desc.find("Snd") != SString::npos || info.desc.find("Sound") != SString::npos)
		return true;

	return false;
}


static void Things_FindDuds(const Instance &inst, selection_c& list)
{
	list.change_type(ObjType::things);

	for (int n = 0 ; n < inst.level.numThings() ; n++)
	{
		const auto T = inst.level.things[n];

		if (T->type == CAMERA_PEST)
			continue;

		int skills  = T->options & (MTF_Easy | MTF_Medium | MTF_Hard);
		int modes   = 1;
		int classes = 1;

		if (inst.loaded.levelFormat != MapFormat::doom)
		{
			modes = T->options & (MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM);
		}
		else if (inst.conf.features.coop_dm_flags)
		{
			modes = (~T->options) & (MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);
		}

		if (inst.loaded.levelFormat != MapFormat::doom)
		{
			classes = T->options & (MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage);
		}

		if (skills == 0 || modes == 0 || classes == 0)
		{
			if (! TH_always_spawned(inst, T->type))
				list.set(n);
		}
	}
}


static void Things_ShowDuds(Instance &inst)
{
	if (inst.edit.mode != ObjType::things)
		inst.Editor_ChangeMode('t');

	Things_FindDuds(inst, *inst.edit.Selected);

	inst.GoToErrors();
}


void Things_FixDuds(Instance &inst)
{
	EditOperation op(inst.level.basis);
	op.setMessage("fixed unspawnable things");

	for (int n = 0 ; n < inst.level.numThings() ; n++)
	{
		const auto T = inst.level.things[n];

		// NOTE: we also "fix" things that are always spawned
		////   if (TH_always_spawned(T->type)) continue;

		if (T->type == CAMERA_PEST)
			continue;

		int new_options = T->options;

		int skills  = T->options & (MTF_Easy | MTF_Medium | MTF_Hard);
		int modes   = 1;
		int classes = 1;

		if (skills == 0)
			new_options |= MTF_Easy | MTF_Medium | MTF_Hard;

		if (inst.loaded.levelFormat != MapFormat::doom)
		{
			modes = T->options & (MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM);

			if (modes == 0)
				new_options |= MTF_Hexen_SP | MTF_Hexen_COOP | MTF_Hexen_DM;
		}
		else if (inst.conf.features.coop_dm_flags)
		{
			modes = (~T->options) & (MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);

			if (modes == 0)
				new_options &= ~(MTF_Not_SP | MTF_Not_COOP | MTF_Not_DM);
		}

		if (inst.loaded.levelFormat != MapFormat::doom)
		{
			classes = T->options & (MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage);

			if (classes == 0)
				new_options |= MTF_Hexen_Cleric | MTF_Hexen_Fighter | MTF_Hexen_Mage;
		}

		if (new_options != T->options)
		{
			op.changeThing(n, Thing::F_OPTIONS, new_options);
		}
	}
}


//------------------------------------------------------------------------

static void CollectBlockingThings(std::vector<int>& list,
                                  std::vector<int>& sizes, const Instance &inst)
{
	for (int n = 0 ; n < inst.level.numThings() ; n++)
	{
		const auto T = inst.level.things[n];

		const thingtype_t &info = inst.conf.getThingType(T->type);

		if (info.flags & THINGDEF_PASS)
			continue;

		// ignore unknown things
		if (info.desc.startsWith("UNKNOWN"))
			continue;

		// TODO: config option: treat ceiling things as non-blocking

		 list.push_back(n);
		sizes.push_back(info.radius);
	}
}


/*
   andrewj: the DOOM movement code for monsters works by moving
   the actor by a stepping distance which is based on its 'speed'
   value.  The move is allowed when the *new position* has no
   blocking things or walls, which means that things can overlap
   a short distance and won't be stuck.

   Properly taking this into account requires knowing the speed of
   each individual monster, but we don't have that information here.
   Hence I've chosen a conservative value based on the speed of the
   slowest monster (8 units).

   TODO: make it either game config or user preference.
*/
#define MONSTER_STEP_DIST  8


static bool ThingStuckInThing(const Instance &inst, const Thing *T1, const thingtype_t *info1,
							  const Thing *T2, const thingtype_t *info2)
{
	SYS_ASSERT(T1 != T2);

	// require one thing to be a monster or player
	bool is_actor1 = (info1->group == 'm' || info1->group == 'p');
	bool is_actor2 = (info2->group == 'm' || info2->group == 'p');

	if (! (is_actor1 || is_actor2))
		return false;

	// check if T1 is stuck in T2
	int r1 = info1->radius;
	int r2 = info2->radius;

	if (info1->group == 'm' && info2->group != 'p')
		r1 = std::max(4, r1 - MONSTER_STEP_DIST);

	else if (info2->group == 'm' && info1->group != 'p')
		r2 = std::max(4, r2 - MONSTER_STEP_DIST);

	if (T1->x() - r1 >= T2->x() + r2) return false;
	if (T1->y() - r1 >= T2->y() + r2) return false;

	if (T1->x() + r1 <= T2->x() - r2) return false;
	if (T1->y() + r1 <= T2->y() - r2) return false;

	// teleporters and DM starts can safely overlap moving actors
	if ((info1->flags & THINGDEF_TELEPT) && is_actor2) return false;
	if ((info2->flags & THINGDEF_TELEPT) && is_actor1) return false;

	// check skill bits, except for players
	int opt1 = T1->options;
	int opt2 = T2->options;

	if (inst.loaded.levelFormat != MapFormat::doom)
	{
		if (info1->group == 'p') opt1 |= 0x7E7;
		if (info2->group == 'p') opt2 |= 0x7E7;

		// check skill bits
		if ((opt1 & opt2 & 0x07) == 0) return false;

		// check class bits
		if ((opt1 & opt2 & 0xE0) == 0) return false;

		// check game mode
		if ((opt1 & opt2 & 0x700) == 0) return false;
	}
	else
	{
		// invert game-mode bits (MTF_Not_COOP etc)
		opt1 ^= 0x70; opt2 ^= 0x70;

		if (info1->group == 'p') opt1 |= 0x77;
		if (info2->group == 'p') opt2 |= 0x77;

		// check skill bits
		if ((opt1 & opt2 & 0x07) == 0) return false;

		// check game mode
		if ((opt1 & opt2 & 0x70) == 0) return false;
	}

	return true;
}


static inline bool LD_is_blocking(const LineDef *L, const Document &doc)
{
#define MONSTER_HEIGHT  36

	// ignore virtual linedefs
	if (L->right < 0 && L->left < 0)
		return false;

	if (L->right < 0 || L->left < 0)
		return true;

	const auto &S1 = doc.getSector(*doc.getRight(*L));
	const auto &S2 = doc.getSector(*doc.getLeft(*L));

	int f_max = std::max(S1.floorh, S2.floorh);
	int c_min = std::min(S1.ceilh, S2.ceilh);

	return (c_min < f_max + MONSTER_HEIGHT);
}


static bool ThingStuckInWall(const Thing *T, int r, char group, const Document &doc)
{
	// only check players and monsters
	if (! (group == 'p' || group == 'm'))
		return false;

	if (group == 'm')
		r = std::max(4, r - MONSTER_STEP_DIST);

	// shrink a tiny bit, because we need to find lines which CROSS the
	// bounding box, not just touch it.
	r = r - 1;

	double x1 = T->x() - r;
	double y1 = T->y() - r;
	double x2 = T->x() + r;
	double y2 = T->y() + r;

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (! LD_is_blocking(L.get(), doc))
			continue;

		if (doc.objects.lineTouchesBox(n, x1, y1, x2, y2))
			return true;
	}

	return false;
}


static void Things_FindStuckies(selection_c& list, const Instance &inst)
{
	list.change_type(ObjType::things);

	std::vector<int> blockers;
	std::vector<int> sizes;

	CollectBlockingThings(blockers, sizes, inst);

	for (int n = 0 ; n < (int)blockers.size() ; n++)
	{
		const auto T = inst.level.things[blockers[n]];

		const thingtype_t &info = inst.conf.getThingType(T->type);

		if (ThingStuckInWall(T.get(), info.radius, info.group, inst.level))
			list.set(blockers[n]);

		for (int n2 = n + 1 ; n2 < (int)blockers.size() ; n2++)
		{
			const auto T2 = inst.level.things[blockers[n2]];

			const thingtype_t &info2 = inst.conf.getThingType(T2->type);

			if (ThingStuckInThing(inst, T.get(), &info, T2.get(), &info2))
				list.set(blockers[n]);
		}
	}
}


static void Things_ShowStuckies(Instance &inst)
{
	if (inst.edit.mode != ObjType::things)
		inst.Editor_ChangeMode('t');

	Things_FindStuckies(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Things : public UI_Check_base
{
public:
	UI_Check_Things(bool all_mode, Instance &inst) :
		UI_Check_base(520, 316, all_mode, "Check : Things",
				      "Thing test results"), inst(inst)
	{ }

public:
	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_LogUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_remove_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_RemoveUnknown(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_void(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowInVoid(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_remove_void(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_RemoveInVoid(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_show_stuck(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowStuckies(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_duds(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_ShowDuds(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_duds(Fl_Widget *w, void *data)
	{
		UI_Check_Things *dialog = (UI_Check_Things *)data;
		Things_FixDuds(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}
private:
	Instance &inst;
};


CheckResult ChecksModule::checkThings(int min_severity) const
{
	UI_Check_Things *dialog = new UI_Check_Things(min_severity > 0, inst);

	selection_c  sel;

	std::map<int, int> types;
	SString check_message;

	for (;;)
	{
		Things_FindUnknown(sel, types, inst);

		if (sel.empty())
			dialog->AddLine("No unknown thing types");
		else
		{
			check_message = SString::printf("%d unknown things", (int)types.size());

			dialog->AddLine(check_message, 2, 200,
			                "Show",   &UI_Check_Things::action_show_unknown,
			                "Log",    &UI_Check_Things::action_log_unknown,
			                "Remove", &UI_Check_Things::action_remove_unknown);
		}


		Things_FindStuckies(sel, inst);

		if (sel.empty())
			dialog->AddLine("No stuck actors");
		else
		{
			check_message = SString::printf("%d stuck actors", sel.count_obj());

			dialog->AddLine(check_message, 2, 200,
			                "Show",  &UI_Check_Things::action_show_stuck);
		}


		Things_FindInVoid(sel, inst);

		if (sel.empty())
			dialog->AddLine("No things in the void");
		else
		{
			check_message = SString::printf("%d things in the void", sel.count_obj());

			dialog->AddLine(check_message, 1, 200,
			                "Show",   &UI_Check_Things::action_show_void,
			                "Remove", &UI_Check_Things::action_remove_void);
		}


		Things_FindDuds(inst, sel);

		if (sel.empty())
			dialog->AddLine("No unspawnable things -- skill flags are OK");
		else
		{
			check_message = SString::printf("%d unspawnable things", sel.count_obj());
			dialog->AddLine(check_message, 1, 200,
			                "Show", &UI_Check_Things::action_show_duds,
			                "Fix",  &UI_Check_Things::action_fix_duds);
		}


		dialog->AddGap(10);


		int dm_num, mask;

		mask = Things_FindStarts(&dm_num, doc);

		if (inst.conf.features.no_need_players)
			dialog->AddLine("Player starts not needed, no check done");
		else if (! (mask & 1))
			dialog->AddLine("Player 1 start is missing!", 2);
		else if (! (mask & 2))
			dialog->AddLine("Player 2 start is missing", 1);
		else if (! (mask & 4))
			dialog->AddLine("Player 3 start is missing", 1);
		else if (! (mask & 8))
			dialog->AddLine("Player 4 start is missing", 1);
		else
			dialog->AddLine("Found all 4 player starts");

		if (inst.conf.features.no_need_players)
		{
			// leave a blank space
		}
		else if (dm_num == 0)
		{
			dialog->AddLine("Map is missing deathmatch starts", 1);
		}
		else if (dm_num < inst.conf.miscInfo.min_dm_starts)
		{
			check_message = SString::printf("Found %d deathmatch starts -- need at least %d", dm_num,
				inst.conf.miscInfo.min_dm_starts);
			dialog->AddLine(check_message, 1);
		}
		else if (dm_num > inst.conf.miscInfo.max_dm_starts)
		{
			check_message = SString::printf("Found %d deathmatch starts -- maximum is %d", dm_num,
				inst.conf.miscInfo.max_dm_starts);
			dialog->AddLine(check_message, 2);
		}
		else
		{
			check_message = SString::printf("Found %d deathmatch starts -- OK", dm_num);
			dialog->AddLine(check_message);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CheckResult::ok;
		}

		CheckResult result = dialog->Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//------------------------------------------------------------------------


static void LineDefs_FindZeroLen(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
		if (doc.isZeroLength(*doc.linedefs[n]))
			lines.set(n);
}


static void LineDefs_RemoveZeroLen(Document &doc)
{
	selection_c lines(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		if (doc.isZeroLength(*doc.linedefs[n]))
			lines.set(n);
	}

	EditOperation op(doc.basis);
	op.setMessage("removed zero-len linedefs");

	// NOTE: the vertex overlapping test handles cases where the
	//       vertices of other lines joining a zero-length one
	//       need to be merged.

	DeleteObjects_WithUnused(op, doc, lines, false, false, false);
}


static void LineDefs_ShowZeroLen(Instance &inst)
{
	if (inst.edit.mode != ObjType::vertices)
		inst.Editor_ChangeMode('v');

	selection_c sel;

	LineDefs_FindZeroLen(sel, inst.level);

	ConvertSelection(inst.level, sel, *inst.edit.Selected);

	inst.GoToErrors();
}


static void LineDefs_FindMissingRight(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
		if (doc.linedefs[n]->right < 0)
			lines.set(n);
}


static void LineDefs_ShowMissingRight(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindMissingRight(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void LineDefs_FindManualDoors(selection_c& lines, const Instance &inst)
{
	// find D1/DR manual doors on one-sided linedefs

	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->type <= 0)
			continue;

		if (L->left >= 0)
			continue;

		const linetype_t &info = inst.conf.getLineType(L->type);

		if (info.desc[0] == 'D' &&
			(info.desc[1] == '1' || info.desc[1] == 'R'))
		{
			lines.set(n);
		}
	}
}


static void LineDefs_ShowManualDoors(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindManualDoors(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


static void LineDefs_FixManualDoors(Instance &inst)
{
	EditOperation op(inst.level.basis);
	op.setMessage("fixed manual doors");

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->type <= 0 || L->left >= 0)
			continue;

		const linetype_t &info = inst.conf.getLineType(L->type);

		if (info.desc[0] == 'D' &&
			(info.desc[1] == '1' || info.desc[1] == 'R'))
		{
			op.changeLinedef(n, LineDef::F_TYPE, 0);
		}
	}
}


static void LineDefs_FindLackImpass(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->OneSided() && (L->flags & MLF_Blocking) == 0)
			lines.set(n);
	}
}


static void LineDefs_ShowLackImpass(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindLackImpass(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void LineDefs_FixLackImpass(Document &doc)
{
	EditOperation op(doc.basis);
	op.setMessage("fixed impassible flags");

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->OneSided() && (L->flags & MLF_Blocking) == 0)
		{
			int new_flags = L->flags | MLF_Blocking;

			op.changeLinedef(n, LineDef::F_FLAGS, new_flags);
		}
	}
}


static void LineDefs_FindBad2SFlag(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->OneSided() && (L->flags & MLF_TwoSided))
			lines.set(n);

		if (L->TwoSided() && ! (L->flags & MLF_TwoSided))
			lines.set(n);
	}
}


static void LineDefs_ShowBad2SFlag(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindBad2SFlag(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void LineDefs_FixBad2SFlag(Document &doc)
{
	EditOperation op(doc.basis);
	op.setMessage("fixed two-sided flags");

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->OneSided() && (L->flags & MLF_TwoSided))
			op.changeLinedef(n, LineDef::F_FLAGS, L->flags & ~MLF_TwoSided);

		if (L->TwoSided() && ! (L->flags & MLF_TwoSided))
			op.changeLinedef(n, LineDef::F_FLAGS, L->flags | MLF_TwoSided);
	}
}


static void bung_unknown_type(std::map<int, int>& t_map, int type)
{
	int count = 0;

	if (t_map.find(type) != t_map.end())
		count = t_map[type];

	t_map[type] = count + 1;
}


static void LineDefs_FindUnknown(selection_c& list, std::map<int, int>& types, const Instance &inst)
{
	types.clear();

	list.change_type(ObjType::linedefs);

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		int type_num = inst.level.linedefs[n]->type;

		// always ignore type #0
		if (type_num == 0)
			continue;

		const linetype_t &info = inst.conf.getLineType(type_num);

		// Boom generalized line type?
		if (inst.conf.features.gen_types && is_genline(type_num))
			continue;

		if (info.desc.startsWith("UNKNOWN"))
		{
			bung_unknown_type(types, type_num);

			list.set(n);
		}
	}
}


static void LineDefs_ShowUnknown(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	std::map<int, int> types;

	LineDefs_FindUnknown(*inst.edit.Selected, types, inst);

	inst.GoToErrors();
}


static void LineDefs_LogUnknown(const Instance &inst)
{
	selection_c sel;

	std::map<int, int> types;
	std::map<int, int>::iterator IT;

	LineDefs_FindUnknown(sel, types, inst);

	gLog.printf("\n");
	gLog.printf("Unknown Line Types:\n");
	gLog.printf("{\n");

	for (IT = types.begin() ; IT != types.end() ; IT++)
		gLog.printf("  %5d  x %d\n", IT->first, IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


static void LineDefs_ClearUnknown(Instance &inst)
{
	selection_c sel;
	std::map<int, int> types;

	LineDefs_FindUnknown(sel, types, inst);

	EditOperation op(inst.level.basis);
	op.setMessage("cleared unknown line types");

	for (sel_iter_c it(sel) ; !it.done() ; it.next())
		op.changeLinedef(*it, LineDef::F_TYPE, 0);
}


//------------------------------------------------------------------------


static int linedef_pos_cmp(int A, int B, const Document &doc)
{
	const auto AL = doc.linedefs[A];
	const auto BL = doc.linedefs[B];

	int A_x1 = static_cast<int>(doc.getStart(*AL).x());
	int A_y1 = static_cast<int>(doc.getStart(*AL).y());
	int A_x2 = static_cast<int>(doc.getEnd(*AL).x());
	int A_y2 = static_cast<int>(doc.getEnd(*AL).y());

	int B_x1 = static_cast<int>(doc.getStart(*BL).x());
	int B_y1 = static_cast<int>(doc.getStart(*BL).y());
	int B_x2 = static_cast<int>(doc.getEnd(*BL).x());
	int B_y2 = static_cast<int>(doc.getEnd(*BL).y());

	if (A_x1 > A_x2 || (A_x1 == A_x2 && A_y1 > A_y2))
	{
		std::swap(A_x1, A_x2);
		std::swap(A_y1, A_y2);
	}

	if (B_x1 > B_x2 || (B_x1 == B_x2 && B_y1 > B_y2))
	{
		std::swap(B_x1, B_x2);
		std::swap(B_y1, B_y2);
	}

	// the "normalized" X1 coordinates is the most significant thing in
	// this comparison function.

	if (A_x1 != B_x1) return A_x1 - B_x1;
	if (A_y1 != B_y1) return A_y1 - B_y1;

	if (A_x2 != B_x2) return A_x2 - B_x2;
	if (A_y2 != B_y2) return A_y2 - B_y2;

	return 0;  // equal : lines are overlapping
}


struct linedef_pos_CMP_pred
{
	const Document &doc;

	inline bool operator() (int A, int B) const
	{
		return linedef_pos_cmp(A, B, doc) < 0;
	}
};


struct linedef_minx_CMP_pred
{
	const Document &doc;

	inline bool operator() (int A, int B) const
	{
		const auto AL = doc.linedefs[A];
		const auto BL = doc.linedefs[B];

		FFixedPoint A_x = std::min(doc.getStart(*AL).raw_x, doc.getEnd(*AL).raw_x);
		FFixedPoint B_x = std::min(doc.getStart(*BL).raw_x, doc.getEnd(*BL).raw_x);

		return A_x < B_x;
	}
};


static void LineDefs_FindOverlaps(selection_c& lines, const Document &doc)
{
	// we only find directly overlapping linedefs here

	lines.change_type(ObjType::linedefs);

	if (doc.numLinedefs() < 2)
		return;

	int n;

	// sort linedefs by their position.  overlapping lines will end up
	// adjacent to each other after the sort.
	std::vector<int> sorted_list(doc.numLinedefs(), 0);

	for (n = 0 ; n < doc.numLinedefs(); n++)
		sorted_list[n] = n;

	std::sort(sorted_list.begin(), sorted_list.end(), linedef_pos_CMP_pred{ doc });

	for (n = 0 ; n < doc.numLinedefs() - 1 ; n++)
	{
		int ld1 = sorted_list[n];
		int ld2 = sorted_list[n + 1];

		// ignore zero-length lines
		if (doc.isZeroLength(*doc.linedefs[ld2]))
			continue;

		// only the second (or third, etc) linedef is stored
		if (linedef_pos_cmp(ld1, ld2, doc) == 0)
			lines.set(ld2);
	}
}


static void LineDefs_ShowOverlaps(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindOverlaps(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void LineDefs_RemoveOverlaps(Document &doc)
{
	selection_c lines, unused_verts;

	LineDefs_FindOverlaps(lines, doc);

	UnusedVertices(doc, lines, unused_verts);

	EditOperation op(doc.basis);
	op.setMessage("removed overlapping lines");

	doc.objects.del(op, lines);
	doc.objects.del(op, unused_verts);
}


static int CheckLinesCross(int A, int B, const Document &doc)
{
	// return values:
	//    0 : the lines do not cross
	//    1 : A is sitting on B (a 'T' junction)
	//    2 : B is sitting on A (a 'T' junction)
	//    3 : the lines cross each other (an 'X' junction)
	//    4 : the lines are co-linear and partially overlap

	const double epsilon = 0.02;


	SYS_ASSERT(A != B);

	const auto AL = doc.linedefs[A];
	const auto BL = doc.linedefs[B];

	// ignore zero-length lines
	if (doc.isZeroLength(*AL) || doc.isZeroLength(*BL))
		return 0;

	// ignore directly overlapping here
	if (linedef_pos_cmp(A, B, doc) == 0)
		return 0;


	// bbox test
	//
	// the algorithm in LineDefs_FindCrossings() ensures that A and B
	// already overlap on the X axis.  hence only check Y axis here.

	if (std::min(doc.getStart(*AL).raw_y, doc.getEnd(*AL).raw_y) >
		std::max(doc.getStart(*BL).raw_y, doc.getEnd(*BL).raw_y))
	{
		return 0;
	}

	if (std::min(doc.getStart(*BL).raw_y, doc.getEnd(*BL).raw_y) >
		std::max(doc.getStart(*AL).raw_y, doc.getEnd(*AL).raw_y))
	{
		return 0;
	}


	// precise (but slower) intersection test

	v2double_t av1 = doc.getStart(*AL).xy();
	v2double_t av2 = doc.getEnd(*AL).xy();

	v2double_t bv1 = doc.getStart(*BL).xy();
	v2double_t bv2 = doc.getEnd(*BL).xy();

	double c = PerpDist(bv1,  av1, av2);
	double d = PerpDist(bv2,  av1, av2);

	int c_side = (c < -epsilon) ? -1 : (c > epsilon) ? +1 : 0;
	int d_side = (d < -epsilon) ? -1 : (d > epsilon) ? +1 : 0;

	if (c_side != 0 && c_side == d_side)
		return 0;

	double e = PerpDist(av1,  bv1, bv2);
	double f = PerpDist(av2,  bv1, bv2);

	int e_side = (e < -epsilon) ? -1 : (e > epsilon) ? +1 : 0;
	int f_side = (f < -epsilon) ? -1 : (f > epsilon) ? +1 : 0;

	if (e_side != 0 && e_side == f_side)
		return 0;


	// check whether the two lines definitely cross each other
	// at a single point (like an 'X' shape), or not.
	bool a_crossed = (c_side * d_side != 0);
	bool b_crossed = (e_side * f_side != 0);

	if (a_crossed && b_crossed)
		return 3;


	// are the two lines are co-linear (or very close to it) ?
	// if so, check the separation between them...
	if ((c_side == 0 && d_side == 0) ||
		(e_side == 0 && f_side == 0))
	{
		// choose longest line as the measuring stick
		if (doc.calcLength(*AL) < doc.calcLength(*BL))
		{
			std::swap(av1, bv1);
			std::swap(av2, bv2);

			// A, B, AL, BL should not be used from here on!
		}

		c = AlongDist(bv1,  av1, av2);
		d = AlongDist(bv2,  av1, av2);
		e = AlongDist(av2,  av1, av2);	// just the length

		if (std::max(c, d) < epsilon)
			return 0;

		if (std::min(c, d) > e - epsilon)
			return 0;

		// colinear and partially overlapping
		return 4;
	}


	// this handles the case where the two linedefs meet at a vertex
	// but are not overlapping at all.
	if (! a_crossed && ! b_crossed)
		return 0;


	// in this case we have a 'T' junction, where the end-point of
	// one linedef is sitting along the other one.
	return a_crossed ? 2 : 1;
}


static void LineDefs_FindCrossings(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	if (doc.numLinedefs() < 2)
		return;

	int n;

	// sort linedefs by their position.  linedefs which cross will be
	// near each other in this list.
	std::vector<int> sorted_list(doc.numLinedefs(), 0);

	for (n = 0 ; n < doc.numLinedefs(); n++)
		sorted_list[n] = n;

	std::sort(sorted_list.begin(), sorted_list.end(), linedef_minx_CMP_pred{ doc });

	for (n = 0 ; n < doc.numLinedefs(); n++)
	{
		int n2 = sorted_list[n];

		const auto L1 = doc.linedefs[n2];

		FFixedPoint max_x = std::max(doc.getStart(*L1).raw_x, doc.getEnd(*L1).raw_x);

		for (int k = n + 1 ; k < doc.numLinedefs(); k++)
		{
			int k2 = sorted_list[k];

			const auto L2 = doc.linedefs[k2];

			FFixedPoint min_x = std::min(doc.getStart(*L2).raw_x, doc.getEnd(*L2).raw_x);

			// stop when all remaining linedefs are to the right of L1
			if (min_x > max_x)
				break;

			int res = CheckLinesCross(n2, k2, doc);

			if (res)
			{
				lines.set(n2);
				lines.set(k2);
			}
		}
	}
}


static void LineDefs_ShowCrossings(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	LineDefs_FindCrossings(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_LineDefs : public UI_Check_base
{
public:
	UI_Check_LineDefs(bool all_mode, Instance &inst) :
		UI_Check_base(530, 370, all_mode, "Check : LineDefs",
		              "LineDef test results"), inst(inst)
	{ }

public:
	static void action_show_zero(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowZeroLen(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_remove_zero(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_RemoveZeroLen(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_show_mis_right(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowMissingRight(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_manual_doors(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowManualDoors(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_manual_doors(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixManualDoors(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_lack_impass(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowLackImpass(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_lack_impass(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixLackImpass(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_bad_2s_flag(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowBad2SFlag(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_bad_2s_flag(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_FixBad2SFlag(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_log_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_LogUnknown(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_clear_unknown(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ClearUnknown(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_remove_overlap(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_RemoveOverlaps(dialog->inst.level);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_show_overlap(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowOverlaps(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_crossing(Fl_Widget *w, void *data)
	{
		UI_Check_LineDefs *dialog = (UI_Check_LineDefs *)data;
		LineDefs_ShowCrossings(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

private:
	Instance &inst;
};


CheckResult ChecksModule::checkLinedefs(int min_severity) const
{
	UI_Check_LineDefs *dialog = new UI_Check_LineDefs(min_severity > 0, inst);

	selection_c  sel, other;

	std::map<int, int> types;

	SString check_buffer;

	for (;;)
	{
		LineDefs_FindZeroLen(sel, doc);

		if (sel.empty())
			dialog->AddLine("No zero-length linedefs");
		else
		{
			check_buffer = SString::printf("%d zero-length linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 220,
			                "Show",   &UI_Check_LineDefs::action_show_zero,
			                "Remove", &UI_Check_LineDefs::action_remove_zero);
		}


		LineDefs_FindOverlaps(sel, doc);

		if (sel.empty())
			dialog->AddLine("No overlapping linedefs");
		else
		{
			check_buffer = SString::printf("%d overlapping linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 220,
			                "Show",   &UI_Check_LineDefs::action_show_overlap,
			                "Remove", &UI_Check_LineDefs::action_remove_overlap);
		}


		LineDefs_FindCrossings(sel, doc);

		if (sel.empty())
			dialog->AddLine("No criss-crossing linedefs");
		else
		{
			check_buffer = SString::printf("%d criss-crossing linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 220,
			                "Show", &UI_Check_LineDefs::action_show_crossing);
		}

		dialog->AddGap(10);


		LineDefs_FindUnknown(sel, types, inst);

		if (sel.empty())
			dialog->AddLine("No unknown line types");
		else
		{
			check_buffer = SString::printf("%d unknown line types", (int)types.size());

			dialog->AddLine(check_buffer, 1, 210,
			                "Show",   &UI_Check_LineDefs::action_show_unknown,
			                "Log",    &UI_Check_LineDefs::action_log_unknown,
			                "Clear",  &UI_Check_LineDefs::action_clear_unknown);
		}


		LineDefs_FindMissingRight(sel, doc);

		if (sel.empty())
			dialog->AddLine("No linedefs without a right side");
		else
		{
			check_buffer = SString::printf("%d linedefs without right side", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 300,
			                "Show", &UI_Check_LineDefs::action_show_mis_right);
		}


		LineDefs_FindManualDoors(sel, inst);

		if (sel.empty())
			dialog->AddLine("No manual doors on 1S linedefs");
		else
		{
			check_buffer = SString::printf("%d manual doors on 1S linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 2, 300,
			                "Show", &UI_Check_LineDefs::action_show_manual_doors,
			                "Fix",  &UI_Check_LineDefs::action_fix_manual_doors);
		}


		LineDefs_FindLackImpass(sel, doc);

		if (sel.empty())
			dialog->AddLine("No non-blocking one-sided linedefs");
		else
		{
			check_buffer = SString::printf("%d non-blocking one-sided linedefs", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 300,
			                "Show", &UI_Check_LineDefs::action_show_lack_impass,
			                "Fix",  &UI_Check_LineDefs::action_fix_lack_impass);
		}


		LineDefs_FindBad2SFlag(sel, doc);

		if (sel.empty())
			dialog->AddLine("No linedefs with wrong 2S flag");
		else
		{
			check_buffer = SString::printf("%d linedefs with wrong 2S flag", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 300,
			                "Show", &UI_Check_LineDefs::action_show_bad_2s_flag,
			                "Fix",  &UI_Check_LineDefs::action_fix_bad_2s_flag);
		}


		// in "ALL" mode, just continue if not too severe
		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CheckResult::ok;
		}

		CheckResult result = dialog->Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//------------------------------------------------------------------------

void ChecksModule::tagsUsedRange(int *min_tag, int *max_tag) const
{
	int i;

	*min_tag = INT_MAX;
	*max_tag = INT_MIN;

	for (i = 0 ; i < doc.numLinedefs(); i++)
	{
		int tag = doc.linedefs[i]->tag;

		if (tag > 0)
		{
			*min_tag = std::min(*min_tag, tag);
			*max_tag = std::max(*max_tag, tag);
		}
	}

	for (i = 0 ; i < doc.numSectors() ; i++)
	{
		int tag = doc.sectors[i]->tag;

		// ignore special tags
		if (inst.conf.features.tag_666 != Tag666Rules::disabled && (tag == 666 || tag == 667))
			continue;

		if (tag > 0)
		{
			*min_tag = std::min(*min_tag, tag);
			*max_tag = std::max(*max_tag, tag);
		}
	}

	// none at all?
	if (*min_tag > *max_tag)
	{
		*min_tag = *max_tag = 0;
	}
}


void ChecksModule::tagsApplyNewValue(int new_tag)
{
	// uses the current selection (caller must set it up)

	bool changed = false;

	{
		EditOperation op(doc.basis);
		op.setMessageForSelection("new tag for", *inst.edit.Selected);

		for (sel_iter_c it(*inst.edit.Selected); !it.done(); it.next())
		{
			if (inst.edit.mode == ObjType::linedefs)
			{
				op.changeLinedef(*it, LineDef::F_TAG, new_tag);
				changed = true;
			}
			else if (inst.edit.mode == ObjType::sectors)
			{
				op.changeSector(*it, Sector::F_TAG, new_tag);
				changed = true;
			}
		}
	}
	if(changed)
		inst.tagInMemory = new_tag;
}


void Instance::CMD_ApplyTag()
{
	if (! (edit.mode == ObjType::sectors || edit.mode == ObjType::linedefs))
	{
		Beep("ApplyTag: wrong mode");
		return;
	}

	bool do_last = false;

	SString mode = EXEC_Param[0];

	if (mode.empty() || mode.noCaseEqual("fresh"))
	{
		// fresh tag
	}
	else if (mode.noCaseEqual("last"))
	{
		do_last = true;
	}
	else
	{
		Beep("ApplyTag: unknown keyword: %s\n", mode.c_str());
		return;
	}

	SelectHighlight unselect = edit.SelectionOrHighlight();
	if (unselect == SelectHighlight::empty)
	{
		Beep("ApplyTag: nothing selected");
		return;
	}

	int new_tag;
	if(do_last)
		new_tag = tagInMemory;
	else
		new_tag = findFreeTag(*this, edit.mode == ObjType::sectors);

	if (new_tag <= 0)
	{
		Beep("No last tag");
	}
	else if (new_tag > 32767)
	{
		Beep("Out of tag numbers");
	}
	else
	{
		level.checks.tagsApplyNewValue(new_tag);
	}

	if (unselect == SelectHighlight::unselect)
		Selection_Clear(true /* nosave */);
}


static bool LD_tag_exists(int tag, const Document &doc)
{
	for (const auto &linedef : doc.linedefs)
		if (linedef->tag == tag)
			return true;

	return false;
}


static bool SEC_tag_exists(int tag, const Document &doc)
{
	for (const auto &sector : doc.sectors)
		if (sector->tag == tag)
			return true;

	return false;
}


static void Tags_FindUnmatchedSectors(selection_c& secs, const Instance &inst)
{
	secs.change_type(ObjType::sectors);

	for (int s = 0 ; s < inst.level.numSectors(); s++)
	{
		int tag = inst.level.sectors[s]->tag;

		if (tag <= 0)
			continue;

		// DOOM and Heretic use tag #666 to open doors (etc) on the
		// death of boss monsters.
		if (inst.conf.features.tag_666 != Tag666Rules::disabled && (tag == 666 || tag == 667))
			continue;

		if (! LD_tag_exists(tag, inst.level))
			secs.set(s);
	}
}


static void Tags_FindUnmatchedLineDefs(selection_c& lines, const Document &doc, const ConfigData &config)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		if (L->tag <= 0)
			continue;

		if (L->type <= 0)
			continue;

		SpecialTagInfo info = {};
		bool hasinfo = getSpecialTagInfo(ObjType::linedefs, n, L->type, L.get(), config, info);
		
		if(!hasinfo)
			continue;
		
		for(int i = 0; i < info.numtags; ++i)
		{
			if(!SEC_tag_exists(info.tags[i], doc))
			{
				lines.set(n);
				goto nextline;
			}
		}
		for(int i = 0; i < info.numlineids; ++i)
		{
			if(!LD_tag_exists(info.lineids[i], doc))
			{
				lines.set(n);
				goto nextline;
			}
		}
	nextline:
		;
	}
}


static void Tags_ShowUnmatchedSectors(Instance &inst)
{
	if (inst.edit.mode != ObjType::sectors)
		inst.Editor_ChangeMode('s');

	Tags_FindUnmatchedSectors(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


static void Tags_ShowUnmatchedLineDefs(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	Tags_FindUnmatchedLineDefs(*inst.edit.Selected, inst.level, inst.conf);

	inst.GoToErrors();
}


static void Tags_FindMissingTags(selection_c& lines, const Instance &inst)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->type <= 0)
			continue;

		if (L->tag > 0)
			continue;

		// use type description to determine if a tag is needed
		// e.g. D1, DR, --, and lowercase first letter all mean "no tag".

		// TODO: boom generalized manual doors (etc??)
		const linetype_t &info = inst.conf.getLineType(L->type);

		if(info.desc.empty())
		{
			gLog.printf("WARNING: invalid empty description for line type %d\n", L->type);
			continue;
		}
		
		char first = info.desc[0];

		if (first == 'D' || first == '-' || ('a' <= first && first <= 'z'))
			continue;

		lines.set(n);
	}
}


static void Tags_ShowMissingTags(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	Tags_FindMissingTags(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


static bool SEC_check_beast_mark(int tag, const Instance &inst)
{
	if (inst.conf.features.tag_666 == Tag666Rules::disabled)
		return true;

	if (tag == 667)
	{
		// tag #667 can only be used on MAP07
		return inst.loaded.levelName.noCaseEqual("MAP07");
	}

	if (tag == 666)
	{
		// for Heretic, the map must be an end-of-episode map: ExM8
		if (inst.conf.features.tag_666 == Tag666Rules::heretic)
		{
			if (inst.loaded.levelName.length() != 4)
				return false;

			return (inst.loaded.levelName[3] == '8');
		}

		// for Doom, either need a particular map, or the presence
		// of a KEEN thing.
		if (inst.loaded.levelName.noCaseEqual("E1M8") || inst.loaded.levelName.noCaseEqual("E4M6") ||
			inst.loaded.levelName.noCaseEqual("E4M8") || inst.loaded.levelName.noCaseEqual("MAP07"))
		{
			return true;
		}

		for (const auto &thing : inst.level.things)
		{
			const thingtype_t &info = inst.conf.getThingType(thing->type);

			if (info.desc.noCaseEqual("Commander Keen"))
				return true;
		}

		return false;
	}

	return true; // Ok
}


static void Tags_FindBeastMarks(selection_c& secs, const Instance &inst)
{
	secs.change_type(ObjType::sectors);

	for (int s = 0 ; s < inst.level.numSectors(); s++)
	{
		int tag = inst.level.sectors[s]->tag;

		if (! SEC_check_beast_mark(tag, inst))
			secs.set(s);
	}
}


static void Tags_ShowBeastMarks(Instance &inst)
{
	if (inst.edit.mode != ObjType::sectors)
		inst.Editor_ChangeMode('s');

	Tags_FindBeastMarks(*inst.edit.Selected, inst);

	inst.GoToErrors();
}


//------------------------------------------------------------------------

class UI_Check_Tags : public UI_Check_base
{
public:
	int fresh_tag = 0;

	UI_Check_Tags(bool all_mode, Instance &inst) :
		UI_Check_base(520, 326, all_mode, "Check : Tags", "Tag test results"), inst(inst)
	{ }

	static void action_fresh_tag(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;

		// fresh_tag is set externally
		dialog->inst.level.checks.tagsApplyNewValue(dialog->fresh_tag);

		dialog->want_close = true;
	}

	static void action_show_unmatch_sec(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowUnmatchedSectors(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_unmatch_line(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowUnmatchedLineDefs(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_missing_tag(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowMissingTags(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_beast_marks(Fl_Widget *w, void *data)
	{
		UI_Check_Tags *dialog = (UI_Check_Tags *)data;
		Tags_ShowBeastMarks(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

private:
	Instance &inst;
};


CheckResult ChecksModule::checkTags(int min_severity) const
{
	UI_Check_Tags dialog(min_severity > 0, inst);

	selection_c  sel;
	SString check_buffer;

	for (;;)
	{
		Tags_FindMissingTags(sel, inst);

		if (sel.empty())
			dialog.AddLine("No linedefs missing a needed tag");
		else
		{
			check_buffer = SString::printf("%d linedefs missing a needed tag", sel.count_obj());

			dialog.AddLine(check_buffer, 2, 320,
			                "Show", &UI_Check_Tags::action_show_missing_tag);
		}


		Tags_FindUnmatchedLineDefs(sel, doc, inst.conf);

		if (sel.empty())
			dialog.AddLine("No tagged linedefs w/o a matching sector");
		else
		{
			check_buffer = SString::printf("%d tagged linedefs w/o a matching sector", sel.count_obj());

			dialog.AddLine(check_buffer, 2, 350,
			                "Show", &UI_Check_Tags::action_show_unmatch_line);
		}


		Tags_FindUnmatchedSectors(sel, inst);

		if (sel.empty())
			dialog.AddLine("No tagged sectors w/o a matching linedef");
		else
		{
			check_buffer = SString::printf("%d tagged sectors w/o a matching linedef", sel.count_obj());

			dialog.AddLine(check_buffer, 1, 350,
			                "Show", &UI_Check_Tags::action_show_unmatch_sec);
		}


		Tags_FindBeastMarks(sel, inst);

		if (sel.empty())
			dialog.AddLine("No sectors with tag 666 or 667 used on the wrong map");
		else
		{
			check_buffer = SString::printf("%d sectors have an invalid 666/667 tag", sel.count_obj());

			dialog.AddLine(check_buffer, 1, 350,
			                "Show", &UI_Check_Tags::action_show_beast_marks);
		}

		dialog.AddGap(10);


		int min_tag, max_tag;

		tagsUsedRange(&min_tag, &max_tag);

		if (max_tag <= 0)
			dialog.AddLine("No tags are in use");
		else
		{
			check_buffer = SString::printf("Lowest tag: %d   Highest tag: %d", min_tag, max_tag);
			dialog.AddLine(check_buffer);
		}

		if ((inst.edit.mode == ObjType::linedefs || inst.edit.mode == ObjType::sectors) &&
		    inst.edit.Selected->notempty())
		{
			// always assume sector beastmark tags here
			dialog.fresh_tag = findFreeTag(inst, true);

			dialog.AddGap(10);
			dialog.AddLine("Apply a fresh tag to the selection", 0, 250, "Apply",
			                &UI_Check_Tags::action_fresh_tag);
		}

		if (dialog.WorstSeverity() < min_severity)
			return CheckResult::ok;

		CheckResult result = dialog.Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog.Reset();
			continue;
		}

		return result;
	}
}


//------------------------------------------------------------------------


static void bump_unknown_name(std::map<SString, int>& list,
                              const SString &name)
{
	int count = 0;

	if (list.find(name) != list.end())
		count = list[name];

	list[name] = count + 1;
}


static void Textures_FindMissing(const Instance &inst, selection_c& lines)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_null_tex(inst.level.getRight(*L)->MidTex()))
				lines.set(n);
		}
		else  // Two Sided
		{
			const Sector &front = inst.level.getSector(*inst.level.getRight(*L));
			const Sector &back  = inst.level.getSector(*inst.level.getLeft(*L));

			if (front.floorh < back.floorh && is_null_tex(inst.level.getRight(*L)->LowerTex()))
				lines.set(n);

			if (back.floorh < front.floorh && is_null_tex(inst.level.getLeft(*L)->LowerTex()))
				lines.set(n);

			// missing uppers are OK when between two sky ceilings
			if (inst.is_sky(front.CeilTex()) && inst.is_sky(back.CeilTex()))
				continue;

			if (front.ceilh > back.ceilh && is_null_tex(inst.level.getRight(*L)->UpperTex()))
				lines.set(n);

			if (back.ceilh > front.ceilh && is_null_tex(inst.level.getLeft(*L)->UpperTex()))
				lines.set(n);
		}
	}
}


static void Textures_ShowMissing(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	Textures_FindMissing(inst, *inst.edit.Selected);

	inst.GoToErrors();
}


static void Textures_FixMissing(Instance &inst)
{
	StringID new_wall = BA_InternaliseString(inst.conf.default_wall_tex);

	EditOperation op(inst.level.basis);
	op.setMessage("fixed missing textures");

	for (const auto &L : inst.level.linedefs)
	{
		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_null_tex(inst.level.getRight(*L)->MidTex()))
				op.changeSidedef(L->right, SideDef::F_MID_TEX, new_wall);
		}
		else  // Two Sided
		{
			const Sector &front = inst.level.getSector(*inst.level.getRight(*L));
			const Sector &back  = inst.level.getSector(*inst.level.getLeft(*L));

			if (front.floorh < back.floorh && is_null_tex(inst.level.getRight(*L)->LowerTex()))
				op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_wall);

			if (back.floorh < front.floorh && is_null_tex(inst.level.getLeft(*L)->LowerTex()))
				op.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_wall);

			// missing uppers are OK when between two sky ceilings
			if (inst.is_sky(front.CeilTex()) && inst.is_sky(back.CeilTex()))
				continue;

			if (front.ceilh > back.ceilh && is_null_tex(inst.level.getRight(*L)->UpperTex()))
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_wall);

			if (back.ceilh > front.ceilh && is_null_tex(inst.level.getLeft(*L)->UpperTex()))
				op.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_wall);
		}
	}
}


static bool is_transparent(const Instance &inst, const SString &tex)
{
	// ignore lack of texture here
	// [ technically "-" is the poster-child of transparency,
	//   but it is handled by the Missing Texture checks ]
	if (is_null_tex(tex))
		return false;

	const Img_c *img = inst.wad.images.getTexture(inst.conf, tex);
	if (! img)
		return false;

	// note : this is slow
	return img->has_transparent();
}


static int check_transparent(const Instance &inst, const SString &tex,
                             std::map<SString, int>& names)
{
	if (is_transparent(inst, tex))
	{
		bump_unknown_name(names, tex);
		return 1;
	}

	return 0;
}


static void Textures_FindTransparent(Instance &inst, selection_c& lines,
                              std::map<SString, int>& names)
{
	lines.change_type(ObjType::linedefs);

	names.clear();

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (check_transparent(inst, inst.level.getRight(*L)->MidTex(), names))
				lines.set(n);
		}
		else  // Two Sided
		{
			// note : plain OR operator here to check all parts (do NOT want short-circuit)
			if (check_transparent(inst, inst.level.getRight(*L)->LowerTex(), names) |
				check_transparent(inst, inst.level.getRight(*L)->UpperTex(), names) |
				check_transparent(inst, inst.level.getLeft(*L)->LowerTex(), names) |
				check_transparent(inst, inst.level.getLeft(*L)->UpperTex(), names))
			{
				lines.set(n);
			}
		}
	}
}


static void Textures_ShowTransparent(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	std::map<SString, int> names;

	Textures_FindTransparent(inst, *inst.edit.Selected, names);

	inst.GoToErrors();
}


static void Textures_FixTransparent(Instance &inst)
{
	SString new_tex = inst.conf.default_wall_tex;

	// do something reasonable if default wall is transparent
	if (is_transparent(inst, new_tex))
	{
		if (inst.wad.images.W_TextureIsKnown(inst.conf, "SANDSQ2"))
			new_tex = "SANDSQ2";	// Heretic
		else if (inst.wad.images.W_TextureIsKnown(inst.conf, "CASTLE07"))
			new_tex = "CASTLE07";	// Hexen
		else if (inst.wad.images.W_TextureIsKnown(inst.conf, "BRKBRN02"))
			new_tex = "BRKBRN02";	// Strife
		else
			new_tex = "GRAY1";		// Doom
	}

	StringID new_wall = BA_InternaliseString(new_tex);

	EditOperation op(inst.level.basis);
	op.setMessage("fixed transparent textures");

	for (const auto &L : inst.level.linedefs)
	{
		if (L->right < 0)
			continue;

		if (L->OneSided())
		{
			if (is_transparent(inst, inst.level.getRight(*L)->MidTex()))
				op.changeSidedef(L->right, SideDef::F_MID_TEX, new_wall);
		}
		else  // Two Sided
		{
			if (is_transparent(inst, inst.level.getLeft(*L)->LowerTex()))
				op.changeSidedef(L->left, SideDef::F_LOWER_TEX, new_wall);

			if (is_transparent(inst, inst.level.getLeft(*L)->UpperTex()))
				op.changeSidedef(L->left, SideDef::F_UPPER_TEX, new_wall);

			if (is_transparent(inst, inst.level.getRight(*L)->LowerTex()))
				op.changeSidedef(L->right, SideDef::F_LOWER_TEX, new_wall);

			if (is_transparent(inst, inst.level.getRight(*L)->UpperTex()))
				op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_wall);
		}
	}
}


static void Textures_LogTransparent(Instance &inst)
{
	selection_c sel;

	std::map<SString, int> names;
	std::map<SString, int>::iterator IT;

	Textures_FindTransparent(inst, sel, names);

	gLog.printf("\n");
	gLog.printf("Transparent textures on solid walls:\n");
	gLog.printf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		gLog.printf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


static int check_medusa(const WadData &wad, const SString &tex,
                        std::map<SString, int>& names)
{
	if (is_null_tex(tex) || is_special_tex(tex))
		return 0;

	if (! wad.images.W_TextureCausesMedusa(tex))
		return 0;

	bump_unknown_name(names, tex);
	return 1;
}


static void Textures_FindMedusa(selection_c& lines,
                         std::map<SString, int>& names, const Instance &inst)
{
	lines.change_type(ObjType::linedefs);

	names.clear();

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->right < 0 || L->left < 0)
			continue;

		if (check_medusa(inst.wad, inst.level.getRight(*L)->MidTex(), names) |  /* plain OR */
			check_medusa(inst.wad, inst.level.getLeft(*L)->MidTex(), names))
		{
			lines.set(n);
		}
	}
}


static void Textures_ShowMedusa(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	std::map<SString, int> names;

	Textures_FindMedusa(*inst.edit.Selected, names, inst);

	inst.GoToErrors();
}


static void Textures_RemoveMedusa(Instance &inst)
{
	StringID null_tex = BA_InternaliseString("-");

	std::map<SString, int> names;

	EditOperation op(inst.level.basis);
	op.setMessage("fixed medusa textures");

	for (const auto &L : inst.level.linedefs)
	{
		if (L->right < 0 || L->left < 0)
			continue;

		if (check_medusa(inst.wad, inst.level.getRight(*L)->MidTex(), names))
		{
			op.changeSidedef(L->right, SideDef::F_MID_TEX, null_tex);
		}

		if (check_medusa(inst.wad, inst.level.getLeft(*L)->MidTex(), names))
		{
			op.changeSidedef(L->left, SideDef::F_MID_TEX, null_tex);
		}
	}
}


static void Textures_LogMedusa(const Instance &inst)
{
	selection_c sel;

	std::map<SString, int> names;
	std::map<SString, int>::iterator IT;

	Textures_FindMedusa(sel, names, inst);

	gLog.printf("\n");
	gLog.printf("Medusa effect textures:\n");
	gLog.printf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		gLog.printf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


static void Textures_FindTuttiFrutti(selection_c& lines, const Instance& inst)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		if (L->right < 0)
			continue;

		if (L->left < 0)	// single sided
		{
			const Img_c* texture = inst.wad.images.getTexture(inst.conf, inst.level.getRight(*L)->MidTex());
			if (!texture)
				continue;
			if (texture->has_transparent())
			{
				lines.set(n);
				continue;
			}
			if (texture->height() >= 128)
				continue;
			const SideDef* side = inst.level.getSide(*L, Side::right);
			if (!side)
				continue;
			const Sector &sector = inst.level.getSector(*side);
			int headroom = sector.ceilh - sector.floorh;
			if (headroom > texture->height() || 
				(L->flags & MLF_LowerUnpegged && (side->y_offset > 0 || side->y_offset < texture->height() - headroom)) || 
				(!(L->flags & MLF_LowerUnpegged) && (side->y_offset < 0 || side->y_offset > texture->height() - headroom)))
			{
				lines.set(n);
			}
			continue;
		}
		// two sided
		// TODO
	}
}

static void Textures_ShowTuttiFrutti(Instance& inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	Textures_FindTuttiFrutti(*inst.edit.Selected, inst);

	inst.GoToErrors();
}

static void Textures_FindUnknownTex(selection_c& lines,
                             std::map<SString, int>& names, const Instance &inst)
{
	lines.change_type(ObjType::linedefs);

	names.clear();

	for (int n = 0 ; n < inst.level.numLinedefs(); n++)
	{
		const auto L = inst.level.linedefs[n];

		for (int side = 0 ; side < 2 ; side++)
		{
			const SideDef *SD = side ? inst.level.getLeft(*L) : inst.level.getRight(*L);

			if (! SD)
				continue;

			for (int part = 0 ; part < 3 ; part++)
			{
				SString tex = (part == 0) ? SD->LowerTex() :
							  (part == 1) ? SD->UpperTex() : SD->MidTex();

				if (! inst.wad.images.W_TextureIsKnown(inst.conf, tex))
				{
					bump_unknown_name(names, tex);

					lines.set(n);
				}
			}
		}
	}
}


static void Textures_FindUnknownFlat(selection_c& secs,
                              std::map<SString, int>& names, const Instance &inst)
{
	secs.change_type(ObjType::sectors);

	names.clear();

	for (int s = 0 ; s < inst.level.numSectors(); s++)
	{
		const auto S = inst.level.sectors[s];

		for (int part = 0 ; part < 2 ; part++)
		{
			SString flat = part ? S->CeilTex() : S->FloorTex();

			if (! inst.wad.images.W_FlatIsKnown(inst.conf, flat))
			{
				bump_unknown_name(names, flat);

				secs.set(s);
			}
		}
	}
}


static void Textures_ShowUnknownTex(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	std::map<SString, int> names;

	Textures_FindUnknownTex(*inst.edit.Selected, names, inst);

	inst.GoToErrors();
}


static void Textures_ShowUnknownFlat(Instance &inst)
{
	if (inst.edit.mode != ObjType::sectors)
		inst.Editor_ChangeMode('s');

	std::map<SString, int> names;

	Textures_FindUnknownFlat(*inst.edit.Selected, names, inst);

	inst.GoToErrors();
}


static void Textures_LogUnknown(bool do_flat, const Instance &inst)
{
	selection_c sel;

	std::map<SString, int> names;
	std::map<SString, int>::iterator IT;

	if (do_flat)
		Textures_FindUnknownFlat(sel, names, inst);
	else
		Textures_FindUnknownTex(sel, names, inst);

	gLog.printf("\n");
	gLog.printf("Unknown %s:\n", do_flat ? "Flats" : "Textures");
	gLog.printf("{\n");

	for (IT = names.begin() ; IT != names.end() ; IT++)
		gLog.printf("  %-9s x %d\n", IT->first.c_str(), IT->second);

	gLog.printf("}\n");

	LogViewer_Open();
}


static void Textures_FixUnknownTex(Instance &inst)
{
	StringID new_wall = BA_InternaliseString(inst.conf.default_wall_tex);

	StringID null_tex = BA_InternaliseString("-");

	EditOperation op(inst.level.basis);
	op.setMessage("fixed unknown textures");

	for (const auto &L : inst.level.linedefs)
	{
		bool two_sided = L->TwoSided();

		for (int side = 0 ; side < 2 ; side++)
		{
			int sd_num = side ? L->left : L->right;

			if (sd_num < 0)
				continue;

			const auto SD = inst.level.sidedefs[sd_num];

			if (! inst.wad.images.W_TextureIsKnown(inst.conf, SD->LowerTex()))
				op.changeSidedef(sd_num, SideDef::F_LOWER_TEX, new_wall);

			if (!inst.wad.images.W_TextureIsKnown(inst.conf, SD->UpperTex()))
				op.changeSidedef(sd_num, SideDef::F_UPPER_TEX, new_wall);

			if (!inst.wad.images.W_TextureIsKnown(inst.conf, SD->MidTex()))
				op.changeSidedef(sd_num, SideDef::F_MID_TEX, two_sided ? null_tex : new_wall);
		}
	}
}


static void Textures_FixUnknownFlat(Instance &inst)
{
	StringID new_floor = BA_InternaliseString(inst.conf.default_floor_tex);
	StringID new_ceil  = BA_InternaliseString(inst.conf.default_ceil_tex);

	EditOperation op(inst.level.basis);
	op.setMessage("fixed unknown flats");

	for (int s = 0 ; s < inst.level.numSectors(); s++)
	{
		const auto S = inst.level.sectors[s];

		if (! inst.wad.images.W_FlatIsKnown(inst.conf, S->FloorTex()))
			op.changeSector(s, Sector::F_FLOOR_TEX, new_floor);

		if (!inst.wad.images.W_FlatIsKnown(inst.conf, S->CeilTex()))
			op.changeSector(s, Sector::F_CEIL_TEX, new_ceil);
	}
}


static bool is_switch_tex(const SString &tex)
{
	// we only check if the name begins with "SW" and a digit or
	// an underscore.  that is sufficient for DOOM and Heretic, and
	// most Hexen switches, but misses a lot in Strife.

	return (tex[0] == 'S') && (tex[1] == 'W') &&
			(tex[2] == '_' || isdigit(tex[2]));
}


static void Textures_FindDupSwitches(selection_c& lines, const Document &doc)
{
	lines.change_type(ObjType::linedefs);

	for (int n = 0 ; n < doc.numLinedefs(); n++)
	{
		const auto L = doc.linedefs[n];

		// only check lines with a special
		if (! L->type)
			continue;

		if (L->right < 0)
			continue;

		// switch textures only work on the front side
		// (no need to look at the back side)

		bool lower = is_switch_tex(doc.getRight(*L)->LowerTex());
		bool upper = is_switch_tex(doc.getRight(*L)->UpperTex());
		bool mid   = is_switch_tex(doc.getRight(*L)->MidTex());

		int count = (lower ? 1:0) + (upper ? 1:0) + (mid ? 1:0);

		if (count > 1)
			lines.set(n);
	}
}


static void Textures_ShowDupSwitches(Instance &inst)
{
	if (inst.edit.mode != ObjType::linedefs)
		inst.Editor_ChangeMode('l');

	Textures_FindDupSwitches(*inst.edit.Selected, inst.level);

	inst.GoToErrors();
}


static void Textures_FixDupSwitches(Instance &inst)
{
	StringID null_tex = BA_InternaliseString("-");

	SString new_tex = inst.conf.default_wall_tex;

	// do something reasonable if default wall is a switch
	if (is_switch_tex(new_tex))
	{
		if (inst.wad.images.W_TextureIsKnown(inst.conf, "SANDSQ2"))
			new_tex = "SANDSQ2";	// Heretic
		else if (inst.wad.images.W_TextureIsKnown(inst.conf, "CASTLE07"))
			new_tex = "CASTLE07";	// Hexen
		else if (inst.wad.images.W_TextureIsKnown(inst.conf, "BRKBRN02"))
			new_tex = "BRKBRN02";	// Strife
		else
			new_tex = "GRAY1";		// Doom
	}

	StringID new_wall = BA_InternaliseString(new_tex);

	EditOperation op(inst.level.basis);
	op.setMessage("fixed non-animating switches");

	for (const auto &L : inst.level.linedefs)
	{
		// only check lines with a special
		if (! L->type)
			continue;

		if (L->right < 0)
			continue;

		// switch textures only work on the front side
		// (hence no need to look at the back side)

		bool lower = is_switch_tex(inst.level.getRight(*L)->LowerTex());
		bool upper = is_switch_tex(inst.level.getRight(*L)->UpperTex());
		bool mid   = is_switch_tex(inst.level.getRight(*L)->MidTex());

		int count = (lower ? 1:0) + (upper ? 1:0) + (mid ? 1:0);

		if (count < 2)
			continue;

		if (L->OneSided())
		{
			// we don't care if "mid" is not a switch
			op.changeSidedef(L->right, SideDef::F_LOWER_TEX, null_tex);
			op.changeSidedef(L->right, SideDef::F_UPPER_TEX, null_tex);
			continue;
		}

		const Sector &front = inst.level.getSector(*inst.level.getRight(*L));
		const Sector &back  = inst.level.getSector(*inst.level.getLeft(*L));

		bool lower_vis = (front.floorh < back.floorh);
		bool upper_vis = (front.ceilh > back.ceilh);

		if (count >= 2 && upper && !upper_vis)
		{
			op.changeSidedef(L->right, SideDef::F_UPPER_TEX, null_tex);
			upper = false;
			count--;
		}

		if (count >= 2 && lower && !lower_vis)
		{
			op.changeSidedef(L->right, SideDef::F_LOWER_TEX, null_tex);
			lower = false;
			count--;
		}

		if (count >= 2 && mid)
		{
			op.changeSidedef(L->right, SideDef::F_MID_TEX, null_tex);
			mid = false;
			count--;
		}

		if (count >= 2)
		{
			op.changeSidedef(L->right, SideDef::F_UPPER_TEX, new_wall);
			upper = false;
			count--;
		}
	}
}


//------------------------------------------------------------------------

class UI_Check_Textures : public UI_Check_base
{
public:
	UI_Check_Textures(bool all_mode, Instance &inst) :
		UI_Check_base(580, 386, all_mode, "Check : Textures",
		              "Texture test results"), inst(inst)
	{ }

	static void action_show_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowUnknownTex(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_log_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogUnknown(false, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_unk_tex(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixUnknownTex(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowUnknownFlat(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_log_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogUnknown(true, dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_unk_flat(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixUnknownFlat(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_missing(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowMissing(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_missing(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixMissing(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowTransparent(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixTransparent(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_log_transparent(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogTransparent(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}


	static void action_show_dup_switch(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowDupSwitches(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_fix_dup_switch(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_FixDupSwitches(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}


	static void action_show_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_ShowMedusa(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_show_tuttifrutti(Fl_Widget* w, void* data)
	{
		auto dialog = static_cast<UI_Check_Textures*>(data);
		Textures_ShowTuttiFrutti(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

	static void action_remove_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_RemoveMedusa(dialog->inst);
		dialog->user_action = CheckResult::tookAction;
	}

	static void action_log_medusa(Fl_Widget *w, void *data)
	{
		UI_Check_Textures *dialog = (UI_Check_Textures *)data;
		Textures_LogMedusa(dialog->inst);
		dialog->user_action = CheckResult::highlight;
	}

private:
	Instance &inst;
};


CheckResult ChecksModule::checkTextures(int min_severity) const
{
	UI_Check_Textures *dialog = new UI_Check_Textures(min_severity > 0, inst);

	selection_c  sel;

	std::map<SString, int> names;
	SString check_buffer;

	for (;;)
	{
		Textures_FindUnknownTex(sel, names, inst);

		if (sel.empty())
			dialog->AddLine("No unknown textures");
		else
		{
			check_buffer = SString::printf("%d unknown textures", (int)names.size());

			dialog->AddLine(check_buffer, 2, 200,
			                "Show", &UI_Check_Textures::action_show_unk_tex,
			                "Log",  &UI_Check_Textures::action_log_unk_tex,
			                "Fix",  &UI_Check_Textures::action_fix_unk_tex);
		}


		Textures_FindUnknownFlat(sel, names, inst);

		if (sel.empty())
			dialog->AddLine("No unknown flats");
		else
		{
			check_buffer = SString::printf("%d unknown flats", (int)names.size());

			dialog->AddLine(check_buffer, 2, 200,
			                "Show", &UI_Check_Textures::action_show_unk_flat,
			                "Log",  &UI_Check_Textures::action_log_unk_flat,
			                "Fix",  &UI_Check_Textures::action_fix_unk_flat);
		}


		if (! inst.conf.features.medusa_fixed)
		{
			Textures_FindMedusa(sel, names, inst);

			if (sel.empty())
				dialog->AddLine("No textures causing Medusa Effect");
			else
			{
				check_buffer = SString::printf("%d Medusa textures", (int)names.size());

				dialog->AddLine(check_buffer, 2, 200,
								"Show", &UI_Check_Textures::action_show_medusa,
								"Log",  &UI_Check_Textures::action_log_medusa,
								"Fix",  &UI_Check_Textures::action_remove_medusa);
			}
		}

		if (!inst.conf.features.tuttifrutti_fixed)
		{
			Textures_FindTuttiFrutti(sel, inst);
			if (sel.empty())
				dialog->AddLine("No tutti-frutti walls");
			else
			{
				check_buffer = SString::printf("%d tutti-frutti walls", sel.count_obj());
				dialog->AddLine(check_buffer, 2, 200, "Show", &UI_Check_Textures::action_show_tuttifrutti);
			}
		}

		dialog->AddGap(10);


		Textures_FindMissing(inst, sel);

		if (sel.empty())
			dialog->AddLine("No missing textures on walls");
		else
		{
			check_buffer = SString::printf("%d missing textures on walls", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 275,
			                "Show", &UI_Check_Textures::action_show_missing,
			                "Fix",  &UI_Check_Textures::action_fix_missing);
		}


		Textures_FindTransparent(inst, sel, names);

		if (sel.empty())
			dialog->AddLine("No transparent textures on solids");
		else
		{
			check_buffer = SString::printf("%d transparent textures on solids", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 275,
			                "Show", &UI_Check_Textures::action_show_transparent,
			                "Fix",  &UI_Check_Textures::action_fix_transparent,
			                "Log",  &UI_Check_Textures::action_log_transparent);
		}


		Textures_FindDupSwitches(sel, doc);

		if (sel.empty())
			dialog->AddLine("No non-animating switch textures");
		else
		{
			check_buffer = SString::printf("%d non-animating switch textures", sel.count_obj());

			dialog->AddLine(check_buffer, 1, 275,
			                "Show", &UI_Check_Textures::action_show_dup_switch,
			                "Fix",  &UI_Check_Textures::action_fix_dup_switch);
		}


		if (dialog->WorstSeverity() < min_severity)
		{
			delete dialog;

			return CheckResult::ok;
		}

		CheckResult result = dialog->Run();

		if (result == CheckResult::tookAction)
		{
			// repeat the tests
			dialog->Reset();
			continue;
		}

		delete dialog;

		return result;
	}
}


//------------------------------------------------------------------------


void ChecksModule::checkAll(bool major_stuff) const
{
	bool no_worries = true;

	int min_severity = major_stuff ? 2 : 1;

	CheckResult result;


	result = checkVertices(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;

	result = checkSectors(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;

	result = checkLinedefs(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;

	result = checkThings(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;

	result = checkTextures(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;

	result = checkTags(min_severity);
	if (result == CheckResult::highlight) return;
	if (result != CheckResult::ok) no_worries = false;


	if (no_worries)
	{
		DLG_Notify(major_stuff ? "No major problems." :
		                         "All tests were successful.");
	}
}


void Instance::CMD_MapCheck()
{
	SString what = EXEC_Param[0];

	if (what.empty())
	{
		Beep("MapCheck: missing keyword");
		return;
	}
	else if (what.noCaseEqual("all"))
	{
		level.checks.checkAll(false);
	}
	else if (what.noCaseEqual("major"))
	{
		level.checks.checkAll(true);
	}
	else if (what.noCaseEqual("vertices"))
	{
		level.checks.checkVertices(0);
	}
	else if (what.noCaseEqual("sectors"))
	{
		level.checks.checkSectors(0);
	}
	else if (what.noCaseEqual("linedefs"))
	{
		level.checks.checkLinedefs(0);
	}
	else if (what.noCaseEqual("things"))
	{
		level.checks.checkThings(0);
	}
	else if (what.noCaseEqual("current"))  // current editing mode
	{
		switch (edit.mode)
		{
			case ObjType::vertices:
				level.checks.checkVertices(0);
				break;

			case ObjType::sectors:
				level.checks.checkSectors(0);
				break;

			case ObjType::linedefs:
				level.checks.checkLinedefs(0);
				break;

			case ObjType::things:
				level.checks.checkThings(0);
				break;

			default:
				Beep("Nothing to check");
				break;
		}
	}
	else if (what.noCaseEqual("textures"))
	{
		level.checks.checkTextures(0);
	}
	else if (what.noCaseEqual("tags"))
	{
		level.checks.checkTags(0);
	}
	else
	{
		Beep("MapCheck: unknown keyword: %s\n", what.c_str());
	}
}


void Debug_CheckUnusedStuff(Document &doc)
{
	selection_c sel;

	Sectors_FindUnused(sel, doc);

	int num = sel.count_obj();

	if (num > 0)
	{
		fl_beep();
		DLG_Notify("Operation left %d sectors unused.", num);

		Sectors_RemoveUnused(doc);
		return;
	}

	SideDefs_FindUnused(sel, doc);

	num = sel.count_obj();

	if (num > 0)
	{
		fl_beep();
		DLG_Notify("Operation left %d sidedefs unused.", num);

		SideDefs_RemoveUnused(doc);
		return;
	}
}

//
// Finds a free tag. Doesn't care about limits at this point
//
int findFreeTag(const Instance &inst, bool forsector)
{
	std::unordered_set<int> tags;
	int freetag = 0;

	const auto &doc = inst.level;

	auto addtag = [&tags, &freetag](int tag)
	{
		tags.insert(tag);
		if(tag == freetag)
			++freetag;	// raise if if we know it's there
	};

	for(int i = 0; i < doc.numLinedefs(); ++i)
		addtag(doc.linedefs[i]->tag);
	for(int i = 0; i < doc.numSectors(); ++i)
		addtag(doc.sectors[i]->tag);

	while(tags.count(freetag) || (forsector &&
								  inst.conf.features.tag_666 != Tag666Rules::disabled &&
								  (freetag == 666 || freetag == 667)))
	{
		++freetag;	// now raise it as necessary
	}
	return freetag;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
