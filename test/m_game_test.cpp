//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2021 Ioan Chera
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

#include "m_game.h"

#include "gtest/gtest.h"



//========================================================================
//
// Mockups
//
//========================================================================

namespace global
{
	SString home_dir;
	SString install_dir;
}

//
// Parse color mockup
//
rgb_color_t ParseColor(const SString &cstr)
{
	// Use some independent example
	return (rgb_color_t)strtol(cstr.c_str(), nullptr, 16) << 8;
}
