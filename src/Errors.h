//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2020 Ioan Chera
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

#ifndef Errors_hpp
#define Errors_hpp

#include "PrintfMacros.h"
#include "m_strings.h"

#include <stdarg.h>
#include <stdexcept>

#define BugError  ThrowException

//
// Wad read exception
//
class WadReadException : public std::runtime_error
{
public:
	WadReadException(const SString& msg) : std::runtime_error(msg.c_str())
	{
	}
};

//
// Result with message in case of failure
//
struct ReportedResult
{
	bool success;
	SString message;
};

//
// Raises an exception with the given format
//
template<typename T>
[[noreturn]] void raise(EUR_FORMAT_STRING(const char *format), ...)
EUR_PRINTF(1, 2);

template<typename T>
[[noreturn]] void raise(EUR_FORMAT_STRING(const char *format), ...)
{
	va_list ap;
	::va_start(ap, format);
	SString text = SString::vprintf(format, ap);
	::va_end(ap);
	throw T(text);
}

[[noreturn]] void ThrowException(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);

#endif /* Errors_hpp */
