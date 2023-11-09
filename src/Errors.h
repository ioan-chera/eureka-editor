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
#include "tl/expected.hpp"

#include <stdarg.h>
#include <stdexcept>

#define BugError  ThrowLogicException

template<typename T>
using Failable = tl::expected<T, SString>;

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


[[noreturn]] void ThrowException(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);
[[noreturn]] void ThrowLogicException(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);

inline static tl::unexpected<typename std::decay<SString>::type> fail(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);

inline static tl::unexpected<typename std::decay<SString>::type> fail(EUR_FORMAT_STRING(const char *fmt), ...)
{
	va_list ap;
	va_start(ap, fmt);
	auto result = tl::make_unexpected(SString::vprintf(fmt, ap));
	va_end(ap);
	return result;
}

inline static tl::unexpected<typename std::decay<SString>::type> fail(const std::runtime_error &e)
{
	return tl::make_unexpected(SString(e.what()));
}


template<typename T>
inline static T attempt(Failable<T> &&operation)
{
	if(!operation)
		throw std::runtime_error(operation.error().get());
	return operation.value();
}

template<>
inline  void attempt(Failable<void> &&operation)
{
	if(!operation)
		throw std::runtime_error(operation.error().get());
}


#endif /* Errors_hpp */
