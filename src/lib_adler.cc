//------------------------------------------------------------------------
//  ADLER-32 CHECKSUM
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2012 Andrew Apted
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
//  This is the Adler-32 algorithm as described in RFC-1950.
//
//  The 'extra' field is my own adaptation to provide an extra 32 bits
//  of checksum.  This should make collisions a lot less likely (though
//  how much remains to be seen -- definitely not the full 32 bits!).
//
//------------------------------------------------------------------------


#include "lib_adler.h"
#include <math.h>

// ---- Primitive routines ----

crc32_c& crc32_c::operator+= (uint8_t data)
{
	uint32_t s1 = raw & 0xFFFF;
	uint32_t s2 = (raw >> 16) & 0xFFFF;

	s1 = (s1 + data) % 65521;
	s2 = (s2 + s1)   % 65521;

	raw = (s2 << 16) | s1;

  extra += s2;

  // modulo the extra value by a large prime number
  if (extra >= 0xFFFEFFF9)
      extra -= 0xFFFEFFF9;

	return *this;
}

crc32_c& crc32_c::AddBlock(const uint8_t *data, int len)
{
	uint32_t s1 = raw & 0xFFFF;
	uint32_t s2 = (raw >> 16) & 0xFFFF;

	for (; len > 0; data++, len--)
	{
		s1 = (s1 + *data) % 65521;
		s2 = (s2 + s1)    % 65521;

    extra += s2;
    if (extra >= 0xFFFEFFF9)
        extra -= 0xFFFEFFF9;
	}

	raw = (s2 << 16) | s1;

	return *this;
}

// ---- Non-primitive routines ----

crc32_c& crc32_c::operator+= (uint16_t value)
{
	*this += (uint8_t) (value >> 8);
	*this += (uint8_t) (value);

	return *this;
}

crc32_c& crc32_c::operator+= (uint32_t value)
{
	*this += (uint8_t) (value >> 24);
	*this += (uint8_t) (value >> 16);
	*this += (uint8_t) (value >> 8);
	*this += (uint8_t) (value);

	return *this;
}

crc32_c& crc32_c::operator+= (float value)
{
	bool neg = (value < 0.0f);
	value = (float)fabs(value);

	int exp;
	uint32_t mant = (uint32_t) (ldexp(frexp(value, &exp), 30));

	*this += (uint8_t) (neg ? '-' : '+');
	*this += (uint32_t) exp;
	*this += mant;

	return *this;
}

crc32_c& crc32_c::AddCStr(const char *str)
{
	return AddBlock((const uint8_t *) str, (int)strlen(str));
}

