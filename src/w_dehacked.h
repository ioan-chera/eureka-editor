//------------------------------------------------------------------------
//  DEHACKED
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2016 Andrew Apted
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

// THANKS TO Isaac Colón (https://github.com/iccolon818) for this contribution. I added reformatting
// and some input checking.

#ifndef __EUREKA_W_DEHACKED_H__
#define __EUREKA_W_DEHACKED_H__

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

class MasterDir;
struct ConfigData;

namespace dehacked
{
struct frame_t
{
	int spritenum;
	int subspritenum;
	bool bright;
};

bool loadFile(const fs::path &resource, ConfigData &config);
void loadLumps(const MasterDir &master, ConfigData &config);
}

#endif
