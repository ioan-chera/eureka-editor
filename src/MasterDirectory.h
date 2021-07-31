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

#ifndef MasterDirectory_h
#define MasterDirectory_h

class SString;
class Wad_file;

//
// Master directory of loaded wads
//
struct MasterDirectory
{
	void add(Wad_file *wad);
	void closeAll();
	bool haveFilename(const SString &chk_path) const;
	void remove(Wad_file *wad);

	std::vector<Wad_file *> dir;	// the IWAD, never NULL, always at master_dir.front()
};

#endif /* MasterDirectory_h */
