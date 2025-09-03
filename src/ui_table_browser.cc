//------------------------------------------------------------------------
//  TABLE BROWSER WIDGET
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Ioan Chera
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

#include "Errors.h"
#include "Instance.h"

#include "main.h"

#include "ui_table_browser.h"

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <algorithm>
#include <sstream>

static const char column_separator = '\t';

UI_TableBrowser::UI_TableBrowser(int X, int Y, int W, int H, const char *label) :
	Fl_Table(X, Y, W, H, label)
{
	// Enable column headers and resizing
	col_header(1);
	col_resize(1);
	col_header_height(25);
	
	// Set default properties
	selection_color(FL_SELECTION_COLOR);
	when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);
	
	// Set initial table dimensions
	rows(0);
	cols(3); // Default: Key, Mode, Function
	
	// Set default column widths
	default_col_widths = {125, 75, 205, 0};
	column_widths(default_col_widths.data());
}

void UI_TableBrowser::clear()
{
	data_rows.clear();
	selected_row = 0;
	rows(0);
	redraw();
}

void UI_TableBrowser::add(const char *text)
{
	if (!text)
		return;
	
	data_rows.push_back(std::string(text));
	rows((int)data_rows.size());
	
	// If this is the first item, select it
	if (data_rows.size() == 1)
		selected_row = 1;
		
	redraw();
}

void UI_TableBrowser::text(int line, const char *new_text)
{
	if (line < 1 || line > (int)data_rows.size() || !new_text)
		return;
		
	data_rows[line - 1] = std::string(new_text);
	redraw();
}

const char *UI_TableBrowser::text(int line) const
{
	if (line < 1 || line > (int)data_rows.size())
		return "";
		
	return data_rows[line - 1].c_str();
}

void UI_TableBrowser::remove(int line)
{
	if (line < 1 || line > (int)data_rows.size())
		return;
		
	data_rows.erase(data_rows.begin() + (line - 1));
	rows((int)data_rows.size());
	
	// Adjust selected row if necessary
	if (selected_row >= line && selected_row > 1)
		selected_row--;
	else if (selected_row == line && selected_row > (int)data_rows.size())
		selected_row = (int)data_rows.size();
		
	redraw();
}

void UI_TableBrowser::select(int line)
{
	if (line < 0 || line > (int)data_rows.size())
		selected_row = 0;
	else
		selected_row = line;
		
	redraw();
	
	if (selection_callback)
		selection_callback(this, selection_callback_data);
}

void UI_TableBrowser::column_widths(const int *widths)
{
	if (!widths) return;
	
	default_col_widths.clear();
	for (int i = 0; widths[i] != 0; i++)
	{
		default_col_widths.push_back(widths[i]);
		col_width(i, widths[i]);
	}
	default_col_widths.push_back(0); // Terminator
	
	redraw();
}

void UI_TableBrowser::SetColumnHeaders(const std::vector<std::string> &headers)
{
	column_headers = headers;
	cols((int)headers.size());
	redraw();
}

void UI_TableBrowser::SetSortCallback(void (*cb)(Fl_Widget *w, void *data), void *data)
{
	sort_callback = cb;
	sort_callback_data = data;
}

void UI_TableBrowser::SetRebindCallback(void (*cb)(Fl_Widget *w, void *data), void *data)
{
	rebind_callback = cb;
	rebind_callback_data = data;
}

void UI_TableBrowser::SetSelectionCallback(void (*cb)(Fl_Widget *w, void *data), void *data)
{
	selection_callback = cb;
	selection_callback_data = data;
}

static std::vector<std::string> ParseRowData(const std::string &row_text)
{	
	if (row_text.empty())
		return {};
		
	std::vector<std::string> columns;
	std::string current_col;
	bool in_color_code = false;
	
	for (size_t i = 0; i < row_text.length(); i++)
	{
		char c = row_text[i];
		
		// Handle color codes (starting with @)
		if (c == '@' && i + 2 < row_text.length() && 
			row_text[i+1] == 'C' && isdigit(row_text[i+2]))
		{
			in_color_code = true;
			current_col += c;
			continue;
		}
		
		if (in_color_code && (c == ' ' || i == row_text.length() - 1))
		{
			in_color_code = false;
		}
		
		if (c == column_separator && !in_color_code)
		{
			columns.push_back(current_col);
			current_col.clear();
		}
		else
		{
			current_col += c;
		}
	}
	
	// Add the last column
	if (!current_col.empty() || !columns.empty())
		columns.push_back(current_col);

	return columns;
}

void UI_TableBrowser::DrawCellText(const std::string &text, int X, int Y, int W, int H, unsigned flags) const
{
	// Set colors
	if (flags & DCT_HEADER)
	{
		fl_color(FL_GRAY);
		fl_rectf(X, Y, W, H);
		fl_color(FL_BLACK);
	}
	else if (flags & DCT_SELECTED)
	{
		fl_color(selection_color());
		fl_rectf(X, Y, W, H);
		fl_color(FL_WHITE);
	}
	else
	{
		fl_color(FL_WHITE);
		fl_rectf(X, Y, W, H);
		fl_color(FL_BLACK);
	}
	
	// Draw cell border
	fl_color(FL_GRAY);
	fl_rect(X, Y, W, H);
	
	if (text.empty())
		return;
	
	// Handle color codes in text
	std::string display_text = text;
	Fl_Color text_color = flags & DCT_SELECTED ? FL_WHITE : FL_BLACK;
	
	// Simple color code parsing - strip @C codes but use the color
	if (text.length() >= 3 && text[0] == '@' && text[1] == 'C' && isdigit(text[2]))
	{
		int color_index = text[2] - '0';
		switch (color_index)
		{
			case 1:
				text_color = flags & DCT_SELECTED ? FL_CYAN : FL_RED;
				break;
			case 2:
				text_color = flags & DCT_SELECTED ? FL_MAGENTA : FL_DARK_GREEN;
				break;
			case 3:
				text_color = flags & DCT_SELECTED ? FL_YELLOW : FL_BLUE;
				break;
			default:
				text_color = flags & DCT_SELECTED ? FL_WHITE : FL_BLACK;
				break;
		}
		
		size_t space_pos = text.find(' ', 3);
		if (space_pos != std::string::npos)
			display_text = text.substr(space_pos + 1);
		else
			display_text = text.substr(3);
	}
	
	fl_color(flags & DCT_HEADER ? FL_BLACK : text_color);
	
	// Set font
	if (flags & DCT_HEADER)
		fl_font(FL_HELVETICA_BOLD, FL_NORMAL_SIZE);
	else
		fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	
	// Draw text with padding
	fl_push_clip(X + 2, Y, W - 4, H);
	fl_draw(display_text.c_str(), X + 4, Y + 4, W - 8, H - 8, FL_ALIGN_LEFT);
	fl_pop_clip();
}

void UI_TableBrowser::draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H)
{
	switch (context)
	{
		case CONTEXT_STARTPAGE:
			fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
			break;
			
		case CONTEXT_COL_HEADER:
			if (C < (int)column_headers.size())
				DrawCellText(column_headers[C], X, Y, W, H, DCT_HEADER);
			break;
		
		case CONTEXT_CELL:
			if (R >= 0 && R < (int)data_rows.size())
			{
				std::vector<std::string> columns = ParseRowData(data_rows[R]);
				
				bool is_selected = (R + 1 == selected_row);
				
				if (C < (int)columns.size())
					DrawCellText(columns[C], X, Y, W, H, is_selected ? DCT_SELECTED : 0);
				else
					DrawCellText("", X, Y, W, H, is_selected ? DCT_SELECTED : 0);
			}
			break;
		
		case CONTEXT_TABLE:
			fl_color(FL_WHITE);
			fl_rectf(X, Y, W, H);
			break;
			
		default:
			break;
	}
}

int UI_TableBrowser::handle(int event)
{
	int ret = Fl_Table::handle(event);
	
	switch (event)
	{
		case FL_PUSH:
			if (Fl::event_clicks() == 1) // Double-click
			{
				TableContext context = callback_context();
				int R = callback_row();
				int C = callback_col();
				
				if (context == CONTEXT_CELL && C == 0 && rebind_callback)
				{
					// Double-click on first column - trigger rebind
					select(R + 1);
					rebind_callback(this, rebind_callback_data);
					return 1;
				}
				else if (context == CONTEXT_COL_HEADER)
				{
					// Double-click on header - auto-fit column

					// FIXME: this isn't useful, as it is triggered after sorting due to first click.

					// AutoFitColumn(C);
					// return 1;
				}
			}
			else if (Fl::event_clicks() == 0) // Single click
			{
				TableContext context = callback_context();
				int R = callback_row();
				int C = callback_col();
				
				if (context == CONTEXT_CELL)
				{
					select(R + 1);
					return 1;
				}
				else if (context == CONTEXT_COL_HEADER && sort_callback)
				{
					// Click on column header - sort
					if (sort_column == C)
						sort_ascending = !sort_ascending;
					else
					{
						sort_column = C;
						sort_ascending = true;
					}
					
					sort_callback(this, sort_callback_data);
					return 1;
				}
			}
			break;
		
		case FL_KEYDOWN:
		{
			int key = Fl::event_key();
			if (key == FL_Up && selected_row > 1)
			{
				select(selected_row - 1);
				EnsureVisible(selected_row);
				return 1;
			}
			else if (key == FL_Down && selected_row < (int)data_rows.size())
			{
				select(selected_row + 1);
				EnsureVisible(selected_row);
				return 1;
			}
			break;
		}
	}
	
	return ret;
}

void UI_TableBrowser::EnsureVisible(int line)
{
	if (line < 1 || line > (int)data_rows.size())
		return;
		
	// Convert to 0-based row index
	int row = line - 1;
	
	// Check if row is visible
	if (row < toprow)
	{
		// Scroll up to show this row
		row_position(row);
	}
	else if (row > botrow)
	{
		// Scroll down to show this row
		row_position(row - (botrow - toprow));
	}
}

void UI_TableBrowser::AutoFitColumn(int col)
{
	if (col < 0 || col >= cols())
		return;
		
	fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	
	int max_width = 50; // Minimum width
	
	// Check header width
	if (col < (int)column_headers.size())
	{
		fl_font(FL_HELVETICA_BOLD, FL_NORMAL_SIZE);
		int header_width = (int)fl_width(column_headers[col].c_str()) + 10;
		max_width = std::max(max_width, header_width);
		fl_font(FL_HELVETICA, FL_NORMAL_SIZE);
	}
	
	// Check data widths
	for (const std::string &row_text : data_rows)
	{
		std::vector<std::string> columns = ParseRowData(row_text);
		
		if (col < (int)columns.size())
		{
			std::string display_text = columns[col];
			
			// Strip color codes for width calculation
			if (display_text.length() >= 3 && display_text[0] == '@' && 
				display_text[1] == 'C' && isdigit(display_text[2]))
			{
				size_t space_pos = display_text.find(' ', 3);
				if (space_pos != std::string::npos)
					display_text = display_text.substr(space_pos + 1);
				else
					display_text = display_text.substr(3);
			}
			
			int text_width = (int)fl_width(display_text.c_str()) + 12; // Padding
			max_width = std::max(max_width, text_width);
		}
	}
	
	col_width(col, max_width);
	redraw();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab