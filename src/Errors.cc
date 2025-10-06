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

#include "Errors.h"

#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

//
// Throw an exception using format
//
void ThrowException(EUR_FORMAT_STRING(const char *fmt), ...)
{
	char message[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(message, sizeof(message), fmt, ap);
	va_end(ap);
	throw std::runtime_error(message);
}
void ThrowLogicException(EUR_FORMAT_STRING(const char *fmt), ...)
{
	char message[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(message, sizeof(message), fmt, ap);
	va_end(ap);
	throw std::logic_error(message);
}

#ifdef _WIN32
SString GetShellExecuteErrorMessage(HINSTANCE result)
{
	INT_PTR val = (INT_PTR)result;
	if (val > 32)
	{
		return "OK";
	}
	switch (val)
	{
	case 0:
		return "out of memory or resources";
	case ERROR_FILE_NOT_FOUND:
		return "file not found";
	case ERROR_PATH_NOT_FOUND:
		return "path not found";
	case ERROR_BAD_FORMAT:
		return "invalid executable file";
	case SE_ERR_ACCESSDENIED:
		return "access denied";
	case SE_ERR_ASSOCINCOMPLETE:
		return "incomplete or invalid file name association";
	case SE_ERR_DDEBUSY:
		return "DDE transaction busy";
	case SE_ERR_DDEFAIL:
		return "DDE transaction failure";
	case SE_ERR_DDETIMEOUT:
		return "DDE request time out";
	case SE_ERR_DLLNOTFOUND:
		return "DLL not found";
	case SE_ERR_NOASSOC:
		return "no app associated with given file name extension (or file is not printable)";
	case SE_ERR_OOM:
		return "out of memory";
	case SE_ERR_SHARE:
		return "a sharing violation occurred";
	default:
		return SString::printf("unexpected error (code %zu)", (size_t)val);
	}
}

SString GetWindowsErrorMessage(DWORD error)
{
	if (!error)
	{
		return "The operation completed successfully.";
	}
	LPWSTR messageBuffer = nullptr;
	size_t size = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&messageBuffer, 0, nullptr);
	if (!size)
	{
		return SString::printf("(failed getting error message from %lu, reason of this failure having code %lu)", (unsigned long)error, (unsigned long)GetLastError());
	}

	// Convert wide string to UTF-8
	int utf8Length = WideCharToMultiByte(CP_UTF8, 0, messageBuffer, (int)size, nullptr, 0, nullptr, nullptr);
	std::string utf8Result;
	if (utf8Length > 0)
	{
		utf8Result.resize(utf8Length);
		WideCharToMultiByte(CP_UTF8, 0, messageBuffer, (int)size, &utf8Result[0], utf8Length, nullptr, nullptr);
	}
	SString result(utf8Result);

	LocalFree(messageBuffer);
	result.trimTrailingSpaces();

	return result;
}
#endif
