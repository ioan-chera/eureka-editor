//------------------------------------------------------------------------
//  EDGE Endian handling
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2006-2008 Andrew Apted
//  Copyright (C) 2025 Ioan Chera
//  Copyright (C) 2025 Cristian Rodríguez
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
//  Using code from SDL_byteorder.h and SDL_endian.h.
//  Copyright (C) 1997-2004 Sam Lantinga.
//
//------------------------------------------------------------------------

#pragma once

#if __has_include(<endian.h>)
#include <endian.h>
// older BSDs
#elif __has_include(<sys/endian.h>)
#include <sys/endian.h>
#endif

#if defined(__APPLE__) && !defined(le16toh)
// NOTE: le16toh may already be defined in <sys/endian.h> for MacOSX.sdk
#include <machine/endian.h>
#define le16toh(X)  OSSwapLittleToHostInt16(X)
#define le32toh(X)  OSSwapLittleToHostInt32(X)
#define be16toh(X)  OSSwapBigToHostInt16(X)
#define be32toh(X)  OSSwapBigToHostInt32(X)
#endif

#if defined(_WIN32)
#if defined(_MSVC_VER)
#include <stdlib.h>
#define bswap_16(x) _byteswap_ushort(x)
#define bswap_32(x) _byteswap_ulong(x)
#elif defined(__GNUC__) || defined(__clang__)
#  define bswap_16(x) __builtin_bswap16((uint16_t)(x))
#  define bswap_32(x) __builtin_bswap32((uint32_t)(x))
#endif
// There is only little-endian windows
#define le16toh(X)  (X)
#define le32toh(X)  (X)
#define be16toh(X)  bswap_16(X)
#define be32toh(X)  bswap_32(X)
#endif

// ---- byte swap from specified endianness to native ----

#define LE_U16(X)  le16toh(X)
#define LE_U32(X)  le32toh(X)
#define BE_U16(X)  be16toh(X)
#define BE_U32(X)  be32toh(X)

// signed versions of the above
#define LE_S16(X)  ((int16_t) LE_U16((uint16_t) (X)))
#define LE_S32(X)  ((int32_t) LE_U32((uint32_t) (X)))
#define BE_S16(X)  ((int16_t) BE_U16((uint16_t) (X)))
#define BE_S32(X)  ((int32_t) BE_U32((uint32_t) (X)))


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
