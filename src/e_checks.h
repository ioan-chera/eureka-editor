//------------------------------------------------------------------------
//  INTEGRITY CHECKS
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2009 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_E_CHECKS_H__
#define __EUREKA_E_CHECKS_H__

#include "DocumentModule.h"
#include "ui_window.h"

// the CHECK_xxx functions return the following values:
enum class CheckResult
{
	ok,				// no issues at all
	minorProblem,	// only minor issues
	majorProblem,	// some major problems
	highlight,		// need to highlight stuff (skip further checks)
	tookAction		// [internal use : user took some action]
};

//
// The map checking module
//
class ChecksModule : public DocumentModule
{
	friend class Instance;
public:
	ChecksModule(Document &doc) : DocumentModule(doc)
	{
	}

	void sidedefsUnpack(bool is_after_load) const;
	void tagsApplyNewValue(int new_tag);
	void tagsUsedRange(int *min_tag, int *max_tag) const;

private:
	void checkAll(bool majorStuff) const;

	CheckResult checkVertices(int minSeverity) const;
	CheckResult checkSectors(int minSeverity) const;
	CheckResult checkThings(int minSeverity) const;
	CheckResult checkLinedefs(int minSeverity) const;
	CheckResult checkTags(int minSeverity) const;
	CheckResult checkTextures(int minSeverity) const;

	int copySidedef(EditOperation &op, int num) const;
};

int findFreeTag(const Instance &inst, bool forsector);

#endif  /* __EUREKA_E_CHECKS_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
