//----------------------------------------------------------------------
//  VM : PUBLIC API
//----------------------------------------------------------------------
// 
//  Copyright (C) 2009-2013  Andrew Apted
//  Copyright (C) 1996-1997  Id Software, Inc.
//
//  This vm is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as
//  published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This vm is distributed in the hope that it will be useful, but
//  WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
//  the GNU General Public License for more details.
//
//----------------------------------------------------------------------
//
//  NOTE:
//       This was originally based on QCC (Quake-C Compiler) and the
//       corresponding virtual machine from the Quake source code.
//       However it has migrated a long way from those roots.
//
//----------------------------------------------------------------------

#ifndef __VM_API_H__
#define __VM_API_H__


#define VM_NIL  0


void VM_Init();

int  VM_CompileFile(const char *filename);

int  VM_FindFunction(const char *name);

void VM_Frame();
int  VM_Call(int func_id);

void VM_Push_Int(int v);
void VM_Push_Float(float v);
void VM_Push_String(const char *s);

int   VM_Result_Int();
float VM_Result_Float();
const char * VM_Result_String();


#endif /* __VM_API_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
