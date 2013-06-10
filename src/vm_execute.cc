//----------------------------------------------------------------------
//  VM : EXECUTION ENGINE
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


#define Con_Printf  printf


bool pr_nilcheck = true;


const char *pr_opnames[] =
{
	"DONE",

	"MUL_F",
	"DIV_F",
	"ADD_F",
	"SUB_F",

	"EQ_F",
	"EQ_S", 
	"EQ_ENT",
	"EQ_FNC",
	 
	"LT",
	"GT", 

	"NOT_F",
	"BOOL_F",
	"BOOL_V",
	"BOOL_S", 
	"BOOL_ENT", 
	"BOOL_FNC", 

	"BITAND",
	"BITOR",
	"BITXOR",
	"BITNOT",

	"IF",
	"IFNOT",
	"GOTO", 
	  
	"FRAME",
	"CALL",
	"RETURN",
	"STATE",

	"ADDRESS", 
	"LOAD",
	"LOAD_V",
	"STORE",
	"STOREP_V",

	"DUP",
	"POP",

	"MOD_F",
	"POWER",

	"LITERAL",

	"READ",
	"READ_V",
	"WRITE",
	"WRITE_V",

	"LOCAL_READ",
	"LOCAL_READ_V",
	"LOCAL_WRITE",
	"LOCAL_WRITE_V",

    "OP_VEC_ADD",
    "OP_VEC_SUB",
    "OP_VEC_PROD",
    "OP_VEC_MUL_F",
    "OP_VEC_DIV_F",
    "OP_VEC_EQ"
};


char *PR_GlobalString (int ofs);
char *PR_GlobalStringNoContents (int ofs);


extern void PC_format_string(void);


//=============================================================================


static void PR_PrintStatement(int ip)
{
	int  i;

	const dstatement_t *s = pr->statements + ip;
	
	if ( (unsigned)s->op < sizeof(pr_opnames) / sizeof(pr_opnames[0]))
	{
		fprintf (stderr, "%04X : line %4i : %s ", ip, s->linenum, pr_opnames[s->op]);
		i = strlen(pr_opnames[s->op]);
		for ( ; i<10 ; i++)
			fprintf (stderr, " ");
	}
		
	if (s->op == OP_IF || s->op == OP_IFNOT || s->op == OP_GOTO)
	{
//		Con_Printf ("branch %i",s->a);
	}
	else if (false) ///??? s->op == OP_READ || s->op == OP_WRITE)
	{
		if (s->a)
			Con_Printf ("%s",PR_GlobalString(s->a));
		if (s->b)
			Con_Printf ("%s",PR_GlobalString(s->b));
		if (s->c)
			Con_Printf ("%s", PR_GlobalStringNoContents(s->c));
	}
	fprintf (stderr, "\n");
}

/*
============
PR_StackTrace
============
*/
void PR_StackTrace (void)
{
	const dfunction_t	*f;
	int			i;
	
	if (pr->call_depth == 0)
	{
		Con_Printf ("<NO STACK>\n");
		return;
	}
	
	pr->call_stack[pr->call_depth].func = pr->x_func;

	for (i=pr->call_depth ; i>=0 ; i--)
	{
		f = pr->call_stack[i].func;
		
		if (!f)
		{
			Con_Printf ("<NO FUNCTION>\n");
		}
		else
			Con_Printf ("%12s : %s\n", mpr.strings + f->s_file, mpr.strings + f->s_name);
	}
}


/*
============
PR_Profile_f

============
*/
void PR_Profile_f (void)
{
	dfunction_t	*f, *best;
	int			max;
	int			num;
	int			i;
	
	num = 0;	
	do
	{
		max = 0;
		best = NULL;
		for (i=0 ; i < pr->numfunctions ; i++)
		{
			// Intentional Const Override
			f = (dfunction_t *)&mpr.functions[i];
			if (f->profile > max)
			{
				max = f->profile;
				best = f;
			}
		}
		if (best)
		{
			if (num < 10)
				Con_Printf ("%7i %s\n", best->profile, mpr.strings+best->s_name);
			num++;
			best->profile = 0;
		}
	} while (best);
}


int PR_Param_Int(int offset)
{
	kval_t *parms = &pr->stack[pr->frame];

	return parms[offset]._int;
}

float PR_Param_Float(int offset)
{
	kval_t *parms = &pr->stack[pr->frame];

	return parms[offset]._float;
}

const char * PR_Param_String(int offset)
{
	kval_t *parms = &pr->stack[pr->frame];

	return mpr.strings + parms[offset]._string;
}

void PR_Param_Vector(int offset, float *vec)
{
	kval_t *parms = &pr->stack[pr->frame];

	vec[0] = parms[offset + 0]._float;
	vec[1] = parms[offset + 1]._float;
	vec[2] = parms[offset + 2]._float;
}

int PR_Param_EdictNum(int offset)
{
	return PR_Param_Int(offset);
}

edict_t * PR_Param_Entity(int offset)
{
	return EDICT_NUM(PR_Param_EdictNum(offset));
}


void PR_Return_Int(int val)
{
	G_INT(OFS_RETURN) = val;
}

void PR_Return_Float(float val)
{
	G_FLOAT(OFS_RETURN) = val;
}

void PR_Return_Vector(float *val)
{
	G_FLOAT(OFS_RETURN + 0) = val[0];
	G_FLOAT(OFS_RETURN + 1) = val[1];
	G_FLOAT(OFS_RETURN + 2) = val[2];
}


void PR_Return_String(int offset)
{
	PR_Return_Int(offset);
}

void PR_Return_Entity(edict_t * ent)
{
	PR_Return_Int(EDICT_TO_PROG(ent));
}


/*
============
PR_RunError

Aborts the currently executing function
============
*/
void PR_RunError (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	Con_Printf("\nStack trace: %s\n", error);
	PR_StackTrace ();

	Con_Printf("Last statement:\n");
	PR_PrintStatement (pr->x_ip);

	Con_Printf ("SCRIPT ERROR: %s\n", string);

	pr->call_depth = 0;		// dump the stack so host_error can shutdown functions

	FatalError ("Program error");
}

/*
============================================================================
PR_ExecuteProgram

The interpretation main loop
============================================================================
*/

static void PR_CallBuiltin (const dfunction_t *f)
{
	// no need to save IP

	const dfunction_t * saved_func = pr->x_func;

	int saved_frame = pr->frame;

	pr->x_func     = f;
	pr->frame      = pr->next_frame;
	pr->next_frame = -1;

	{
		pr->builtins[f->first_statement].func ();
	}

	pr->stack_top  = pr->frame;
	pr->frame      = saved_frame;
	pr->x_func     = saved_func;

	// pop value stored by OP_FRAME instruction
	pr->next_frame = pr->stack[-- pr->stack_top]._int;
}


static void PR_CallFormatter (void)
{
	int saved_frame = pr->frame;

	pr->frame      = pr->next_frame;
	pr->next_frame = -1;

	{
		PC_format_string();
	}

	pr->stack_top  = pr->frame;
	pr->frame      = saved_frame;

	// pop value stored by OP_FRAME instruction
	pr->next_frame = pr->stack[-- pr->stack_top]._int;
}



/*
====================
PR_EnterFunction

Returns the new program statement counter
====================
*/
static int PR_EnterFunction (const dfunction_t *f)
{
	// save previous values of ip (etc)
	pr->call_stack[pr->call_depth].ip    = pr->x_ip;
	pr->call_stack[pr->call_depth].func  = pr->x_func;
	pr->call_stack[pr->call_depth].frame = pr->frame;

	pr->call_depth++;

	if (pr->call_depth >= MAX_CALL_STACK)
		PR_RunError ("call stack overflow\n");

	if (pr->stack_top >= MAX_LOCAL_STACK)
		PR_RunError ("stack overflow\n");


	pr->x_func = f;
	pr->x_ip   = f->first_statement;

	pr->frame      = pr->next_frame;
	pr->next_frame = -1;


	return pr->x_ip - 1;  // offset the ip++
}

/*
====================
PR_LeaveFunction
====================
*/
static int PR_LeaveFunction (void)
{
//	const dfunction_t *f = pr->x_func;


	pr->stack_top  = pr->frame;
	pr->next_frame = pr->stack[-- pr->stack_top]._int;


	// restore previous values of ip (etc)

	if (pr->call_depth <= 0)
		BugError ("call stack underflow");

	pr->call_depth--;

	pr->x_ip   = pr->call_stack[pr->call_depth].ip;
	pr->x_func = pr->call_stack[pr->call_depth].func;
	pr->frame  = pr->call_stack[pr->call_depth].frame;

	return pr->x_ip;
}


#define MAX_RUNAWAY  (1 << 30)


#define STACK(ofs)		pr->stack[pr->stack_top + (ofs) - 1]
#define STACK_F(ofs)	STACK(ofs)._float

#define DROP(n)			pr->stack_top -= (n)

#define PUSH_F(x)		do { if (pr->stack_top >= MAX_LOCAL_STACK) PR_RunError("Stack overflow (push)"); \
						pr->stack[pr->stack_top++]._float = (x); \
						} while(0)


/*
====================
PR_ExecuteProgram
====================

Note: never any parameters passed to it.
*/
void PR_ExecuteProgram (func_t fnum)
{
	int		ip;

	const dstatement_t	*st;
	const dfunction_t	*f, *newf;

	int		runaway;
	int		k;
	edict_t	*ed;
	int		exit_depth;
	int		exit_stack;
	kval_t	*ptr;

	if (!fnum || fnum >= pr->numfunctions)
	{
		if (pr->global_struct->self)
			ED_Print (PROG_TO_EDICT(pr->global_struct->self));

		Host_Error ("PR_ExecuteProgram: NULL function");
	}

	f = &mpr.functions[fnum];

/// fprintf(stderr, "PR_ExecuteProgram : %s\n", mpr.strings + f->s_name);

	pr->trace = false;

// make a stack frame
	exit_depth = pr->call_depth;
	exit_stack = pr->stack_top;


	// do a fake OP_FRAME
	pr->stack[pr->stack_top++]._int = pr->next_frame;
	pr->next_frame = pr->stack_top;


	ip = PR_EnterFunction(f);


	for (runaway = MAX_RUNAWAY ; runaway ; --runaway)
	{
		if (pr->stack_top < pr->frame)
			PR_RunError("Stack underflow");

		ip++;	// next statement

		pr->x_ip = ip;

		st = &pr->statements[ip];

		if (pr->trace)
			PR_PrintStatement(ip);
			
		switch (st->op)
		{
		case OP_ADD_F:
			STACK_F(-1) += STACK_F(0);
			DROP(1);
			break;

		case OP_SUB_F:
			STACK_F(-1) -= STACK_F(0);
			DROP(1);
			break;

		case OP_MUL_F:
			STACK_F(-1) *= STACK_F(0);
			DROP(1);
			break;

		case OP_DIV_F:
			if (STACK_F(0) == 0)
				PR_RunError("Division by zero");

			STACK_F(-1) /= STACK_F(0);
			DROP(1);
			break;
		

		case OP_BITAND:
			STACK_F(-1) = (int)STACK_F(-1) & (int)STACK_F(0);
			DROP(1);
			break;
		
		case OP_BITOR:
			STACK_F(-1) = (int)STACK_F(-1) | (int)STACK_F(0);
			DROP(1);
			break;
		
		case OP_BITXOR:
			STACK_F(-1) = (int)STACK_F(-1) ^ (int)STACK_F(0);
			DROP(1);
			break;
		
		case OP_BITNOT:
			// floating point only gives us 23 bits to work with
			STACK_F(0) = ~ (int)STACK_F(0) & 0x7FFFFF;
			DROP(1);
			break;
		

		case OP_LT:
			STACK_F(-1) = (STACK_F(-1) < STACK_F(0));
			DROP(1);
			break;

		case OP_GT:
			STACK_F(-1) = (STACK_F(-1) > STACK_F(0));
			DROP(1);
			break;


		case OP_NOT_F:
			STACK_F(0) = ! STACK_F(0);
			break;

		case OP_BOOL_F:
			STACK_F(0) = STACK_F(0) && 1;
			break;

		case OP_BOOL_V:
			STACK_F(-2) = STACK_F(-2) || STACK_F(-1) || STACK_F(0);
			DROP(2);
			break;

		case OP_BOOL_S:
			STACK_F(0) = STACK(0)._string && mpr.strings[STACK(0)._string];
			break;

		case OP_BOOL_FNC:
			STACK_F(0) = STACK(0)._func && 1;
			break;

		case OP_BOOL_ENT:
			STACK_F(0) = STACK(0)._edict && 1;
			break;


		case OP_EQ_F:
			STACK_F(-1) = (STACK_F(-1) == STACK_F(0));
			DROP(1);
			break;

		case OP_EQ_S:
			STACK_F(-1) = !strcmp(mpr.strings + STACK(-1)._string, mpr.strings + STACK(0)._string);
			DROP(1);
			break;

		case OP_EQ_ENT:
			STACK_F(-1) = (STACK(-1)._edict == STACK(0)._edict);
			DROP(1);
			break;

		case OP_EQ_FNC:
			STACK_F(-1) = (STACK(-1)._func == STACK(0)._func);
			DROP(1);
			break;


	//==================

		case OP_ADDRESS:
			if (pr_nilcheck)
				if (STACK(0)._edict == 0)
					PR_RunError ("nil entity access");

			ed = PROG_TO_EDICT(STACK(0)._edict);
			NUM_FOR_EDICT(ed);  // make sure it's in range

			{
				int _pointer = (byte *)(EDICT_VARS(ed) + st->a) - (byte *)sv.edicts;
				STACK(0)._int = _pointer;
			}
			break;

		case OP_LOAD:
			ptr = (kval_t *)((byte *)sv.edicts + STACK(0)._int);
			STACK(0) = *ptr;
			break;

		case OP_LOAD_V:
			ptr = (kval_t *)((byte *)sv.edicts + STACK(0)._int);
			DROP(1);

			if (pr->stack_top + 2 >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			pr->stack[pr->stack_top++] = ptr[0];
			pr->stack[pr->stack_top++] = ptr[1];
			pr->stack[pr->stack_top++] = ptr[2];
			break;
			
		case OP_STORE:
			//!!!  FIXME: check for assignment to world entity
			//!! if (ed == (edict_t *)sv.edicts && sv.state == ss_active)
			//!!	PR_RunError ("assignment to world entity");
			ptr = (kval_t *)((byte *)sv.edicts + STACK(0)._int);
			*ptr = STACK(-1);
			DROP(2);
			break;

		case OP_STORE_V:
			ptr = (kval_t *)((byte *)sv.edicts + STACK(0)._int);
			ptr[0] = STACK(-3);
			ptr[1] = STACK(-2);
			ptr[2] = STACK(-1);
			DROP(4);
			break;

	//==================

		case OP_IF:
			if (STACK(0)._int)
				ip += st->a - 1;	// offset the ip++
			DROP(1);
			break;

		case OP_IFNOT:
			if (! STACK(0)._int)
				ip += st->a - 1;	// offset the ip++
			DROP(1);
			break;

		case OP_GOTO:
			ip += st->a - 1;	// offset the ip++
			break;


		case OP_FRAME:
			// save the current next_frame value
			pr->stack[pr->stack_top++]._int = pr->next_frame;

			pr->next_frame = pr->stack_top;
			break;

		case OP_CALL:
			pr->argc = st->a;
			{
				func_t f = STACK(0)._func;
				DROP(1);

				if (! f)
					PR_RunError ("NULL function");

				newf = &mpr.functions[f];
			}

if (pr->trace) fprintf(stderr, "Calling : %s\n", mpr.strings + newf->s_name);

			if (pr->next_frame < 0)
				PR_RunError("PR_ExecuteProgram: no frame for OP_CALL");

			if (newf->builtin)
				PR_CallBuiltin (newf);
			else
				ip = PR_EnterFunction (newf);
			break;

		case OP_CALL_FORMAT:
			if (pr->next_frame < 0)
				PR_RunError("PR_ExecuteProgram: no frame for OP_CALL_FORMAT");

			PR_CallFormatter();
			break;

		case OP_DONE:
		case OP_RETURN:
			ip = PR_LeaveFunction ();

			if (pr->call_depth == exit_depth)
			{
				SYS_ASSERT(pr->stack_top == exit_stack);

if (pr->trace) fprintf(stderr, "PR_ExecuteProgram EXIT\n");
				return;		// all done
			}
			break;

		case OP_STATE:
			ed = PROG_TO_EDICT(pr->global_struct->self);
			ed->v.frame = st->a;
			ed->v.think = STACK(0)._func;
			ed->v.nextthink = pr->global_struct->time + st->b / 100.0;
			DROP(1);
			break;

		// andrewj: new or transitional stuff....

		case OP_DUP:
			if (pr->stack_top >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			pr->stack[pr->stack_top] = pr->stack[pr->stack_top - st->a];
			pr->stack_top++;
			break;

		case OP_POP:
			DROP(st->a);
			break;

		case OP_MOD_F:
			{
				float a = STACK_F(-1);
				float b = STACK_F(0);

				if (b == 0)
					PR_RunError("Division by zero");

				STACK_F(-1) = a - floor(a / b) * b;
				DROP(1);
			}
			break;

		case OP_POWER:
			STACK_F(-1) = pow(STACK_F(-1), STACK_F(0));
			DROP(1);
			break;

		case OP_LITERAL:
			if (pr->stack_top >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			// oh the humanity...
			pr->stack[pr->stack_top++] = *(const kval_t *)&st->a;
			break;

		case OP_READ:
			if (pr->stack_top >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			pr->stack[pr->stack_top++] = mpr.registers[st->a];
			break;

		case OP_READ_V:
			if (pr->stack_top + 2 >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			pr->stack[pr->stack_top++] = mpr.registers[st->a + 0];
			pr->stack[pr->stack_top++] = mpr.registers[st->a + 1];
			pr->stack[pr->stack_top++] = mpr.registers[st->a + 2];
			break;

		case OP_WRITE:
			if (pr->stack_top <= pr->frame) PR_RunError("stack underflow");
			mpr.registers[st->a] = pr->stack[-- pr->stack_top];
			break;

		case OP_WRITE_V:
			if (pr->stack_top - 2 <= pr->frame) PR_RunError("stack underflow");
			mpr.registers[st->a + 2] = pr->stack[-- pr->stack_top];
			mpr.registers[st->a + 1] = pr->stack[-- pr->stack_top];
			mpr.registers[st->a + 0] = pr->stack[-- pr->stack_top];
			break;


		case OP_LOCAL_READ:
			if (pr->stack_top >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			k = pr->frame + st->a;
			pr->stack[pr->stack_top++] = pr->stack[k];
			break;

		case OP_LOCAL_READ_V:
			if (pr->stack_top + 2 >= MAX_LOCAL_STACK) PR_RunError("stack overflow");
			k = pr->frame + st->a;
			pr->stack[pr->stack_top++] = pr->stack[k + 0];
			pr->stack[pr->stack_top++] = pr->stack[k + 1];
			pr->stack[pr->stack_top++] = pr->stack[k + 2];
			break;

		case OP_LOCAL_WRITE:
			if (pr->stack_top <= pr->frame) PR_RunError("stack underflow");
			k = pr->frame + st->a;
			pr->stack[k] = pr->stack[-- pr->stack_top];
			break;

		case OP_LOCAL_WRITE_V:
			if (pr->stack_top - 2 <= pr->frame) PR_RunError("stack underflow");
			k = pr->frame + st->a;
			pr->stack[k + 2] = pr->stack[-- pr->stack_top];
			pr->stack[k + 1] = pr->stack[-- pr->stack_top];
			pr->stack[k + 0] = pr->stack[-- pr->stack_top];
			break;

//------ Vector Ops --------//

		case OP_VEC_ADD:
			STACK_F(-5) += STACK_F(-2);
			STACK_F(-4) += STACK_F(-1);
			STACK_F(-3) += STACK_F(0);

			DROP(3);
			break;

		case OP_VEC_SUB:
			STACK_F(-5) -= STACK_F(-2);
			STACK_F(-4) -= STACK_F(-1);
			STACK_F(-3) -= STACK_F(0);

			DROP(3);
			break;

		case OP_VEC_PROD:
			STACK_F(-5) = STACK_F(-5) * STACK_F(-2) +
				          STACK_F(-4) * STACK_F(-1) +
				          STACK_F(-3) * STACK_F(0);
			DROP(5);
			break;

		case OP_VEC_MUL_F:
			{
				float f = STACK_F(0);

				STACK_F(-3) *= f;
				STACK_F(-2) *= f;
				STACK_F(-1) *= f;
			}
			DROP(1);
			break;

		case OP_VEC_DIV_F:
			{
				float f = STACK_F(0);
				if (f == 0)
					PR_RunError("Division by zero");

				STACK_F(-3) /= f;
				STACK_F(-2) /= f;
				STACK_F(-1) /= f;
			}
			DROP(1);
			break;

		case OP_VEC_EQ:
			STACK_F(-5) = (
			    STACK_F(-5) == STACK_F(-2) &&
				STACK_F(-4) == STACK_F(-1) &&
				STACK_F(-3) == STACK_F(0));

			DROP(5);
			break;

		default:
			PR_RunError ("Bad opcode %i", st->op);
		}
	}

	// infinite loop detected

	PR_RunError("runaway loop error");
}


// andrewj: added this, find function by name and run it

void PR_ExecuteProgramByName (const char *name)
{
	const dfunction_t * func = PR_FindFunction(name);

	if (! func)
	{
		FatalError ("Can't find function in progs: %s\n", name);
	}

	PR_ExecuteProgram (func - mpr.functions);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
