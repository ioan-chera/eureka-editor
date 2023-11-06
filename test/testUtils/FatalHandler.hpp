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

#ifndef FatalHandler_hpp
#define FatalHandler_hpp

#include <iostream>
#include <stdexcept>

//
// Handles a runtime_error exception fatally. Needed for death tests.
//
template<typename Callable>
void Fatal(const Callable &callable)
{
	try
	{
		callable();
	}
	catch(const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		abort();
	}
}

#endif /* FatalHandler_hpp */
