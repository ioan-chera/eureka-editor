//------------------------------------------------------------------------
//  EVENT HANDLING
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

#include "m_events.h"

#include "e_main.h"
#include "e_hover.h"
#include "Errors.h"
#include "Instance.h"
#include "m_parse.h"
#include "m_streams.h"
#include "main.h"
#include "r_render.h"
#include "ui_window.h"
#include "ui_misc.h"


void Instance::ClearStickyMod()
{
	if (edit.sticky_mod)
		Status_Clear();

	edit.sticky_mod = 0;
}

void Instance::Editor_ClearAction() noexcept
{
	switch (edit.action)
	{
		case EditorAction::nothing:
			return;

		case EditorAction::adjustOfs:
			main_win->SetCursor(FL_CURSOR_DEFAULT);
			break;

		default:
			/* no special for the rest */
			break;
	}

	edit.action = EditorAction::nothing;
}


void Instance::Editor_SetAction(EditorAction  new_action)
{
	Editor_ClearAction();

	edit.action = new_action;

	switch (edit.action)
	{
		case EditorAction::nothing:
			return;

		case EditorAction::adjustOfs:
			mouse_last_pos.x = Fl::event_x();
			mouse_last_pos.y = Fl::event_y();

			main_win->SetCursor(FL_CURSOR_HAND);
			break;

		default:
			/* no special for the rest */
			break;
	}
}


void Instance::Editor_Zoom(int delta, v2int_t mid)
{
    float S1 = static_cast<float>(grid.getScale());

    grid.AdjustScale(delta);

    grid.RefocusZoom(v2double_t(mid), S1);
}


// this is only used for mouse scrolling
void Instance::Editor_ScrollMap(int mode, v2int_t dpos, keycode_t mod)
{
	// started?
	if (mode < 0)
	{
		edit.is_panning = true;
		main_win->SetCursor(FL_CURSOR_HAND);
		return;
	}

	// finished?
	if (mode > 0)
	{
		edit.is_panning = false;
		main_win->SetCursor(FL_CURSOR_DEFAULT);
		return;
	}

	if (! edit.panning_lax)
		mod = 0;

	if (!dpos)
		return;

	if (edit.render3d)
	{
		Render3D_ScrollMap(*this, dpos, mod);
	}
	else
	{
		float speed = static_cast<float>(edit.panning_speed / grid.getScale());

		v2double_t delta = {
			((double) -dpos.x * speed),
			((double)  dpos.y * speed)
		};

		grid.Scroll(delta);
	}
}


void Instance::Navigate2D()
{
	float delay_ms = static_cast<float>(Nav_TimeDiff());

	delay_ms = delay_ms / 1000.0f;

	keycode_t mod = 0;

	if (edit.nav.lax)
		mod = M_ReadLaxModifiers();

	float mod_factor = 1.0;
	if (mod & EMOD_SHIFT)   mod_factor = 0.5;
	if (mod & EMOD_COMMAND) mod_factor = 2.0;

	if (edit.nav.left || edit.nav.right ||
		edit.nav.up   || edit.nav.down)
	{
		v2double_t delta = {
			edit.nav.right - edit.nav.left,
			edit.nav.up    - edit.nav.down
		};

		delta *= static_cast<double>(mod_factor) * delay_ms;

		grid.Scroll(delta);
	}

	RedrawMap();
}

void Instance::Nav_Clear()
{
	edit.clearNav();

	memset(nav_actives, 0, sizeof(nav_actives));

	edit.is_navigating = false;
}


void Instance::Nav_Navigate()
{
	if (edit.render3d)
		Render3D_Navigate();
	else
		Navigate2D();
}


bool Instance::Nav_SetKey(keycode_t key, nav_release_func_t func)
{
	// when starting a navigation, grab the current time
	if (! edit.is_navigating)
		Nav_TimeDiff();

	keycode_t lax_mod = 0;

	edit.nav.lax = Exec_HasFlag("/LAX");
	if (edit.nav.lax)
		lax_mod = EMOD_SHIFT | EMOD_COMMAND;

	edit.is_navigating = true;

	int free_slot = -1;

	for (int i = 0 ; i < MAX_NAV_ACTIVE_KEYS ; i++)
	{
		nav_active_key_t& N = nav_actives[i];

		if (! N.key)
		{
			if (free_slot < 0)
				free_slot = i;

			continue;
		}

		// already active?
		if ((N.key | N.lax_mod) == (key | N.lax_mod) && N.release == func)
			return false;

		// if it's the same physical key, release the previous action
		if ((N.key & FL_KEY_MASK) == (key & FL_KEY_MASK))
		{
			(this->*N.release)();
			N.key = 0;
		}
	}

	if (free_slot >= 0)
	{
		nav_active_key_t& N = nav_actives[free_slot];

		N.key     = key;
		N.release = func;
		N.lax_mod = lax_mod;
	}

	return true;
}


bool Instance::Nav_ActionKey(keycode_t key, nav_release_func_t func)
{
	keycode_t lax_mod = 0;

	if (Exec_HasFlag("/LAX"))
		lax_mod = EMOD_SHIFT | EMOD_COMMAND;

	nav_active_key_t& N = cur_action_key;

	if (N.key)
	{
		// already active?
		if ((N.key | N.lax_mod) == (key | N.lax_mod) && N.release == func)
			return false;

		// release the existing action
		(this->*N.release)();
	}

	N.key     = key;
	N.release = func;
	N.lax_mod = lax_mod;

	return true;
}


static inline bool CheckKeyPressed(nav_active_key_t& N)
{
#if 0  // IGNORE MODIFIER CHANGES
	keycode_t mod  = N.key & EMOD_ALL_MASK;

	// grab current modifiers, but simplify to a single one
	keycode_t cur_mod = M_TranslateKey(0, Fl::event_state());

	if ((mod | N.lax_mod) != (cur_mod | N.lax_mod))
		return false;
#endif

	keycode_t base = N.key & FL_KEY_MASK;

	if (is_mouse_button(base))
	{
		if (Fl::event_buttons() & FL_BUTTON(base - FL_Button))
			return true;
	}
	else if (is_mouse_wheel(base))
	{
		return false;
	}
	else  // key on keyboard
	{
		if (Fl::event_key(base))
			return true;
	}

	return false;
}


static void Nav_UpdateActionKey(Instance &inst)
{
	nav_active_key_t& N = inst.cur_action_key;

	if (! N.key)
		return;

	if (! CheckKeyPressed(N))
	{
		(inst.*N.release)();
		 N.key = 0;
	}
}


static void Nav_UpdateKeys(Instance &inst)
{
	// ensure the currently active keys are still pressed
	// [ call this after getting each keyboard event from FLTK ]

	Nav_UpdateActionKey(inst);

	if (! inst.edit.is_navigating)
		return;

	// we rebuilt this value
	inst.edit.is_navigating = false;

	for (int i = 0 ; i < MAX_NAV_ACTIVE_KEYS ; i++)
	{
		nav_active_key_t& N = inst.nav_actives[i];

		if (! N.key)
			continue;

		if (! CheckKeyPressed(N))
		{
			// call release function, clear the slot
			(inst.*N.release)();
			N.key = 0;
			continue;
		}

		// at least one navigation key is still active
		inst.edit.is_navigating = true;
	}
}


// returns number of milliseconds since the previous call
unsigned int Instance::Nav_TimeDiff()
{
	unsigned int old_time = nav_time;

	nav_time = TimeGetMillies();

	// handle overflow
	if (nav_time < old_time)
		return 10;

	unsigned int diff = (nav_time - old_time);

	// clamp large values
	if (diff > 250)
		diff = 250;

	return diff;
}


//------------------------------------------------------------------------
//   EVENT HANDLING
//------------------------------------------------------------------------

void Instance::EV_EnterWindow()
{
	if (!global::app_has_focus)
	{
		edit.pointer_in_window = false;
		return;
	}

	edit.pointer_in_window = true;

	main_win->canvas->PointerPos(true /* in_event */);

	// restore keyboard focus to the canvas
	Fl_Widget * foc = main_win->canvas;

	if (Fl::focus() != foc)
		foc->take_focus();

	RedrawMap();
}


void Instance::EV_LeaveWindow()
{
	// ignore FL_LEAVE event when doing a popup operation menu,
	// otherwise we lose the highlight and kill drawing mode.
	if (in_operation_menu)
		return;

	edit.pointer_in_window = false;

	// this offers a handy way to get out of drawing mode
	if (edit.action == EditorAction::drawLine)
		Editor_ClearAction();

	// this will update (disable) any current highlight
	RedrawMap();
}


void Instance::EV_EscapeKey()
{
	Nav_Clear();
	ClearStickyMod();
	Editor_ClearAction();
	Status_Clear();

	edit.clicked.clear();
	edit.dragged.clear();
	edit.split_line.clear();
	edit.drawLine.from.clear();

	UpdateHighlight();
	RedrawMap();
}


void Instance::EV_MouseMotion(v2int_t pos, keycode_t mod, v2int_t dpos)
{
	// this is probably redundant, as we generally shouldn't get here
	// unless the mouse is in the 2D/3D view (or began a drag there).
	edit.pointer_in_window = true;

	main_win->canvas->PointerPos(true /* in_event */);

//  fprintf(stderr, "MOUSE MOTION: (%d %d)  map: (%1.2f %1.2f)\n", x, y, inst.edit.map_x, inst.edit.map_y);

	if (edit.is_panning)
	{
		Editor_ScrollMap(0, dpos, mod);
		return;
	}

	main_win->info_bar->SetMouse();

	if (edit.action == EditorAction::transform)
	{
		Transform_Update();
		return;
	}

	if (edit.action == EditorAction::drawLine)
	{
		// this calls UpdateHighlight() which updates inst.edit.draw_to_x/y
		RedrawMap();
		return;
	}

	if (edit.action == EditorAction::selbox)
	{
		edit.selbox2 = edit.map.xy;

		main_win->canvas->redraw();
		return;
	}

	if (edit.action == EditorAction::drag)
	{
		edit.drag_screen_dpos = pos - edit.click_screen_pos;

		edit.drag_cur.xy = edit.map.xy;

		// if dragging a single vertex, update the possible split_line.
		// Note: ratio-lock is handled in UI_Canvas::DragDelta
		if (edit.mode == ObjType::vertices && edit.dragged.valid())
			UpdateHighlight();

		main_win->canvas->redraw();
		return;
	}

	// begin dragging?
	if (edit.action == EditorAction::click)
	{
		CheckBeginDrag();
	}

	// in general, just update the highlight, split-line (etc)
	UpdateHighlight();
}


//------------------------------------------------------------------------


keycode_t M_RawKeyForEvent(int event)
{
	// event must be FL_KEYDOWN, FL_PUSH or FL_MOUSEWHEEL

	if (event == FL_PUSH)
	{
		// convert mouse button to a fake key
		return FL_Button + Fl::event_button();
	}

	if (event == FL_MOUSEWHEEL)
	{
		int dx = Fl::event_dx();
		int dy = Fl::event_dy();

		// convert wheel direction to a fake key
		if (abs(dx) > abs(dy))
			return dx < 0 ? FL_WheelLeft : FL_WheelRight;

		return dy < 0 ? FL_WheelUp : FL_WheelDown;
	}

	return Fl::event_key();
}


keycode_t M_CookedKeyForEvent(int event)
{
	int raw_key   = M_RawKeyForEvent(event);
	int raw_state = Fl::event_state();

	return M_TranslateKey(raw_key, raw_state);
}


keycode_t M_ReadLaxModifiers()
{
	// this is a workaround for X-windows, where we don't get new
	// modifier state until the event *after* the shift key is
	// pressed or released.

#if defined(WIN32) || defined(__APPLE__)
	return Fl::event_state() & (EMOD_COMMAND | EMOD_SHIFT);

#else /* Linux and X-Windows */

	// we only need FLTK to read the true keyboard state *once*
	// from the X server, which is what this Fl::get_key() is
	// doing here (with an arbitrary key).

	Fl::get_key('1');

	keycode_t result = 0;

	if (Fl::event_key(FL_Shift_L) || Fl::event_key(FL_Shift_R))
		result |= EMOD_SHIFT;

	if (Fl::event_key(FL_Control_L) || Fl::event_key(FL_Control_R))
		result |= EMOD_COMMAND;

	return result;
#endif
}


int Instance::EV_RawKey(int event)
{
	Nav_UpdateKeys(*this);

	if (event == FL_KEYUP || event == FL_RELEASE)
		return 0;

	int raw_key   = M_RawKeyForEvent(event);
	int raw_state = Fl::event_state();

	int old_sticky_mod = edit.sticky_mod;

	if (edit.sticky_mod)
	{
		raw_state = edit.sticky_mod;
		ClearStickyMod();
	}

	keycode_t key = M_TranslateKey(raw_key, raw_state);

	if (key == 0)
		return 1;

#if 0  // DEBUG
	fprintf(stderr, "Key code: 0x%08x : %s\n", key, M_KeyToString(key).c_str());
#endif

	// keyboard propagation logic

	if (main_win->browser->visible() && ExecuteKey(key, KeyContext::browser))
		return 1;

	if (edit.render3d && ExecuteKey(key, KeyContext::render))
		return 1;

	if (ExecuteKey(key, M_ModeToKeyContext(edit.mode)))
		return 1;

	if (ExecuteKey(key, KeyContext::general))
		return 1;


	// always eat mouse buttons
	if (event == FL_PUSH)
		return 1;


	// NOTE: the key may still get handled by something (e.g. Menus)
	// fprintf(stderr, "Unknown key %d (0x%04x)\n", key, key);


	// prevent a META-fied key from being sent elsewhere, because it
	// won't really be META-fied anywhere else -- including the case
	// of it being sent back to this function as a SHORTCUT event.
	return old_sticky_mod ? 1 : 0;
}


int Instance::EV_RawWheel(int event)
{
	ClearStickyMod();

	// ensure we zoom from correct place
	main_win->canvas->PointerPos(true /* in_event */);

	wheel_dpos.x = Fl::event_dx();
	wheel_dpos.y = Fl::event_dy();

	if (!wheel_dpos)
		return 1;

	EV_RawKey(FL_MOUSEWHEEL);

	return 1;
}


int Instance::EV_RawButton(int event)
{
	ClearStickyMod();

	main_win->canvas->PointerPos(true /* in_event */);

	// Hack Alert : this is required to support pressing two buttons at the
	// same time.  Without this, FLTK does not send us the second button
	// release event, because when the first button is released the "pushed"
	// widget becomes NULL.

	if (Fl::event_buttons() != 0)
		Fl::pushed(main_win->canvas);


	int button = Fl::event_button();

	if (button < 1 || button > 8)
		return 0;

	return EV_RawKey(event);
}


int Instance::EV_RawMouse(int event)
{
	if (!global::app_has_focus)
		return 1;

	int mod = Fl::event_state() & EMOD_ALL_MASK;

	v2int_t dpos = {
		Fl::event_x() - mouse_last_pos.x,
		Fl::event_y() - mouse_last_pos.y
	};

	if (edit.render3d)
	{
		Render3D_MouseMotion({ Fl::event_x(), Fl::event_y() }, mod, dpos);
	}
	else
	{
		EV_MouseMotion({ Fl::event_x(), Fl::event_y() }, mod, dpos);
	}

	mouse_last_pos.x = Fl::event_x();
	mouse_last_pos.y = Fl::event_y();

	return 1;
}


int Instance::EV_HandleEvent(int event)
{
	//// fprintf(stderr, "HANDLE EVENT %d\n", event);

	switch (event)
	{
		case FL_FOCUS:
			return 1;

		case FL_ENTER:
			EV_EnterWindow();
			return 1;

		case FL_LEAVE:
			EV_LeaveWindow();
			return 1;

		case FL_KEYDOWN:
		case FL_KEYUP:
		case FL_SHORTCUT:
			return EV_RawKey(event);

		case FL_PUSH:
		case FL_RELEASE:
			return EV_RawButton(event);

		case FL_MOUSEWHEEL:
			return EV_RawWheel(event);

		case FL_DRAG:
		case FL_MOVE:
			return EV_RawMouse(event);

		default:
			// pass on everything else
			break;
	}

	return 0;
}


//------------------------------------------------------------------------
//   OPERATION MENU(S)
//------------------------------------------------------------------------

struct operation_command_t
{
	const editor_command_t *cmd = nullptr;

	SString param[MAX_EXEC_PARAM];
};


static void ParseOperationLine(const std::vector<SString> &tokens, Fl_Menu_Button *menu)
{
	// just a divider?
	if (tokens[0].noCaseEqual("divider"))
	{
		menu->add("", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE);
		return;
	}

	// parse the key
	int shortcut = 0;

	if (!tokens[0].noCaseEqual("UNBOUND"))
	{
		keycode_t key = M_ParseKeyString(tokens[0]);
		if (key != 0)
			shortcut = M_KeyToShortcut(key);
	}

	// parse the command and its parameters...
	if (tokens.size() < 2)
		ThrowException("operations.cfg: entry missing description.\n");

	if (tokens.size() < 3)
		ThrowException("operations.cfg: entry missing command name.\n");

	const editor_command_t *cmd = FindEditorCommand(tokens[2]);

	if (! cmd)
	{
		gLog.printf("operations.cfg: unknown function: %s\n", tokens[2].c_str());
		return;
	}

	operation_command_t * info = new operation_command_t;

	info->cmd = cmd;

	for (int p = 0 ; p < MAX_EXEC_PARAM ; p++)
		if ((int)tokens.size() >= 4 + p)
			info->param[p] = tokens[3 + p];

	menu->add(tokens[1].c_str(), shortcut, 0 /* callback */, (void *)info, 0 /* flags */);
}


void Instance::M_AddOperationMenu(const SString &context, Fl_Menu_Button *menu)
{
	if (menu->size() < 2)
	{
		ThrowException("operations.cfg: no %s items.\n", context.c_str());
		return;
	}

	// enable the menu

	// this is quite hacky, use button 7 as the trigger button(s) since
	// we don't actually want mouse buttons to directly open it, rather
	// we want to open it explictly by our own code.
	menu->type(0x40);

	// the boxtype MUST be FL_NO_BOX
	menu->box(FL_NO_BOX);

	menu->textsize(16);

	// NOTE: we cannot show() this menu, as that will interfere with
	// mouse motion events [ canvas will get FL_LEAVE when the mouse
	// enters this menu's bbox ]

	menu->hide();

	op_all_menus[context] = menu;

	main_win->add(menu);
}


#define MAX_TOKENS  30

bool Instance::M_ParseOperationFile()
{
	// open the file and build all the menus it contains.

	// look in user's $HOME directory first
	fs::path filename = global::home_dir / "operations.cfg";

	LineFile file(filename);

	// otherwise load it from the installation directory
	if (! file.isOpen())
	{
		filename = global::install_dir / "operations.cfg";

		file.open(filename);
	}

	if (! file.isOpen())
		return false;


	// parse each line

	SString line;
	std::vector<SString> tokens;

	Fl_Menu_Button *menu = NULL;

	SString context;

	while (file.readLine(line))
	{
		int num_tok = M_ParseLine(line, tokens, ParseOptions::haveStrings);
		if (num_tok == 0)
			continue;

		if (num_tok < 0)
		{
			gLog.printf("operations.cfg: failed parsing a line\n");
			continue;
		}

		if (tokens[0].noCaseEqual("menu"))
		{
			if (num_tok < 3)
			{
				gLog.printf("operations.cfg: bad menu line\n");
				continue;
			}

			if (menu != NULL)
				M_AddOperationMenu(context, menu);

			// create new menu
			menu = new Fl_Menu_Button(0, 0, 99, 99, "");
			menu->copy_label(tokens[2].c_str());
			menu->clear();

			context = tokens[1];
			continue;
		}

		if (menu != NULL)
			ParseOperationLine(tokens, menu);
	}

	file.close();

	if (menu != NULL)
		M_AddOperationMenu(context, menu);

	return true;
}


void Instance::M_LoadOperationMenus()
{
	gLog.printf("Loading Operation menus...\n");

	if (! M_ParseOperationFile())
	{
		no_operation_cfg = true;
		DLG_Notify("Installation problem: cannot find \"operations.cfg\" file!");
	}
}


void Instance::CMD_OperationMenu()
{
	if (no_operation_cfg)
		return;

	SString context = EXEC_Param[0];

	// if no context given, pick one based on editing mode
	if (context.empty())
	{
		if (edit.render3d)
		{
			context = "render";
		}
		else
		{
			switch (edit.mode)
			{
			case ObjType::linedefs:	context = "line";   break;
			case ObjType::sectors:	context = "sector"; break;
			case ObjType::vertices:	context = "vertex"; break;
			default: context = "thing";
			}
		}
	}

	Fl_Menu_Button *menu = op_all_menus[context];

	if (menu == NULL)
	{
		Beep("no such menu: %s", context.c_str());
		return;
	}

	SYS_ASSERT(menu);

	// forget the last chosen command in this menu, otherwise FLTK
	// positions the menu to point at that item, which can be annoying
	// especially if the last command was destructive.
	menu->value((const Fl_Menu_Item *)NULL);

	// the pop-up menu will cause an FL_LEAVE event on the canvas,
	// which we need to ignore to prevent forgetting the current
	// highlight or killing line drawing mode.
	in_operation_menu = true;

	const Fl_Menu_Item *item = menu->popup();

	in_operation_menu = false;

	if (item)
	{
		operation_command_t *info = (operation_command_t *)item->user_data();
		SYS_ASSERT(info);

		ExecuteCommand(info->cmd, info->param[0], info->param[1],
					   info->param[2], info->param[3]);
	}
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
