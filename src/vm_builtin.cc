//----------------------------------------------------------------------
//  VM : BUILTIN FUNCTIONS
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

#include "main.h"

#include "vm_local.h"

/*
===============================================================================

				COMMON BUILT-IN FUNCTIONS

===============================================================================
*/

const char * PC_VarString (int first)
{
	int  i;
	static char out[256];
	
	out[0] = 0;

	for (i = first ; i < pr->argc ; i++)
	{
		strcat (out, PR_Param_String(i));
	}

	return out;
}


void PC_blah_blah (void)
{
	const char *s = PR_Param_String(0);

	fprintf(stderr, "%s\n", s);
}


/*
=================
PC_error

This is a TERMINAL error, which will kill off the entire server.

error(value)
=================
*/
void PC_error (void)
{
	const char	*s;

	s = PC_VarString(0);

	Con_Printf ("======SERVER ERROR in %s:\n%s\n",
		 pr->strings + pr->x_func->s_name, s);

	Host_Error ("Program error");
}


/*
=================
PC_normalize

vector normalize(vector)
=================
*/
void PC_normalize (void)
{
	vec3_t	value1;
	vec3_t	newvalue;

	float	new;

	PR_Param_Vector(0, value1);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);
	
	if (new == 0)
		newvalue[0] = newvalue[1] = newvalue[2] = 0;
	else
	{
		new = 1/new;
		newvalue[0] = value1[0] * new;
		newvalue[1] = value1[1] * new;
		newvalue[2] = value1[2] * new;
	}
	
	PR_Return_Vector(newvalue);
}

/*
=================
PC_vlen

scalar vlen(vector)
=================
*/
void PC_vlen (void)
{
	vec3_t	value1;
	float	new;
	
	PR_Param_Vector(0, value1);

	new = value1[0] * value1[0] + value1[1] * value1[1] + value1[2]*value1[2];
	new = sqrt(new);

	PR_Return_Float(new);
}

/*
=================
PC_vectoyaw

float vectoyaw(vector)
=================
*/
void PC_vectoyaw (void)
{
	vec3_t	value1;
	float	yaw;
	
	PR_Param_Vector(0, value1);

	if (value1[1] == 0 && value1[0] == 0)
		yaw = 0;
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;
	}

	PR_Return_Float(yaw);
}


/*
=================
PC_vectoangles

vector vectoangles(vector)
=================
*/
void PC_vectoangles (void)
{
	vec3_t	value1;
	vec3_t	vec;

	float	forward;
	float	yaw, pitch;
	
	PR_Param_Vector(0, value1);

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
	}
	else
	{
		yaw = (int) (atan2(value1[1], value1[0]) * 180 / M_PI);
		if (yaw < 0)
			yaw += 360;

		forward = sqrt (value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (int) (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}

	vec[0] = pitch;
	vec[1] = yaw;
	vec[2] = 0;

	PR_Return_Vector(vec);
}


/*
=================
PC_Random

Returns a number from 0.0 to 1.0 (full range)

random()
=================
*/
void PC_random (void)
{
	PR_Return_Float((rand() & 0x7fff) / (float)0x7fff);
}


/*
=================
PC_break

break()
=================
*/
void PC_break (void)
{
	Con_Printf ("break statement\n");
	*(int *)-4 = 0;	// dump to debugger
//	PR_RunError ("break statement");
}


/*
=================
PC_cvar

float cvar (string)
=================
*/
void PC_cvar (void)
{
	const char *str = PR_Param_String(0);

	PR_Return_Float(Cvar_VariableValue (str));
}

/*
=================
PC_cvar_set

float cvar (string)
=================
*/
void PC_cvar_set (void)
{
	const char *var, *val;
	
	var = PR_Param_String(0);
	val = PR_Param_String(1);

	Cvar_Set (var, val);
}


/*
=========
PC_dprint
=========
*/
void PC_dprint (void)
{
fprintf(stderr, "%s", PC_VarString(0));
//	Con_DPrintf ("%s",PC_VarString(0));
}

void PC_fabs (void)
{
	float	v;
	v = PR_Param_Float(0);
	PR_Return_Float(fabs(v));
}


// andrewj: temp string is now part of string buffer (after the empty string)

void PC_ftos (void)
{
	float v = PR_Param_Float(0);
	
	if (v == (int)v)
		sprintf ((char *)pr->strings + 1, "%d",(int)v);
	else
		sprintf ((char *)pr->strings + 1, "%5.1f",v);
	
	PR_Return_String(1);
}

void PC_vtos (void)
{
	vec3_t val;

	PR_Param_Vector(0, val);

	sprintf ((char *)pr->strings + 1, "'%5.1f %5.1f %5.1f'", val[0], val[1], val[2]);

	PR_Return_String(1);
}

void PC_etos (void)
{
	int num = PR_Param_EdictNum(0);

	// Intentional Const Override
	if (! num)
		strcpy ((char *)pr->strings + 1, "nil");
	else
		sprintf ((char *)pr->strings + 1, "entity_%i", num);

	PR_Return_String(1);
}


void PC_coredump (void)
{
	ED_PrintEdicts ();
}

void PC_traceon (void)
{
	pr->trace = true;
}

void PC_traceoff (void)
{
	pr->trace = false;
}


void PC_rint (void)
{
	float	f;
	f = PR_Param_Float(0);
	if (f > 0)
		PR_Return_Float((int)(f + 0.5));
	else
		PR_Return_Float((int)(f - 0.5));
}

void PC_floor (void)
{
	PR_Return_Float(floor(PR_Param_Float(0)));
}

void PC_ceil (void)
{
	PR_Return_Float(ceil(PR_Param_Float(0)));
}

void PC_sin (void)
{
	PR_Return_Float(sin(PR_Param_Float(0)));
}

void PC_cos (void)
{
	PR_Return_Float(cos(PR_Param_Float(0)));
}

void PC_sqrt (void)
{
	PR_Return_Float(sqrt(PR_Param_Float(0)));
}


void PC_Fixme (void)
{
	PR_RunError ("unimplemented builtin");
}


/*
===============================================================================

				STRING FORMATTING

===============================================================================
*/

// andrewj: added this formatting code

static int fmt_parm_ofs;  // position in stack for next parameter

static int  fmt_field_width;
static int  fmt_precision;
static int  fmt_left_justify;
static char fmt_val_kind;


static const char * DecodeSpecifier(const char *fmt)
{
	char buf[64];
	char *b;

	fmt_field_width  = -1;
	fmt_precision    = -1;
	fmt_left_justify = 0;

	if (*fmt == '-')
	{
		fmt_left_justify = 1;
		fmt++;
	}

	if (isdigit(*fmt))
	{
		b = buf;
		while (isdigit(*fmt)) *b++ = *fmt++;
		*b = 0;

		fmt_field_width = atoi(buf);
	}

	if (*fmt == '.')
	{
		fmt++;

		b = buf;
		while (isdigit(*fmt)) *b++ = *fmt++;
		*b = 0;

		fmt_precision = atoi(buf);
	}

	fmt_val_kind = *fmt;

	if (*fmt)
		fmt++;

	return fmt;
}


static char * OutputSpecifier(char *p)
{
	char buf[256];
	vec3_t vec;
	int len, spaces;
	int num;

// create the unjustified representation
	switch (fmt_val_kind)
	{
		case 'd':
			sprintf(buf, "%d", I_ROUND(PR_Param_Float(fmt_parm_ofs)));
			fmt_parm_ofs++;
			break;

		case 'f':
			if (fmt_precision < 0)
				fmt_precision = 3;

			sprintf(buf, "%1.*f", fmt_precision, PR_Param_Float(fmt_parm_ofs));
			fmt_parm_ofs++;
			break;

		case 'e':
			num = PR_Param_EdictNum(fmt_parm_ofs);
			if (! num)
				sprintf(buf, "nil");
			else
				sprintf(buf, "entity_%i", PR_Param_EdictNum(fmt_parm_ofs));
			fmt_parm_ofs++;
			break;

		case 's':
			strcpy(buf, PR_Param_String(fmt_parm_ofs));
			fmt_parm_ofs++;
			break;

		case 'v':
			if (fmt_precision < 0)
				fmt_precision = 3;

			PR_Param_Vector(fmt_parm_ofs, vec);
			fmt_parm_ofs += 3;

			sprintf(buf, "'%1.*f %1.*f %1.*f'",
				fmt_precision, vec[0],
				fmt_precision, vec[1],
				fmt_precision, vec[2]);
			break;

		// should not happen
		default:
			strcpy(buf, "[?!?!]");
			break;
	}

// justify the value by adding spaces
	if (fmt_field_width > 200)
		fmt_field_width = 200;

	len    = strlen(buf);
	spaces = fmt_field_width - len;

	if (! fmt_left_justify)
		for (; spaces > 0 ; spaces--)
			*p++ = ' ';

	strcpy(p, buf); p += len;

	if (fmt_left_justify)
		for (; spaces > 0 ; spaces--)
			*p++ = ' ';

	return p;
}


void PC_format_string (void)
{
	const char *fmt = PR_Param_String(0);

	// place into a buffer [rather than PR temp string] for two
	// reasons: (1) easier to detect overflow, (2) allows _one_ of
	// the parameters to be the PR temp string.
	static char result[512];

	char *p = result;

	fmt_parm_ofs = 1;  // offset #0 is the format string

	while (*fmt)
	{
		// check for overflow : silently truncate it
		if (p - result > 250)
			break;

		if (*fmt != '%')
		{
			*p++ = *fmt++;
			continue;
		}

		fmt++;

		if (*fmt == '%')
		{
			*p++ = *fmt++;
			continue;
		}

		fmt = DecodeSpecifier(fmt);

		p = OutputSpecifier(p);
	}

	*p = 0;
	result[250] = 0;

	strcpy((char *)pr->strings + 1, result);

	PR_Return_String(1);
}


//============================================================================

builtin_t  builtins[] =
{
	// very first one is a dummy -- never used
	{ "<DUMMY>",  PC_Fixme },

	//======= COMMON =========

	{ "error",          PC_error },	// void(string e) error				= #10;

	{ "random",         PC_random },	// float() random						= #7;
	{ "rint",           PC_rint },
	{ "floor",          PC_floor },
	{ "ceil",           PC_ceil },
	{ "fabs",           PC_fabs },
	{ "sin",            PC_sin },
	{ "cos",            PC_cos },
	{ "sqrt",           PC_sqrt },
	{ "normalize",      PC_normalize },	// vector(vector v) normalize			= #9;
	{ "vlen",           PC_vlen },	// float(vector v) vlen				= #12;
	{ "vectoyaw",       PC_vectoyaw },	// float(vector v) vectoyaw		= #13;
	{ "vectoangles",    PC_vectoangles },

	{ "ftos",           PC_ftos },	// void(string s) ftos				= #26;
	{ "vtos",           PC_vtos },	// void(string s) vtos				= #27;
	{ "etos",           PC_etos },

	{ "blah",           PC_blah_blah },
	{ "break",          PC_break },	// void() break						= #6;
	{ "coredump",       PC_coredump },
	{ "dprint",         PC_dprint },	// void(string s) dprint				= #25;
	{ "traceon",        PC_traceon },
	{ "traceoff",       PC_traceoff },

	{ "cvar",           PC_cvar },
	{ "cvar_set",       PC_cvar_set },

	//======== OTHER ===========


	// end of list
	{ NULL, NULL }
};


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
