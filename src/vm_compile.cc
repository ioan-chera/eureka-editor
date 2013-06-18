//----------------------------------------------------------------------
//  VM : COMPILER
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


int			pr_edict_size;

static def_t * all_defs;

static int pr_error_count;


//========================================

dfunction_t	*pr_scope;		// the function being parsed, or NULL

bool	pr_dumpasm;

int		pr_local_ofs;

jmp_buf		pr_parse_abort;		// longjump with this on parse error

static void PR_ParseLocal (void);


extern const char *pr_opnames[];


typedef struct
{
	const char	*name;
	int			op;
	int			priority;
	const char  *flags;
	// Flags:
	//   'b' = result is boolean
	//   'n' = negate boolean result
	//   'w' = swap arguments
	type_t		*type_a, *type_b, *type_c;

} operator_t;


// fake operations
#define FAKE_OP_AND  251
#define FAKE_OP_OR   252


//========================================

static operator_t unary_operators[] =
{
	{"!", OP_NOT_F,    5, "b",  &type_float,    &type_float},

	{"!", OP_BOOL_V,   5, "bn", &type_vector,   &type_float},
	{"!", OP_BOOL_S,   5, "bn", &type_string,   &type_float},
	{"!", OP_BOOL_ENT, 5, "bn", &type_entity,   &type_float},
	{"!", OP_BOOL_FNC, 5, "bn", &type_function, &type_float},

	{"~", OP_BITNOT,   5, "",  &type_float,    &type_float},

	{NULL}
};


static operator_t binary_operators[] =
{
	/* priority 1 is for function calls and field access */

	{"^", OP_POWER,     2, "",  &type_float,  &type_float,  &type_float},

	{"*", OP_MUL_F,     3, "",  &type_float,  &type_float,  &type_float},
	{"*", OP_VEC_PROD,  3, "",  &type_vector, &type_vector, &type_float},
	{"*", OP_VEC_MUL_F, 3, "",  &type_vector, &type_float,  &type_vector},
	{"*", OP_VEC_MUL_F, 3, "w", &type_float,  &type_vector, &type_vector},
 
	{"/", OP_DIV_F,     3, "", &type_float,  &type_float, &type_float},
	{"/", OP_VEC_DIV_F, 3, "", &type_vector, &type_float, &type_vector},
	{"%", OP_MOD_F,     3, "", &type_float,  &type_float, &type_float},

	{"&",  OP_BITAND,  3, "", &type_float, &type_float, &type_float},
	{"|",  OP_BITOR,   3, "", &type_float, &type_float, &type_float},
	{"^^", OP_BITXOR,  3, "", &type_float, &type_float, &type_float},

	{"+", OP_ADD_F,   4, "", &type_float, &type_float, &type_float},
	{"+", OP_VEC_ADD, 4, "", &type_vector, &type_vector, &type_vector},
  
	{"-", OP_SUB_F,   4, "", &type_float, &type_float, &type_float},
	{"-", OP_VEC_SUB, 4, "", &type_vector, &type_vector, &type_vector},

	{"==", OP_EQ_F,   5, "b", &type_float, &type_float, &type_float},
	{"==", OP_EQ_S,   5, "b", &type_string, &type_string, &type_float},
	{"==", OP_EQ_ENT, 5, "b", &type_entity, &type_entity, &type_float},
	{"==", OP_EQ_FNC, 5, "b", &type_function, &type_function, &type_float},
	{"==", OP_VEC_EQ, 5, "b", &type_vector, &type_vector, &type_float},
 
	{"!=", OP_EQ_F,   5, "bn", &type_float, &type_float, &type_float},
	{"!=", OP_EQ_S,   5, "bn", &type_string, &type_string, &type_float},
	{"!=", OP_EQ_ENT, 5, "bn", &type_entity, &type_entity, &type_float},
	{"!=", OP_EQ_FNC, 5, "bn", &type_function, &type_function, &type_float},
	{"!=", OP_VEC_EQ, 5, "bn", &type_vector, &type_vector, &type_float},
 
	{"<",  OP_LT,  5, "b",  &type_float, &type_float, &type_float},
	{">",  OP_GT,  5, "b",  &type_float, &type_float, &type_float},
	{">=", OP_LT,  5, "bn", &type_float, &type_float, &type_float},
	{"<=", OP_GT,  5, "bn", &type_float, &type_float, &type_float},

	{"&&", FAKE_OP_AND, 6, "b", &type_float, &type_float, &type_float},
	{"||", FAKE_OP_OR,  7, "b", &type_float, &type_float, &type_float},

	/* priority 8 is for the ?: ternary operator */

	/* priority 9 is for assignment */

	{NULL}
};

#define TERNARY_PRIORITY	8
#define ASSIGN_PRIORITY		9

#define TOP_PRIORITY		9


//===========================================================================


static dstatement_t * PR_EmitOp(int opcode, int a, int b, int c);
static void PR_EmitKVal(kval_t val);
static void PR_PatchOp(dstatement_t *patch);


typedef enum
{
	EV_INVALID = 0,

	EV_LITERAL,
	EV_VARIABLE,     // constants too (which are just read-only vars)
	EV_ASSIGNMENT,

	EV_UNARY_OP,
	EV_BINARY_OP,
	EV_TERNARY_OP,

	EV_FUNC_CALL,
	EV_FORMAT_STR,
	EV_FIELD_ACCESS

} eval_kind_e;


typedef struct eval_s
{
	int kind;  // EV_XXX

	type_t * type;  // type computed by this eval

	struct eval_s * args[MAX_PARMS + 1];  // for FUNC_CALL, [0] is function itself

	kval_t  literal[3];

	def_t * def;  // variable for EV_VARIABLE
	              // field for EV_FIELD_ACCESS

	int op;	   // for EV_UNARY_OP or EV_BINARY_OP
	const char *op_flags;

	int  num_parms;  // for EV_FUNC_CALL, EV_FORMAT_STR

} eval_t;


static eval_t * EXP_Expression(int priority);
static void CODEGEN_Eval(eval_t *ev, bool no_result);
static void CODEGEN_Boolean(eval_t *ev);
static void PR_ParseStatement(void);
static def_t *PR_GetDef (const char *name, type_t *type, dfunction_t *scope, int allocate);


static eval_t * PR_AllocEval(int kind, type_t *type)
{
	eval_t * ev = new eval_t;

	memset(ev, 0, sizeof(eval_t));

	ev->kind = kind;
	ev->type = type;

	return ev;
}


static object_ref_c * empty_str;


static object_ref_c * GlobalizeString(const char *str)
{
	if (! str[0])
		return empty_str;

	object_ref_c * ref = object_ref_c::NewString(str);

	ref->MakePermanent();

	return ref;
}


static eval_t * EXP_Literal(void)
{
	eval_t *ev = PR_AllocEval(EV_LITERAL, pr_immediate_type);

	if (ev->type->kind == ev_vector)
	{
		ev->literal[0]._float = pr_immediate_float[0];
		ev->literal[1]._float = pr_immediate_float[1];
		ev->literal[2]._float = pr_immediate_float[2];
	}
	else if (ev->type->kind == ev_string)
	{
		ev->literal[0]._string = GlobalizeString(pr_immediate_string);
	}
	else  // ev_float
	{
		ev->literal[0]._float = pr_immediate_float[0];
	}

	PR_Lex();

	return ev;
}


static eval_t * EXP_Name(void)
{
	def_t *d;
	eval_t *ev;
	const char *name;

	name = PR_ParseName ();

	d = PR_GetDef (name, NULL, pr_scope, 0);

	if (!d)
		PR_ParseError ("unknown identifier: %s", name);

	switch (d->type->kind)
	{
		case ev_float:
		case ev_vector:
		case ev_string:
		case ev_entity:
		case ev_function:
			// variable or constant
			ev = PR_AllocEval(EV_VARIABLE, d->type);
			ev->def = d;
			return ev;

		default:
			PR_ParseError ("name terminal not supported (yet)");
			return NULL;  /* not reached */
	}
}


static eval_t * EXP_UnaryOp(void)
{
	int k;
	eval_t *ev;
	eval_t *unary;

	const operator_t *op;

	// find it...
	for (op = unary_operators ; op->name ; op++)
	{
		if (! PR_Check(op->name))
			continue;

		// get expression which the unary operator applies to
		ev = EXP_Expression(op->priority);

		for (k = 0 ; op[k].name && strcmp(op->name, op[k].name) == 0 ; k++)
		{
			if (ev->type->kind != op[k].type_a->kind)
				continue;

			op = &op[k];

			unary = PR_AllocEval(EV_UNARY_OP, op->type_b);

			unary->args[0] = ev;

			unary->op       = op->op;
			unary->op_flags = op->flags;

			return unary;
		}

		PR_ParseError("type mismatch for %s", op->name);
	}

	PR_ParseError("expected value, got %s", pr_token);
	return NULL; /* not reached */
}


static int FormatStr_Decode(const char *str, etype_t *types)
{
	int count = 0;

	while (*str)
	{
		if (*str++ != '%')
			continue;

		// check for %%
		if (*str == '%')
		{
			str++;
			continue;
		}

		if (count >= MAX_PARMS)
			PR_ParseError("format string requires too many parameters");

		while (isdigit(*str) || *str == '.' || *str == '-')
			str++;

		if (! ('a' <= *str && *str <= 'z'))
		{
			// invalid formatter
			PR_ParseError("invalid format string");
		}

		switch (*str)
		{
			case 'd':
			case 'f': types[count] = ev_float;  break;
			case 'e': types[count] = ev_entity; break;
			case 's': types[count] = ev_string; break;
			case 'v': types[count] = ev_vector; break;

			default:
				PR_ParseError("invalid format letter: '%c'", *str);
		}

		str++;

		count++;
	}

	return count;
}


static eval_t * EXP_FormatString(void)
{
	eval_t *fmt;
	etype_t types[MAX_PARMS];
	int num_parms, got;

	PR_Expect("(");

// grab the format string
	if (pr_token_type != tt_immediate || pr_immediate_type->kind != ev_string)
		PR_ParseError ("missing format string");

	fmt = PR_AllocEval(EV_FORMAT_STR, &type_string);

	fmt->literal[0]._string = GlobalizeString(pr_immediate_string);

// determine number/type of parameters from string
	num_parms = FormatStr_Decode(pr_immediate_string, types);

	PR_Lex();

// grab the other parameters
	got = 0;

	while (PR_Check(","))
	{
		eval_t *parm;

		parm = EXP_Expression(TOP_PRIORITY);

		if (got >= num_parms)
			PR_ParseError("too many parameters for format");

		if (parm->type->kind != types[got])
			PR_ParseError("type mismatch on format parm #%d", got + 1);

		fmt->args[got++] = parm;
	}

	PR_Expect(")");

	if (got < num_parms)
		PR_ParseError("not enough parameters for format");

	fmt->num_parms = got;

	return fmt;
}


static eval_t * EXP_Term(void)
{
	if (pr_token_type == tt_immediate)
		return EXP_Literal();

	if (PR_Check("("))
	{
		eval_t * ev = EXP_Expression(TOP_PRIORITY);
		PR_Expect(")");
		return ev;
	}

	if (pr_token_type != tt_name)
		return EXP_UnaryOp();

	if (PR_Check("true"))
	{
		eval_t *ev = PR_AllocEval(EV_LITERAL, &type_float);
		ev->literal[0]._float = 1;
		return ev;
	}
	else if (PR_Check("false"))
	{
		eval_t *ev = PR_AllocEval(EV_LITERAL, &type_float);
		ev->literal[0]._float = 0;
		return ev;
	}
	else if (PR_Check("format"))
	{
		return EXP_FormatString();
	}

	// FIXME: handle 'nil' too

	return EXP_Name();
}


static eval_t * EXP_FunctionCall(eval_t *e1)
{
	def_t *func = e1->def;
	eval_t *call;
	int got;

	if (e1->type->kind != ev_function)
		PR_ParseError ("not a function");

	call = PR_AllocEval(EV_FUNC_CALL, func->type->aux_type);

	call->args[0] = e1;
	call->num_parms = e1->type->num_parms;

	got = 0;

	if (! PR_Check(")"))
	{
		do
		{
			if (got >= call->num_parms)
				PR_ParseError("too many parameters (need %d)", call->num_parms);

			call->args[1 + got] = EXP_Expression(TOP_PRIORITY);

			got++;

			// FIXME!!! check type

		} while (PR_Check(","));

		PR_Expect(")");
	}

	if (got < call->num_parms)
		PR_ParseError("too few parameters (got %d, need %d)", got, call->num_parms);


	return call;
}


static eval_t * EXP_FieldAccess(eval_t *e1)
{
	def_t *d;
	eval_t *acc;
	const char *name;

	// TODO: field access for vectors (e.g. dir.z instead of dir_z)

	if (e1->type->kind != ev_entity)
		PR_ParseError("bad field access (not an entity, but %d)", e1->type->kind);

	name = PR_ParseName ();

	d = PR_GetDef (name, NULL, pr_scope, 0);

	if (!d || d->type->kind != ev_field)
		PR_ParseError("unknown field: %s", name);

	acc = PR_AllocEval(EV_FIELD_ACCESS, d->type->aux_type);

	acc->args[0] = e1;
	acc->def = d;
			
	return acc;
}


static eval_t * EXP_Assignment(eval_t *e1, eval_t *e2)
{
	// assigning e2 to e1

	eval_t *assign;

	// check for a valid "lvalue"

	if (! (e1->kind == EV_VARIABLE || e1->kind == EV_FIELD_ACCESS) ||
	    e1->type->kind == ev_void)
	{
		PR_ParseError("bad assignment (lhs not an lvalue)");
	}

	// check type
	// FIXME: TypeMatch(...)
	if (e1->type->kind != e2->type->kind)
		PR_ParseError("type mismatch in assignment");

	// check for constant
	if (e1->kind == EV_VARIABLE && e1->def->initialized)
		PR_ParseError("assignment to a constant");

	assign = PR_AllocEval(EV_ASSIGNMENT, e2->type);

	assign->args[0] = e1;
	assign->args[1] = e2;

	return assign;
}


static eval_t * EXP_Ternary(eval_t *cond)
{
	eval_t *tern;
	eval_t *e_true;
	eval_t *e_false;


	e_true = EXP_Expression(TOP_PRIORITY);

	PR_Expect(":");

	e_false = EXP_Expression(TOP_PRIORITY);

	if (e_true->type->kind != e_false->type->kind)
		PR_ParseError("type mismatch in ?: operator");


	tern = PR_AllocEval(EV_TERNARY_OP, e_true->type);

	tern->args[0] = cond;
	tern->args[1] = e_true;
	tern->args[2] = e_false;

	return tern;
}


static eval_t * EXP_MakeBinary(const operator_t *op, eval_t *e1, eval_t *e2)
{
	int k;
	eval_t *bin;

	for (k = 0 ; op[k].name && strcmp(op->name, op[k].name) == 0 ; k++)
	{
		if (e1->type->kind != op[k].type_a->kind)
			continue;

		if (e2->type->kind != op[k].type_b->kind)
			continue;

		op = &op[k];

		bin = PR_AllocEval(EV_BINARY_OP, op->type_c);

		if (strchr(op->flags, 'w'))  // swapped
		{
			eval_t *tmp = e1; e1 = e2; e2 = tmp;
		}

		bin->args[0] = e1;
		bin->args[1] = e2;

		bin->op       = op->op;
		bin->op_flags = op->flags;

		return bin;
	}

	PR_ParseError("type mismatch for %s", op->name);
	return NULL; /* not reached */
}


static eval_t * EXP_Expression(int priority)
{
	eval_t * e1;
	eval_t * e2;

	const operator_t *op;
	bool found;


	if (priority == 0)
		return EXP_Term();

	e1 = EXP_Expression(priority - 1);

    // loop through a sequence of same-priority operators

	do
	{
		found = false;

		if (priority == 1 && PR_Check("("))
			return EXP_FunctionCall(e1);

		if (priority == 1 && PR_Check("."))
		{
			e1 = EXP_FieldAccess(e1);
			found = true;
			continue;
		}

		if (priority == ASSIGN_PRIORITY && PR_Check("="))
		{
			// right associative, so get RHS at same priority
			// (and hence no need to continue through loop)

			e2 = EXP_Expression(priority);

			return EXP_Assignment(e1, e2);
		}

		if (priority == TERNARY_PRIORITY && PR_Check("?"))
		{
			return EXP_Ternary(e1);
		}

		for (op = binary_operators ; op->name ; op++)
		{
			if (op->priority != priority)
				continue;

			if (! PR_Check(op->name))
				continue;

			found = true;

			// get expression for right side
			e2 = EXP_Expression(priority - 1);

			e1 = EXP_MakeBinary(op, e1, e2);
			break;
		}

	} while (found);

	return e1;
}


#if 0
static void CODEGEN_DropValue(type_t *type)
{
	switch (type->kind)
	{
		case ev_void:
			break;  // nothing to drop

		case ev_vector:
			PR_EmitOp(OP_POP, 3, 0, 0);
			break;

		default:
			PR_EmitOp(OP_POP, 1, 0, 0);
			break;
	}
}
#endif


static void CODEGEN_Literal(const kval_t * literal, type_t *type)
{
	PR_EmitOp(OP_LITERAL, 0, 0, 0);

	int count = (type->kind == ev_vector) ? 3 : 1;

	for (int i = 0 ; i < count ; i++)
	{
		PR_EmitKVal(literal[i]);
	}
}


static void CODEGEN_VarValue(eval_t * ev)
{
	def_t *var = ev->def;

	if (var->scope)
	{
		if (ev->type->kind == ev_vector)
			PR_EmitOp(OP_LOCAL_READ_V, var->ofs, 0, 0);
		else
			PR_EmitOp(OP_LOCAL_READ, var->ofs, 0, 0);
	}
	else
	{
		if (ev->type->kind == ev_vector)
			PR_EmitOp(OP_READ_V, var->ofs, 0, 0);
		else
			PR_EmitOp(OP_READ, var->ofs, 0, 0);
	}
}


static void CODEGEN_FunctionCall(eval_t * ev, bool no_result)
{
	int i;

	// mark beginning of function's stack frame
	PR_EmitOp(OP_FRAME, 0, 0, 0);

	// push parameters, same order as given
	for (i = 0 ; i < ev->num_parms ; i++)
	{
		CODEGEN_Eval(ev->args[1 + i], false);
	}

	// Note: locals are created by code in the function

	// get function ID
	CODEGEN_Eval(ev->args[0], false);

    PR_EmitOp(OP_CALL, ev->num_parms, 0, 0);

	// copy result to stack
	if (! no_result && ev->type->kind != ev_void)
	{
		if (ev->type->kind == ev_vector)
			PR_EmitOp(OP_READ_V, OFS_RETURN, 0, 0);
		else
			PR_EmitOp(OP_READ, OFS_RETURN, 0, 0);
	}
}


static void CODEGEN_FormatString(eval_t * ev)
{
	// mark beginning of function's stack frame
	PR_EmitOp(OP_FRAME, 0, 0, 0);

	// push parameters, same order as given
	CODEGEN_Literal(ev->literal, ev->type);

	for (int i = 0 ; i < ev->num_parms ; i++)
	{
		CODEGEN_Eval(ev->args[i], false);
	}

    PR_EmitOp(OP_CALL_FORMAT, 0, 0, 0);

	// copy result to stack
	PR_EmitOp(OP_READ, OFS_RETURN, 0, 0);
}


static void CODEGEN_FieldAccess(eval_t * ev, bool load_it)
{
	int field_ofs;

	// push entity
	CODEGEN_Eval(ev->args[0], false);

	// get offset -- assumed to be constant!
	field_ofs = G_INT(ev->def->ofs);

	// compute address of field
	PR_EmitOp(OP_ADDRESS, field_ofs, 0, 0);

	if (load_it)
	{
		if (ev->def->type->aux_type->kind == ev_vector)
			PR_EmitOp(OP_LOAD_V, 0, 0, 0);
		else
			PR_EmitOp(OP_LOAD, 0, 0, 0);
	}
}


static void CODEGEN_Assignment(eval_t * ev, bool no_result)
{
	// get assigned value on the stack
	CODEGEN_Eval(ev->args[1], false);

	if (! no_result)
	{
		// duplicate it
		if (ev->type->kind == ev_vector)
		{
			PR_EmitOp(OP_DUP, 3, 0, 0);
			PR_EmitOp(OP_DUP, 3, 0, 0);
			PR_EmitOp(OP_DUP, 3, 0, 0);
		}
		else
			PR_EmitOp(OP_DUP, 1, 0, 0);
	}

	if (ev->args[0]->kind == EV_FIELD_ACCESS)
	{
		CODEGEN_FieldAccess(ev->args[0], false /* load_it */);

		if (ev->type->kind == ev_vector)
			PR_EmitOp(OP_STORE_V, 0, 0, 0);
		else
			PR_EmitOp(OP_STORE, 0, 0, 0);
	}
	else
	{
		SYS_ASSERT(ev->args[0]->kind == EV_VARIABLE);

		def_t * var = ev->args[0]->def;

		if (var->scope)
		{
			if (ev->type->kind == ev_vector)
				PR_EmitOp(OP_LOCAL_WRITE_V, var->ofs, 0, 0);
			else
				PR_EmitOp(OP_LOCAL_WRITE, var->ofs, 0, 0);
		}
		else
		{
			if (ev->type->kind == ev_vector)
				PR_EmitOp(OP_WRITE_V, var->ofs, 0, 0);
			else
				PR_EmitOp(OP_WRITE, var->ofs, 0, 0);
		}
	}
}


static void CODEGEN_ShortCircuit(eval_t *ev)
{
	// Code for (A && B) :
	//
	//		... calc A (as boolean) ...
	//		DUP
	//		IF zero GOTO label
	//      POP
	//		... calc B (as boolean) ...
	//		label:


	// Code for (A || B) :
	//
	//		... calc A (as boolean) ...
	//		DUP
	//		IF non-zero GOTO label
	//      POP
	//		... calc B (as boolean) ...
	//		label:

	dstatement_t *patch;


	CODEGEN_Boolean(ev->args[0]);

	PR_EmitOp(OP_DUP, 1, 0, 0);

	if (ev->op == FAKE_OP_AND)
		patch = PR_EmitOp(OP_IFNOT, 0, 0, 0);
	else
		patch = PR_EmitOp(OP_IF, 0, 0, 0);

	PR_EmitOp(OP_POP, 1, 0, 0);

	CODEGEN_Boolean(ev->args[1]);

	PR_PatchOp(patch);
}


static void CODEGEN_Operator(eval_t * ev)
{
	// handle short-circuit operators && and ||
	if (ev->op == FAKE_OP_AND || ev->op == FAKE_OP_OR)
	{
		CODEGEN_ShortCircuit(ev);
		return;
	}

	CODEGEN_Eval(ev->args[0], false);

	if (ev->kind == EV_BINARY_OP)
		CODEGEN_Eval(ev->args[1], false);
	
	PR_EmitOp(ev->op, 0, 0, 0);

	if (strchr(ev->op_flags, 'n'))  // negated
		PR_EmitOp(OP_NOT_F, 0, 0, 0);
}


static void CODEGEN_TernaryOp(eval_t * ev)
{
	dstatement_t *patch1;
	dstatement_t *patch2;

	// (a) the condition

	CODEGEN_Boolean(ev->args[0]);

	patch1 = PR_EmitOp(OP_IFNOT, 0, 0, 0);

	// (b) code for true part

	CODEGEN_Eval(ev->args[1], false);

	patch2 = PR_EmitOp(OP_GOTO, 0, 0, 0);

	// (c) code for false part

	PR_PatchOp(patch1);

	CODEGEN_Eval(ev->args[2], false);

	// (d) all done
	
	PR_PatchOp(patch2);
}


static void CODEGEN_Eval(eval_t *ev, bool no_result)
{
	if (no_result)
	{
		if (! (ev->kind == EV_FUNC_CALL || ev->kind == EV_ASSIGNMENT))
		{
			PR_ParseError ("expression has no effect");
		}
	}

	switch (ev->kind)
	{
		case EV_LITERAL:
			CODEGEN_Literal(ev->literal, ev->type);
			break;

		case EV_VARIABLE:
			CODEGEN_VarValue(ev);
			break;

		case EV_ASSIGNMENT:
			CODEGEN_Assignment(ev, no_result);
			break;

		case EV_UNARY_OP:
		case EV_BINARY_OP:
			CODEGEN_Operator(ev);
			break;

		case EV_TERNARY_OP:
			CODEGEN_TernaryOp(ev);
			break;

		case EV_FUNC_CALL:
			CODEGEN_FunctionCall(ev, no_result);
			break;

		case EV_FORMAT_STR:
			CODEGEN_FormatString(ev);
			break;

		case EV_FIELD_ACCESS:
			CODEGEN_FieldAccess(ev, true /* load_it */);
			break;

		default:
			PR_ParseError("bad expression");
			break;
	}
}


static void CODEGEN_Boolean(eval_t *ev)
{
	if (ev->type->kind == ev_void)
		PR_ParseError("void conditional expression");

	CODEGEN_Eval(ev, false);

	// check if already a boolean
	if (ev->kind == EV_UNARY_OP || ev->kind == EV_BINARY_OP)
		if (strchr(ev->op_flags, 'b'))
			return;

	switch (ev->type->kind)
	{
		case ev_float:
			PR_EmitOp(OP_BOOL_F, 0, 0, 0);
			break;

		case ev_vector:
			PR_EmitOp(OP_BOOL_V, 0, 0, 0);
			break;

		case ev_string:
			PR_EmitOp(OP_BOOL_S, 0, 0, 0);
			break;

		case ev_entity:
			PR_EmitOp(OP_BOOL_ENT, 0, 0, 0);
			break;

		case ev_function:
			PR_EmitOp(OP_BOOL_FNC, 0, 0, 0);
			break;

		default:
			BugError("unknown type for CODEGEN_Boolean\n");
			break;
	}
}


//========================================


static int trace_ops;


static dstatement_t * PR_EmitOp(int opcode, int a, int b, int c)
{
	if (mpr.numstatements >= MAX_STATEMENTS)
		PR_ParseError("Instruction buffer overflow");

	if (trace_ops)
		fprintf(stdout, "%04X %s a:%d\n", mpr.numstatements, pr_opnames[opcode], a);

	dstatement_t *st = &mpr.statements[mpr.numstatements++];

	st->op = opcode;
	st->a  = a;
	st->b  = b;
	st->c  = c;

	st->linenum = pr_source_line;

	return st;
}


static void PR_EmitKVal(kval_t val)
{
	if (mpr.numstatements >= MAX_STATEMENTS)
		PR_ParseError("Instruction buffer overflow");

	if (trace_ops)
		fprintf(stdout, "%04X KVAL", mpr.numstatements);
	
	dstatement_t *st = &mpr.statements[mpr.numstatements++];

	*(kval_t *)st = val;
}


static void PR_PatchOp(dstatement_t *patch)
{
	patch->a = &mpr.statements[mpr.numstatements] - patch;
}


static void STAT_Return(int ret_line)
{
	eval_t * ev;

	type_t * res_type = pr_scope->def->type->aux_type;

	if (res_type->kind == ev_void)
	{
		PR_Check (";");

		PR_EmitOp(OP_RETURN, 0, 0, 0);
		return;
	}

	// check for missing value
    //
    // Note: we require the value to start on the same line
    //       (otherwise we may parse a func_call/assignment which
    //        was not meant to be the return expression).

    if (pr_source_line > ret_line ||
        pr_token[0] == ';' ||
        pr_token[0] == '}')
    {
        PR_ParseError("return is missing a value");
    }

	ev = EXP_Expression(TOP_PRIORITY);

	// FIXME: check type

	CODEGEN_Eval(ev, false);

	if (ev->type->kind == ev_vector)
		PR_EmitOp(OP_WRITE_V, OFS_RETURN, 0, 0);
	else
		PR_EmitOp(OP_WRITE, OFS_RETURN, 0, 0);
	
	PR_EmitOp(OP_RETURN, 0, 0, 0);

	PR_Check(";");
}


static void STAT_If(void)
{
	eval_t *ev;
	dstatement_t *patch1;
	dstatement_t *patch2;

	PR_Expect("(");
	ev = EXP_Expression(TOP_PRIORITY);
	PR_Expect(")");

	CODEGEN_Boolean(ev);

	patch1 = PR_EmitOp(OP_IFNOT, 0, 0, 0);

	PR_ParseStatement ();
	
	if (PR_Check("else"))
	{
		// use GOTO to skip over the else statements
		patch2 = PR_EmitOp(OP_GOTO, 0, 0, 0);

		PR_PatchOp(patch1);

		PR_ParseStatement ();

		patch1 = patch2;
	}

	PR_PatchOp(patch1);
}


static void STAT_WhileLoop(void)
{
	eval_t *ev;
	dstatement_t *patch1;

	int start = mpr.numstatements;

	PR_Expect ("(");
	ev = EXP_Expression (TOP_PRIORITY);
	PR_Expect (")");

	CODEGEN_Boolean(ev);

	// jump to end when condition is false
	patch1 = PR_EmitOp(OP_IFNOT, 0, 0, 0);

	PR_ParseStatement ();

	PR_EmitOp(OP_GOTO, start - mpr.numstatements, 0, 0);

	// the end

	PR_PatchOp(patch1);
}


static void STAT_RepeatUntil(void)
{
	eval_t *ev;

	int start = mpr.numstatements;

	PR_ParseStatement ();

	PR_Expect("until");
	PR_Expect("(");

	ev = EXP_Expression(TOP_PRIORITY);

	PR_Expect (")");
	PR_Check (";");

	CODEGEN_Boolean(ev);

	PR_EmitOp(OP_IFNOT, start - mpr.numstatements, 0, 0);
}


/*
============
PR_ParseStatement

============
*/
static void PR_ParseStatement (void)
{
	if (PR_Check("{"))
	{
		do
		{
			PR_ParseStatement();

		} while (! PR_Check("}"));

		return;
	}

	int cur_line = pr_source_line;

	if (PR_Check("return"))
	{
		STAT_Return(cur_line);
		return;
	}

	if (PR_Check("while"))
	{
		STAT_WhileLoop();
		return;
	}
	
	if (PR_Check("repeat"))
	{
		STAT_RepeatUntil();
		return;
	}
	
	if (PR_Check("if"))
	{
		STAT_If();
		return;
	}

	if (PR_CheckNameColon())
	{
		PR_ParseLocal ();
		return;
	}

	// handle assignment and function calls

	{
		eval_t * ev = EXP_Expression(TOP_PRIORITY);

		PR_Check(";");

		CODEGEN_Eval(ev, true /* no result */);
	}
}


char * CalcNextStateName(const char * prev)
{
	SYS_ASSERT(prev[0]);

	char buffer[256];

	strcpy(buffer, prev);

	int base_len = strlen(prev);

	while (base_len > 0 && isdigit(prev[base_len-1]))
		base_len--;

	if (! isdigit(prev[base_len]))
	{
		buffer[base_len++] = '1';
		buffer[base_len] = 0;
	}
	else
	{
		int num = atoi(prev + base_len);

		sprintf(buffer + base_len, "%d", num + 1);
	}

	return strdup(buffer);
}


static int PR_ResolveBuiltin(const char *name)
{
	// very first builtin is a dummy -- skip it
	for (int k = 1 ; all_builtins[k].name ; k++)
		if (strcmp(name, all_builtins[k].name) == 0)
			return k;

	PR_ParseError("No such builtin: '%s'", name);
	return 0;  /* NOT REACHED */
}



/*
============
PR_ParseFunctionBody
============
*/
static void PR_ParseFunctionBody(dfunction_t *df, type_t *type, bool is_builtin)
{
	int			i;
	def_t		*defs[MAX_PARMS];
	
//
// check for builtin function definition
//
	df->builtin = 0;

	if (is_builtin)
	{
		PR_Check(";");

		df->builtin = 1;
		df->first_statement = PR_ResolveBuiltin(df->def->name);
		return;
	}

//
// define the parms
//
	for (i=0 ; i < type->num_parms ; i++)
	{
		defs[i] = PR_GetDef (pr_parm_names[i], type->parm_types[i], pr_scope, 2);

		df->parm_ofs[i] = defs[i]->ofs;

		if (i > 0 && df->parm_ofs[i] < df->parm_ofs[i-1])
			BugError ("bad parm order");
	}
	
	df->first_statement = mpr.numstatements;


//
// parse regular statements
//
	PR_Expect ("{");

	while (! PR_Check("}"))
		PR_ParseStatement ();

	// andrewj: optional semicolon after { } block
	PR_Check(";");


// emit an end of statements opcode
	PR_EmitOp(OP_DONE, 0, 0, 0);
}

/*
============
PR_GetDef

If type is NULL, it will match any type
If allocate is true, a new def will be allocated if it can't be found
============
*/
static def_t * PR_GetDef (const char *name, type_t *type, dfunction_t *scope, int allocate)
{
	def_t	* def;

	char element[MAX_NAME];

// see if the name is already in use
	for (def = all_defs ; def ; def = def->next)
	{
		if (strcmp(def->name,name) != 0)
			continue;

		if ( def->scope && def->scope != scope)
			continue;		// in a different function

		if (type && def->type != type)
			PR_ParseError ("Type mismatch on redeclaration of %s",name);

//???			// move to head of list to find fast next time
//???			*old = def->search_next;
//???			def->search_next = pr2.search;
//???			pr2.search = def;

		return def;
	}

	if (!allocate)
		return NULL;
		
// allocate a new def
	def = new def_t;

	memset (def, 0, sizeof(*def));

	def->next = all_defs;
	all_defs  = def;

	def->name = strdup(name);
	def->type = type;

	def->scope = scope;

	if (allocate == 2)
		def->is_param = 1;

// andrewj: local variables
	if (scope)
		def->ofs = pr_local_ofs;
	else
		def->ofs = mpr.numregisters;

//
// make automatic defs for the vectors elements
// .origin can be accessed as .origin_x, .origin_y, and .origin_z
//
	if (type->kind == ev_vector)
	{		
		sprintf (element, "%s_x",name);
		PR_GetDef (element, &type_float, scope, allocate);
		
		sprintf (element, "%s_y",name);
		PR_GetDef (element, &type_float, scope, allocate);
		
		sprintf (element, "%s_z",name);
		PR_GetDef (element, &type_float, scope, allocate);
	}
	else if (scope)
		pr_local_ofs += type_size[type->kind];
	else
		mpr.numregisters += type_size[type->kind];


	if (type->kind == ev_field)
	{
		G_INT(def->ofs) = 123; /// pr2.size_fields;

		if (type->aux_type->kind == ev_vector)
		{
			sprintf (element, "%s_x",name);
			PR_GetDef (element, &type_floatfield, scope, allocate);
			
			sprintf (element, "%s_y",name);
			PR_GetDef (element, &type_floatfield, scope, allocate);
			
			sprintf (element, "%s_z",name);
			PR_GetDef (element, &type_floatfield, scope, allocate);
		}
		else
		{
		//???	pr2.size_fields += type_size[type->aux_type->kind];
		}
	}

//	if (pr_dumpasm)
//		PR_PrintOfs (def->ofs);
		
	return def;
}


/*
================
PR_ParseGlobals

Called at the outer layer
================
*/
void PR_ParseGlobals (void)
{
	char		*name;
	type_t		*type;
	def_t		*def;
	dfunction_t	*df;

	int			i;

// field definition ?

	if (PR_Check("."))
	{
		name = strdup(PR_ParseName());

		PR_Expect(":");

		type = PR_ParseFieldType ();

		def = PR_GetDef (name, type, NULL, 1);

		PR_Check(";");
		return;
	}


	bool is_builtin = PR_Check("builtin");
	bool is_forward = PR_Check("forward");


	name = strdup(PR_ParseName());


// constants
	
	if (PR_Check(":="))
	{
		def = PR_GetDef (name, pr_immediate_type, NULL, 1);

		if (def->initialized)
			PR_ParseError ("%s redeclared", name);

		def->initialized = 1;

		switch (pr_immediate_type->kind)
		{
			case ev_float:
				G_FLOAT(def->ofs) = pr_immediate_float[0];
				break;
			
			case ev_vector:
				G_FLOAT(def->ofs + 0) = pr_immediate_float[0];
				G_FLOAT(def->ofs + 1) = pr_immediate_float[1];
				G_FLOAT(def->ofs + 2) = pr_immediate_float[2];
				break;

			case ev_string:
				G_STRING(def->ofs) = GlobalizeString(pr_immediate_string);
				break;

			default:
				PR_ParseError("weird value for constant");
				/* NOT REACHED */
		}

		PR_Lex();

		PR_Check(";");
		return;
	}


// variables

	if (! PR_Check("("))
	{
		PR_Expect(":");

		type = PR_ParseType();

		def = PR_GetDef (name, type, NULL, 1);

		if (type->kind == ev_string)
			G_STRING(def->ofs) = empty_str;

		PR_Check (";");
		return;
	}

	
// functions
	
	type = PR_ParseAltFuncType(true /* seen_first_bracket */);

	def = PR_GetDef (name, type, NULL, 1);


// check for an initialization
	if (is_forward)
	{
		PR_Check(";");
		return;
	}

	if (def->initialized)
		PR_ParseError ("%s redeclared", name);


	// functions with bodies

	G_FUNCTION(def->ofs) = mpr.numfunctions;

	df = &mpr.functions[mpr.numfunctions++];

	df->def = def;


// fill in the dfunction
	df->parm_start = mpr.numregisters;
	df->numparms = df->def->type->num_parms;

	for (i=0 ; i < df->numparms ; i++)
		df->parm_size[i] = type_size[df->def->type->parm_types[i]->kind];

	df->filename = StringDup(pr_source_file);


	pr_scope = df;
	pr_local_ofs = 0;
	{
		PR_ParseFunctionBody (df, type, is_builtin);
	}
	pr_scope = NULL;

	def->initialized = 1;

//	if (pr_dumpasm)
//		PR_PrintFunction (def);
}


static void PR_ParseLocal (void)
{
	char		*name;
	type_t		*type;
	def_t		*def;

	name = PR_ParseName();

	PR_Expect(":");


	type = PR_ParseType ();

	def = PR_GetDef (name, type, pr_scope, 1);


	// generate code to set its value

	if (PR_Check("="))
	{
		eval_t *e = EXP_Expression(TOP_PRIORITY);

		if (e->type->kind != type->kind)
			PR_ParseError("type mismatch for local initializer");

		CODEGEN_Eval(e, false);
	}
	else if (type->kind == ev_string)
	{
		kval_t  s;

		s._string = empty_str;

		CODEGEN_Literal(&s, type);
	}
	else
	{
		kval_t  literal[3];

		memset(literal, 0, sizeof(literal));

		CODEGEN_Literal(literal, type);
	}

	PR_Check (";");
}


/*
============
PR_CompileFile

compiles the 0 terminated text, adding defintions to the pr2 structure
============
*/
bool PR_CompileFile (char *string, const char *filename)
{	
	pr_file_p = string;

	pr_source_file = filename;
	pr_source_line = 0;
	pr_scope = NULL;

	if (! empty_str)
	{
		empty_str = object_ref_c::NewString("");
		empty_str->MakePermanent();
	}

	PR_NewLine ();

	PR_Lex ();	// read first token

	while (pr_token_type != tt_eof)
	{
		if (setjmp(pr_parse_abort))
		{
			if (++pr_error_count > MAX_ERRORS)
				return false;
			PR_SkipToSemicolon ();
			if (pr_token_type == tt_eof)
				return false;		
		}

		pr_scope = NULL;	// outside all functions
		
		PR_ParseGlobals ();
	}
	
	return (pr_error_count == 0);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
