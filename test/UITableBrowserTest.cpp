//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2025
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

#include "ui_table_browser.h"

#include <FL/Fl.H>
#include "gtest/gtest.h"

static void IncrementCallback(Fl_Widget *, void *data)
{
	int *count = (int *)data;
	(*count)++;
}

TEST(UITableBrowserTest, AddClearAndText)
{
	UI_TableBrowser table(0, 0, 100, 100, nullptr);
	table.add("row1\tmode1\tfunc1");
	table.add("row2\tmode2\tfunc2");
	EXPECT_EQ(2, table.size());
	EXPECT_STREQ("row1\tmode1\tfunc1", table.text(1));
	EXPECT_EQ(1, table.value());
	table.clear();
	EXPECT_EQ(0, table.size());
	EXPECT_EQ(0, table.value());
}

TEST(UITableBrowserTest, TextModifyAndRemove)
{
	UI_TableBrowser table(0, 0, 100, 100, nullptr);
	table.add("first\ta\tb");
	table.add("second\tc\td");
	table.text(2, "second_mod\te\tf");
	EXPECT_STREQ("second_mod\te\tf", table.text(2));
	table.remove(1);
	EXPECT_EQ(1, table.size());
	EXPECT_STREQ("second_mod\te\tf", table.text(1));
	EXPECT_EQ(1, table.value());
}

TEST(UITableBrowserTest, SelectionCallback)
{
	UI_TableBrowser table(0, 0, 100, 100, nullptr);
	table.add("alpha\tb\tc");
	table.add("beta\td\te");
	int count = 0;
	table.SetSelectionCallback(IncrementCallback, &count);
	table.select(2);
	EXPECT_EQ(1, count);
	EXPECT_EQ(2, table.value());
	table.select(0);
	EXPECT_EQ(2, count);
	EXPECT_EQ(0, table.value());
}

TEST(UITableBrowserTest, ColumnWidthsAndHeaders)
{
	UI_TableBrowser table(0, 0, 100, 100, nullptr);
	int widths[] = {50, 60, 70, 0};
	table.column_widths(widths);
	EXPECT_EQ(50, table.col_width(0));
	EXPECT_EQ(60, table.col_width(1));
	EXPECT_EQ(70, table.col_width(2));
	std::vector<std::string> headers = {"H1", "H2", "H3"};
	table.SetColumnHeaders(headers);
	EXPECT_EQ(3, table.cols());
}

TEST(UITableBrowserTest, EnsureVisibleScrolls)
{
	UI_TableBrowser table(0, 0, 100, 100, nullptr);
	for (int i = 0; i < 20; i++)
		table.add("row\t\t");
	table.row_position(0);
	table.EnsureVisible(20);
	EXPECT_GT(table.row_position(), 0);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
