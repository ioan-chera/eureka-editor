//------------------------------------------------------------------------
//  TABLE BROWSER WIDGET
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025 Andrew Apted
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

#ifndef __EUREKA_UI_TABLE_BROWSER_H__
#define __EUREKA_UI_TABLE_BROWSER_H__

#include <FL/Fl_Table.H>
#include <vector>
#include <string>

class UI_TableBrowser : public Fl_Table
{
private:
	std::vector<std::string> data_rows;
	std::vector<std::string> column_headers;
	std::vector<int> default_col_widths;
	
	int selected_row;
	int sort_column;
	bool sort_ascending;
	
	char column_separator;
	
	// Callback for header clicks (sorting)
	void (*sort_callback)(Fl_Widget *w, void *data);
	void *sort_callback_data;
	
	// Callback for double-click on first column
	void (*rebind_callback)(Fl_Widget *w, void *data);
	void *rebind_callback_data;
	
	// Callback for selection change
	void (*selection_callback)(Fl_Widget *w, void *data);
	void *selection_callback_data;
	
protected:
	void draw_cell(TableContext context, int R, int C, int X, int Y, int W, int H) FL_OVERRIDE;
	int handle(int event) FL_OVERRIDE;
	
	void ParseRowData(const std::string &row_text, std::vector<std::string> &columns);
	void DrawCellText(const std::string &text, int X, int Y, int W, int H, bool selected = false, bool header = false);
	
public:
	UI_TableBrowser(int X, int Y, int W, int H, const char *label = 0);
	virtual ~UI_TableBrowser();
	
	// Interface compatible with Fl_Hold_Browser
	void clear() override;
	void add(const char *text);
	void text(int line, const char *new_text);
	void remove(int line);
	int size() const { return (int)data_rows.size(); }
	
	// Selection methods
	int value() const { return selected_row; }
	void select(int line);
	bool selected(int line) const { return line == selected_row; }
	const char *text(int line) const;
	
	// Column management
	void column_char(char sep) { column_separator = sep; }
	void column_widths(const int *widths);
	void SetColumnHeaders(const std::vector<std::string> &headers);
	
	// Callback methods
	void SetSortCallback(void (*cb)(Fl_Widget *w, void *data), void *data);
	void SetRebindCallback(void (*cb)(Fl_Widget *w, void *data), void *data);
	void SetSelectionCallback(void (*cb)(Fl_Widget *w, void *data), void *data);
	
	// Utility methods
	void EnsureVisible(int line);
	void AutoFitColumn(int col);
	int GetSortColumn() const { return sort_column; }
	bool GetSortAscending() const { return sort_ascending; }
};

#endif  /* __EUREKA_UI_TABLE_BROWSER_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab