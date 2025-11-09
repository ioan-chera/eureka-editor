//------------------------------------------------------------------------
//  UDMF PARSING / WRITING
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

#ifndef M_UDMF_H_
#define M_UDMF_H_

struct UDMFMapping
{
	enum class Set
	{
		line1,
		line2,
		thing
	};

	const char *name;
	Set set;
	unsigned value;
};

const auto& getUDMFMapping();

const UDMFMapping *UDMF_MappingForName(const char *name);


#endif
