//------------------------------------------------------------------------
//  STREAMS HANDLING
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

#include "m_streams.h"
#include "m_strings.h"

//
// this automatically strips CR/LF from the line.
// returns true if ok, false on EOF or error.
//
bool M_ReadTextLine(SString &string, std::istream &is)
{
    string.clear();
    std::getline(is, string.get());
    if(is.bad() || is.fail())
        return false;
    if(is.eof() && string.empty())
        return false;
    if(string.substr(0, 3) == "\xef\xbb\xbf")
        string.erase(0, 3);
    string.trimTrailingSpaces();
    return true;
}

//
// Opens the file
//
bool LineFile::open(const fs::path &path) noexcept
{
    is.close();
    is.open(path);
    return is.is_open();
}

//
// Close if out of scope
//
void LineFile::close() noexcept
{
    if(is.is_open())
        is.close();
}
