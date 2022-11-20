//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#include "m_bitvec.h"
#include "gtest/gtest.h"

TEST(BitVec, Construct)
{
    bitvec_c vec(16);
    ASSERT_EQ(vec.size(), 16);
}

TEST(BitVec, Get)
{
    bitvec_c vec;
    ASSERT_FALSE(vec.get(1));
    ASSERT_FALSE(vec.get(2));
    ASSERT_FALSE(vec.get(4));
    ASSERT_FALSE(vec.get(8));
    ASSERT_FALSE(vec.get(16));
    ASSERT_FALSE(vec.get(32));
    ASSERT_FALSE(vec.get(64));
    ASSERT_FALSE(vec.get(128));
    ASSERT_FALSE(vec.get(256));
    ASSERT_FALSE(vec.get(512));
    ASSERT_FALSE(vec.get(1024));
}

TEST(BitVec, SetClearToggle)
{
    bitvec_c vec(0);	// must be okay
    vec.set(100);
	ASSERT_GE(vec.size(), 100);
	int lastSize = vec.size();
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 100);

	vec.set(50);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 100 || i == 50);

	vec.toggle_all();
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i != 100 && i != 50);

	vec.toggle_all();
	vec.clear(100);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 50);

	vec.clear(150);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 50);

	vec.clear(50);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_FALSE(vec.get(i));

	vec.toggle(25);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 25);

	vec.toggle(25);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_FALSE(vec.get(i));

	vec.set_all();
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_TRUE(vec.get(i));

	vec.clear(150);
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i != 150);

	vec.clear_all();
	ASSERT_EQ(vec.size(), lastSize);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_FALSE(vec.get(i));
}

TEST(BitVec, Frob)
{
	bitvec_c vec;
	vec.frob(100, BitOp::add);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 100);

	vec.frob(50, BitOp::add);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 100 || i == 50);

	vec.frob(100, BitOp::remove);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 50);

	vec.frob(150, BitOp::remove);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 50);

	vec.frob(50, BitOp::remove);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_FALSE(vec.get(i));

	vec.frob(25, BitOp::toggle);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_EQ(vec.get(i), i == 25);

	vec.frob(25, BitOp::toggle);
	for(int i = 0; i < vec.size(); ++i)
		ASSERT_FALSE(vec.get(i));
}

