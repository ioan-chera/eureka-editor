//------------------------------------------------------------------------
//  MAIN WINDOW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2016 Andrew Apted
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

#include "e_main.h"
#include "m_config.h"
#include "m_testmap.h"
#include "r_render.h"
#include "ui_window.h"
#include "w_wad.h"

#ifndef WIN32
#include <unistd.h>
#endif

#if (FL_MAJOR_VERSION != 1 || FL_MINOR_VERSION < 3)
#error "Require FLTK version 1.3.0 or later"
#endif



#define WINDOW_MIN_W  928
#define WINDOW_MIN_H  640

//
// MainWin Constructor
//
UI_MainWindow::UI_MainWindow(Instance &inst) :
	Fl_Double_Window(WINDOW_MIN_W, WINDOW_MIN_H, EUREKA_TITLE),
	cursor_shape(FL_CURSOR_DEFAULT), mInstance(inst)
{
	end(); // cancel begin() in Fl_Group constructor

	size_range(WINDOW_MIN_W, WINDOW_MIN_H);

	callback((Fl_Callback *) quit_callback);

	color(WINDOW_BG, WINDOW_BG);

	int cy = 0;
	int ey = h();

	panel_W = 308;

	/* ---- Menu bar ---- */
	{
		menu_bar = menu::create(0, 0, w()-3 - panel_W, 31, &mInstance);
		add(menu_bar);
		testmap::updateMenuName(menu_bar, inst.loaded);

#ifndef __APPLE__
		cy += menu_bar->h();
#endif
	}


	info_bar = new UI_InfoBar(mInstance, 0, ey - 31, w(), 31);
	add(info_bar);

	ey = ey - info_bar->h();


	int browser_W = MIN_BROWSER_W + 66;

	int cw = w() - panel_W - browser_W;
	int ch = ey - cy;

	scroll = new UI_CanvasScroll(mInstance, 0, cy, cw, ch);

	// UI_CanvasScroll creates these, we mirror them for easier access
	canvas = scroll->canvas;
	status_bar = scroll->status;

	browser = new UI_Browser(inst, w() - panel_W - browser_W, cy, browser_W, ey - cy);

	tile = new UI_Tile(0, cy, w() - panel_W, ey - cy, NULL, scroll, browser);
	add(tile);

	resizable(tile);


	int BY = 0;     // cy+2
	int BH = ey-4;  // ey-BY-2

	thing_box = new UI_ThingBox(inst, w() - panel_W, BY, panel_W, BH);
	thing_box->hide();
	add(thing_box);

	line_box = new UI_LineBox(inst, w() - panel_W, BY, panel_W, BH);
	line_box->hide();
	add(line_box);

	sec_box = new UI_SectorBox(inst, w() - panel_W, BY, panel_W, BH);
	sec_box->hide();
	add(sec_box);

	vert_box = new UI_VertexBox(inst, w() - panel_W, BY, panel_W, BH);
	add(vert_box);

	props_box = new UI_DefaultProps(inst, w() - panel_W, BY, panel_W, BH);
	props_box->hide();
	add(props_box);

	find_box = new UI_FindAndReplace(inst, w() - panel_W, BY, panel_W, BH);
	find_box->hide();
	add(find_box);
	
	mapItemBoxes[0] = thing_box;
	mapItemBoxes[1] = line_box;
	mapItemBoxes[2] = sec_box;
	mapItemBoxes[3] = vert_box;
}


//
// MainWin Destructor
//
UI_MainWindow::~UI_MainWindow()
{ }


void UI_MainWindow::quit_callback(Fl_Widget *w, void *data)
{
	Main_Quit();
}


void UI_MainWindow::NewEditMode(ObjType mode)
{
	UnselectPics();

	thing_box->hide();
	 line_box->hide();
	  sec_box->hide();
	 vert_box->hide();
	props_box->hide();
	 find_box->hide();

	switch (mode)
	{
		case ObjType::things:  thing_box->show(); break;
		case ObjType::linedefs: line_box->show(); break;
		case ObjType::sectors:   sec_box->show(); break;
		case ObjType::vertices: vert_box->show(); break;

		default: break;
	}

	info_bar->NewEditMode(mode);
	browser ->NewEditMode(mode);

	redraw();
}


void UI_MainWindow::SetCursor(Fl_Cursor shape) noexcept
{
	if (shape == cursor_shape)
		return;

	cursor_shape = shape;

	cursor(shape);
}


void UI_MainWindow::BrowserMode(::BrowserMode kind)
{
	bool is_visible = browser->visible() ? true : false;

	if (is_visible && kind == ::BrowserMode::toggle)
		kind = ::BrowserMode::hide;

	bool want_visible = (kind != ::BrowserMode::hide) ? true : false;

	if (is_visible != want_visible)
	{
		if (want_visible)
			tile->ShowRight();
		else
			tile->HideRight();

//??	// hiding the browser also clears any pic selection
//??	if (! want_visible)
//??		UnselectPics();
	}

	if (kind != ::BrowserMode::hide && kind != ::BrowserMode::toggle)
	{
		browser->ChangeMode(kind);
	}
}


void UI_MainWindow::HideSpecialPanel()
{
	props_box->hide();
	 find_box->hide();

	switch (mInstance.edit.mode)
	{
		case ObjType::things:   thing_box->show(); break;
		case ObjType::linedefs:  line_box->show(); break;
		case ObjType::vertices:  vert_box->show(); break;
		case ObjType::sectors:    sec_box->show(); break;

		default: break;
	}

	redraw();
}


void UI_MainWindow::ShowDefaultProps()
{
	// already shown?
	if (props_box->visible())
	{
		HideSpecialPanel();
		return;
	}

	thing_box->hide();
	 line_box->hide();
	  sec_box->hide();
	 vert_box->hide();
	 find_box->hide();

	props_box->show();

	redraw();
}


void UI_MainWindow::ShowFindAndReplace()
{
	// already shown?
	if (find_box->visible())
	{
		HideSpecialPanel();
		return;
	}

	thing_box->hide();
	 line_box->hide();
	  sec_box->hide();
	 vert_box->hide();
	props_box->hide();

	 find_box->Open();

	redraw();
}


void UI_MainWindow::UpdateTotals(const Document &doc) noexcept
{
	for(MapItemBox *box : mapItemBoxes)
		box->UpdateTotal(doc);
}


int UI_MainWindow::GetPanelObjNum() const
{
	// FIXME: using 'inst.edit' here feels like a hack or mis-design
	switch (mInstance.edit.mode)
	{
		case ObjType::things:   return thing_box->GetObj();
		case ObjType::vertices: return  vert_box->GetObj();
		case ObjType::sectors:  return   sec_box->GetObj();
		case ObjType::linedefs: return  line_box->GetObj();

		default:
			return -1;
	}
}

void UI_MainWindow::InvalidatePanelObj()
{
	for(MapItemBox *box : mapItemBoxes)
		if(box->visible())
			box->SetObj(-1, 0);
}

void UI_MainWindow::UpdatePanelObj()
{
	if (thing_box->visible())
		thing_box->UpdateField();

	if (line_box->visible())
	{
		line_box->UpdateField();
		line_box->UpdateSides();
	}

	if (sec_box->visible())
		sec_box->UpdateField();

	if (vert_box->visible())
		vert_box->UpdateField();
}


void UI_MainWindow::UnselectPics()
{
	for(MapItemBox *box : mapItemBoxes)
		box->UnselectPics();
}


void UI_MainWindow::SetTitle(const SString &wad_namem, const SString &map_name,
						  bool read_only)
{
	static char title_buf[FL_PATH_MAX];

	SString wad_name(wad_namem);
	if (wad_name.good())
	{
		wad_name = fl_filename_name(wad_name.c_str());
		snprintf(title_buf, sizeof(title_buf), "%s (%s)%s",
				 wad_name.c_str(), map_name.c_str(), read_only ? " [Read-Only]" : "");
	}
	else
	{
		snprintf(title_buf, sizeof(title_buf), "Unsaved %s", map_name.c_str());
	}

	copy_label(title_buf);
}


void UI_MainWindow::UpdateTitle(char want_prefix)
{
	if (! label())
		return;

	char got_prefix = label()[0];

	if (! ispunct(got_prefix))
		got_prefix = 0;

	if (got_prefix == want_prefix)
		return;


	SString title_buf;

	const char *src  = label() + (got_prefix ? 1 : 0);

	if (want_prefix)
	{
		title_buf.push_back(want_prefix);
	}
	title_buf.insert(title_buf.length(), src);

	copy_label(title_buf.c_str());
}


/* DISABLED, since it fails miserably on every platform

void UI_MainWindow::ToggleFullscreen()
{
	if (last_w)
	{
		fullscreen_off(last_x, last_y, last_w, last_h);

		last_w = last_h = 0;
	}
	else
	{
		last_x = x(); last_y = y();
		last_w = w(); last_h = h();

		fullscreen();
	}
}
*/

//
// see if one of the panels wants to perform a clipboard op,
// because a texture is highlighted or selected (for example).
//
bool UI_MainWindow::ClipboardOp(EditCommand op)
{
	if(props_box->visible())
		return props_box->ClipboardOp(op);
	if(find_box->visible())
		return find_box->ClipboardOp(op);
	if(line_box->visible())
		return line_box->ClipboardOp(op);
	if(sec_box->visible())
		return sec_box->ClipboardOp(op);
	if(thing_box->visible())
		return thing_box->ClipboardOp(op);

	// no panel wanted it
	return false;
}


void UI_MainWindow::BrowsedItem(::BrowserMode kind, int number, const char *name, int e_state)
{
//	fprintf(stderr, "BrowsedItem: kind '%c' --> %d / \"%s\"\n", kind, number, name);

	if (props_box->visible())
	{
		props_box->BrowsedItem(kind, number, name, e_state);
	}
	else if (find_box->visible())
	{
		find_box->BrowsedItem(kind, number, name, e_state);
	}
	else if (line_box->visible())
	{
		line_box->BrowsedItem(kind, number, name, e_state);
	}
	else if (sec_box->visible())
	{
		sec_box->BrowsedItem(kind, number, name, e_state);
	}
	else if (thing_box->visible())
	{
		thing_box->BrowsedItem(kind, number, name, e_state);
	}
	else
	{
		mInstance.Beep("no target for browsed item");
	}
}


void UI_MainWindow::Maximize()
{
#if defined(WIN32)
	HWND hWnd = fl_xid(this);

	ShowWindow(hWnd, SW_MAXIMIZE);

#elif defined(__APPLE__)
	// FIXME : something like this... (AFAIK)
	//
	// Window handle = fl_xid(this);   // really an NSWindow *
	//
	// if (! [handle isZoomed])
	//     [handle zoom:nil];

#else  /* Linux + X11 */

	Window xid = fl_xid(this);

	Atom wm_state = XInternAtom(fl_display, "_NET_WM_STATE", False);
	Atom vmax = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	Atom hmax = XInternAtom(fl_display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

	XEvent xev;
	memset(&xev, 0, sizeof(xev));

	xev.type = ClientMessage;
	xev.xclient.window = xid;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 2;
	xev.xclient.data.l[1] = hmax;
	xev.xclient.data.l[2] = vmax;

	XSendEvent(fl_display, DefaultRootWindow(fl_display), False,
			   SubstructureNotifyMask, &xev);

	Delay(3);
#endif
}


void UI_MainWindow::Delay(int steps)
{
	for (; steps > 0 ; steps--)
	{
		TimeDelay(100);

		Fl::check();
	}
}


void UI_MainWindow::UpdateGameInfo(const LoadingData &loading, const ConfigData &config)
{
	for(MapItemBox *box : mapItemBoxes)
		box->UpdateGameInfo(loading, config);
}


// draw() and handle() are overridden here merely to prevent fatal
// errors becoming infinite loops.  That can happen because we use
// FLTK to show a dialog with the error message, and the same code
// which triggered the fatal error can be re-entered.

int UI_MainWindow::handle(int event)
{
	if (global::in_fatal_error)
		return 0;

	return Fl_Double_Window::handle(event);
}

void UI_MainWindow::draw()
{
	if (global::in_fatal_error)
		return;

	return Fl_Double_Window::draw();
}


//------------------------------------------------------------------------

int UI_Escapable_Window::handle(int event)
{
	if (event == FL_KEYDOWN && Fl::event_key() == FL_Escape)
	{
		do_callback();
		return 1;
	}

	return Fl_Double_Window::handle(event);
}


//------------------------------------------------------------------------

#define MAX_LOG_LINES  1000


UI_LogViewer * log_viewer;


UI_LogViewer::UI_LogViewer(Instance &inst) :
	UI_Escapable_Window(580, 400, "Eureka Log Viewer"), inst(inst)
{
	box(FL_NO_BOX);

	size_range(480, 200);

	int ey = h() - 65;

	browser = new Fl_Multi_Browser(0, 0, w(), ey);
	browser->textsize(16);
	browser->callback(select_callback, this);

	resizable(browser);

	{
		Fl_Group *o = new Fl_Group(0, ey, w(), h() - ey);
		o->box(FL_FLAT_BOX);

		if (config::gui_color_set == 2)
			o->color(fl_gray_ramp(4));
		else
			o->color(WINDOW_BG);

		int bx  = w() - 110;
		int bx2 = bx;
		{
			Fl_Button * but = new Fl_Button(bx, ey + 15, 80, 35, "Close");
			but->labelfont(1);
			but->callback(ok_callback, this);
		}

		bx = 30;
		{
			Fl_Button * but = new Fl_Button(bx, ey + 15, 80, 35, "Save...");
			but->callback(save_callback, this);
		}

		bx += 105;
		{
			Fl_Button * but = new Fl_Button(bx, ey + 15, 80, 35, "Clear");
			but->callback(clear_callback, this);
		}

		bx += 105;
		{
			copy_but = new Fl_Button(bx, ey + 15, 80, 35, "Copy");
			copy_but->callback(copy_callback, this);
			copy_but->shortcut(FL_CTRL + 'c');
			copy_but->deactivate();
		}

		bx += 80;

		Fl_Group *resize_box = new Fl_Group(bx + 10, ey + 2, bx2 - bx - 20, h() - ey - 4);
		resize_box->box(FL_NO_BOX);

		o->resizable(resize_box);

		o->end();
	}

	end();
}


UI_LogViewer::~UI_LogViewer()
{ }


void UI_LogViewer::Deselect()
{
	browser->deselect();

	copy_but->deactivate();
}


void UI_LogViewer::JumpEnd()
{
	if (browser->size() > 0)
	{
		browser->bottomline(browser->size());
	}
}


int UI_LogViewer::CountSelectedLines() const
{
	int count = 0;

	for (int i = 1 ; i <= browser->size() ; i++)
		if (browser->selected(i))
			count++;

	return count;
}


SString UI_LogViewer::GetSelectedText() const
{
	SString buf;

	for (int i = 1 ; i <= browser->size() ; i++)
	{
		if (! browser->selected(i))
			continue;

		const char *line_text = browser->text(i);
		if (! line_text)
			continue;

		// append current line onto previous ones

		SString new_buf = buf + line_text;

		if (!new_buf.empty() && new_buf.back() != '\n')
			new_buf.push_back('\n');

		buf = new_buf;
	}

	return buf;
}


void UI_LogViewer::Add(const char *line)
{
	browser->add(line);

	if (browser->size() > MAX_LOG_LINES)
		browser->remove(1);

	if (shown())
		browser->bottomline(browser->size());
}


void LogViewer_AddLine(const char *str)
{
	if (log_viewer)
		log_viewer->Add(str);
}


void UI_LogViewer::ok_callback(Fl_Widget *w, void *data)
{
	UI_LogViewer *that = (UI_LogViewer *)data;

	that->do_callback();
}


void UI_LogViewer::clear_callback(Fl_Widget *w, void *data)
{
	UI_LogViewer *that = (UI_LogViewer *)data;

	that->browser->clear();
	that->copy_but->deactivate();

	that->Add("");
}


void UI_LogViewer::select_callback(Fl_Widget *w, void *data)
{
	UI_LogViewer *that = (UI_LogViewer *)data;

	// require 2 or more lines to activate Copy button
	if (that->CountSelectedLines() >= 2)
		that->copy_but->activate();
	else
		that->copy_but->deactivate();
}


void UI_LogViewer::copy_callback(Fl_Widget *w, void *data)
{
	UI_LogViewer *that = (UI_LogViewer *)data;

	SString text = that->GetSelectedText();

	if (!text.empty())
	{
		Fl::copy(text.c_str(), (int)text.length(), 1);
	}
}


void UI_LogViewer::save_callback(Fl_Widget *w, void *data)
{
	Fl_Native_File_Chooser chooser;
	auto viewer = static_cast<UI_LogViewer*>(data);

	chooser.title("Pick file to save to");
	chooser.type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
	chooser.filter("Text files\t*.txt");
	chooser.directory(viewer->inst.Main_FileOpFolder().u8string().c_str());

	switch (chooser.show())
	{
		case -1:
			DLG_Notify("Unable to save the log file:\n\n%s",
					   chooser.errmsg());
			return;

		case 1:
			// cancelled
			return;

		default:
			break;  // OK
	}


	// add an extension if missing
	SString filename = chooser.filename();

	if(!HasExtension(filename.get()))
		filename += ".txt";

	// TODO: #55
    std::ofstream os(filename.c_str(), std::ios::trunc);

	if (! os.is_open())
	{
		filename = GetErrorMessage(errno);

		DLG_Notify("Unable to save the log file:\n\n%s", filename.c_str());
		return;
	}

	gLog.saveTo(os);
}


void LogViewer_Open()
{
	if (! log_viewer)
		return;

	// always call show() -- to raise the window
	log_viewer->show();
	log_viewer->Deselect();

	log_viewer->JumpEnd();
}

//
// Executes Fl::Focus if safe to do so. ONLY NEEDED to be called during window setup.
//
// Under the ratpoison Linux desktop environment it has been seen to block drawing of other
// controls.
//
void FLFocusOnCreation(Fl_Widget *widget)
{
#if defined(_WIN32) || defined(__APPLE__)
	Fl::focus(widget);
#endif
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
