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

#ifndef M_STREAMS_H_
#define M_STREAMS_H_

#include <fstream>
#include <istream>

#include <filesystem>
namespace fs = std::filesystem;

class SString;

// returns true if ok, false on EOF or error
bool M_ReadTextLine(SString &string, std::istream &is);

//
// File to be read by line (encapsulated)
//
class LineFile
{
public:
    //
    // Read one line
    //
    bool readLine(SString &line)
    {
        return M_ReadTextLine(line, is);
    }

    bool open(const fs::path &path) noexcept;
    void close() noexcept;
    LineFile() = default;
    explicit LineFile(const fs::path &path) noexcept
    {
        open(path);
    }
    ~LineFile() noexcept
    {
        close();
    }

    //
    // Easy check
    //
    bool isOpen() const noexcept
    {
        return is.is_open();
    }
private:
    std::ifstream is;
};

#endif
