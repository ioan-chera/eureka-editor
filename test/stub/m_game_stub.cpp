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

#include "Instance.h"
#include "m_game.h"

bool Instance::is_sky(const SString &flat) const
{
   return false;
}

const linetype_t &Instance::M_GetLineType(int type) const
{
   static linetype_t linetype;
   return linetype;
}

const sectortype_t &Instance::M_GetSectorType(int type) const
{
   static sectortype_t sectortype;
   return sectortype;
}

const thingtype_t &ConfigData::getThingType(int type)
{
   static thingtype_t thingtype;
   return thingtype;
}
