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

#ifdef _WIN32
#include <Windows.h>
#endif

#define BugError  ThrowLogicException


[[noreturn]] void ThrowException(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);
[[noreturn]] void ThrowLogicException(EUR_FORMAT_STRING(const char *fmt), ...) EUR_PRINTF(1, 2);

#ifdef _WIN32
SString GetShellExecuteErrorMessage(HINSTANCE result);
SString GetWindowsErrorMessage(DWORD error);
#endif

#endif /* Errors_hpp */
