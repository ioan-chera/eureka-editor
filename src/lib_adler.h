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

#ifndef __LIB_CRC_H__
#define __LIB_CRC_H__

#include "m_strings.h"
#include <stdint.h>

#include "filesystem.hpp"
namespace fs = ghc::filesystem;

class crc32_c
{
private:
	uint32_t raw;
	uint32_t extra;

	static const uint32_t INIT_VALUE = 1;

public:
	crc32_c() : raw(INIT_VALUE), extra(0) { }
	crc32_c(const crc32_c &rhs) { raw = rhs.raw; extra = rhs.extra; }
	~crc32_c() { }

	void Reset(void) { raw = INIT_VALUE; extra = 0; }

	crc32_c& operator+= (uint8_t value);
	crc32_c& operator+= (int8_t value);
	crc32_c& operator+= (uint16_t value);
	crc32_c& operator+= (int16_t value);
	crc32_c& operator+= (uint32_t value);
	crc32_c& operator+= (int32_t value);
	crc32_c& operator+= (float value);
	crc32_c& operator+= (bool value);
	crc32_c &operator+= (const char *value)
	{
		return AddCStr(value);
	}
	crc32_c &operator+= (const SString &value)
	{
		return AddCStr(value.c_str());
	}

	crc32_c& AddBlock(const uint8_t *data, int len);
	crc32_c& AddCStr(const char *str);
	
	fs::path getPath() const
	{
		return SString::printf("%08X%08X.dat", extra, raw).get();
	}

  // TODO: operator==  and  operator!=
};


//------------------------------------------------------------------------
// IMPLEMENTATION
//------------------------------------------------------------------------

inline crc32_c& crc32_c::operator+= (int8_t value)
{
	*this += (uint8_t) value; return *this;
}

inline crc32_c& crc32_c::operator+= (int16_t value)
{
	*this += (uint16_t) value; return *this;
}

inline crc32_c& crc32_c::operator+= (int32_t value)
{
	*this += (uint32_t) value; return *this;
}

inline crc32_c& crc32_c::operator+= (bool value)
{
	*this += (value ? (uint8_t)1 : (uint8_t)0); return *this;
}

#endif // __LIB_CRC_H__
