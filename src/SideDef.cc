//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2022 Ioan Chera
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

#include "SideDef.h"
#include "e_basis.h"
#include "Instance.h"
#include "m_strings.h"

SString SideDef::UpperTex() const
{
	return global::basis_strtab.get(upper_tex);
}

SString SideDef::MidTex() const
{
	return global::basis_strtab.get(mid_tex);
}

SString SideDef::LowerTex() const
{
	return global::basis_strtab.get(lower_tex);
}

void SideDef::SetDefaults(const Instance &inst, bool two_sided, int new_tex)
{
	if (new_tex < 0)
		new_tex = BA_InternaliseString(inst.conf.default_wall_tex);

	lower_tex = new_tex;
	upper_tex = new_tex;

	if (two_sided)
		mid_tex = BA_InternaliseString("-");
	else
		mid_tex = new_tex;
}


Sector * SideDef::SecRef(const Document &doc) const
{
	return doc.sectors[sector];
}
