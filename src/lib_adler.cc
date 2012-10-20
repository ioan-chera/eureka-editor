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

#include "main.h"

#include "lib_adler.h"

// ---- Primitive routines ----

crc32_c& crc32_c::operator+= (u8_t data)
{
	u32_t s1 = raw & 0xFFFF;
	u32_t s2 = (raw >> 16) & 0xFFFF;

	s1 = (s1 + data) % 65521;
	s2 = (s2 + s1)   % 65521;

	raw = (s2 << 16) | s1;

  extra += s2;

  // modulo the extra value by a large prime number
  if (extra >= 0xFFFEFFF9)
      extra -= 0xFFFEFFF9;

	return *this;
}

crc32_c& crc32_c::AddBlock(const u8_t *data, int len)
{
	u32_t s1 = raw & 0xFFFF;
	u32_t s2 = (raw >> 16) & 0xFFFF;

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

crc32_c& crc32_c::operator+= (u16_t value)
{
	*this += (u8_t) (value >> 8);
	*this += (u8_t) (value);

	return *this;
}

crc32_c& crc32_c::operator+= (u32_t value)
{
	*this += (u8_t) (value >> 24);
	*this += (u8_t) (value >> 16);
	*this += (u8_t) (value >> 8);
	*this += (u8_t) (value);

	return *this;
}

crc32_c& crc32_c::operator+= (float value)
{
	bool neg = (value < 0.0f);
	value = (float)fabs(value);

	int exp;
	u32_t mant = (u32_t) (ldexp(frexp(value, &exp), 30));

	*this += (u8_t) (neg ? '-' : '+');
	*this += (u32_t) exp;
	*this += mant;

	return *this;
}

crc32_c& crc32_c::AddCStr(const char *str)
{
	return AddBlock((const u8_t *) str, (int)strlen(str));
}

