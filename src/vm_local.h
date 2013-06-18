//----------------------------------------------------------------------
//  VM : LOCAL API
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

#ifndef __VM_LOCAL_H__
#define __VM_LOCAL_H__

#include <setjmp.h>

#include "vm_object.h"

typedef int	func_t;
typedef int	string_t;

class edict_t;


typedef union
{
	float			_float;
	int				_int;
	object_ref_c *	_string;
	func_t			_func;
	int				_edict;
} kval_t;


typedef enum
{
	ev_void,
	ev_string,
	ev_float,
	ev_vector,
	ev_entity,
	ev_field,
	ev_function,
	ev_pointer

} etype_t;


#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28


typedef enum
{
	OP_DONE,

	OP_MUL_F,
	OP_DIV_F,
	OP_ADD_F,
	OP_SUB_F,

	OP_EQ_F,
	OP_EQ_S,
	OP_EQ_ENT,
	OP_EQ_FNC,
	
	OP_LT,
	OP_GT,

	OP_NOT_F,

	OP_BOOL_F,    // convert value to 0 or 1
	OP_BOOL_V,
	OP_BOOL_S,
	OP_BOOL_ENT,
	OP_BOOL_FNC,

	OP_BITAND,
	OP_BITOR,
	OP_BITXOR,
	OP_BITNOT,

	OP_IF,       // require a 'boolean' (_int) on the stack
	OP_IFNOT,    //

	OP_GOTO,

	OP_FRAME,    // prepare for call, mark start of stack frame
	OP_CALL,
	OP_CALL_FORMAT,
	OP_RETURN,

	OP_ADDRESS,
	OP_LOAD,
	OP_LOAD_V,
	OP_STORE,
	OP_STORE_V,

	// andrewj: new or transitional stuff

	OP_DUP,		// duplicate a stack item (a is offset)
	OP_POP,     // pop values off stack (a is count)

	OP_MOD_F,
	OP_POWER,

	OP_LITERAL,		// push a 32-bit literal value

	OP_READ,
	OP_READ_V,

	OP_WRITE,
	OP_WRITE_V,

	OP_LOCAL_READ,
	OP_LOCAL_READ_V,
	OP_LOCAL_WRITE,
	OP_LOCAL_WRITE_V,

//--------- VECTOR OPS ----------//

    OP_VEC_ADD,     // add two vectors on stack
    OP_VEC_SUB,     // subtract vectors (second minus top)

    OP_VEC_PROD,    // dot product of two vectors, result is float

    OP_VEC_MUL_F,   // multiply vector by float at top of stack
    OP_VEC_DIV_F,   // divide vector by float at top of stack

    OP_VEC_EQ       // compare two vectors for equality

} op_kind_e;


typedef struct statement_s
{
	unsigned short	op;
	short	a,b,c;
	int  linenum;
} dstatement_t;

typedef struct
{
	unsigned short	type;		// if DEF_SAVEGLOBAL bit is set
								// the variable needs to be saved in savegames
	unsigned short	ofs;
	int			s_name;
} globaldef_t;

typedef struct
{
	unsigned short	type;
	unsigned short	ofs;
	int			s_name;
} fielddef_t;

#define	DEF_SAVEGLOBAL	(1<<15)

#define	MAX_PARMS	8

typedef struct
{
    int                 builtin;    // if non 0, call an internal function

    struct def_s        *def;   // has name (etc)

    int                 parm_ofs[MAX_PARMS];    // allways contiguous, right?


	int		first_statement;

	int		parm_start;
	
	int		profile;		// runtime
	
	int		s_name;
	int		s_file;			// source file defined in

	int		numparms;
	byte	parm_size[MAX_PARMS];

} dfunction_t;


//============================================================================

typedef int	gofs_t;				// offset in global data block

#define	MAX_PARMS	8

typedef struct type_s
{
	etype_t			kind;
	struct type_s	*next;
// function types are more complex
	struct type_s	*aux_type;	// return type or field type
	int				num_parms;	// -1 = variable args
	struct type_s	*parm_types[MAX_PARMS];	// only [num_parms] allocated
} type_t;

typedef struct def_s
{
	type_t		*type;
	char		*name;
	dfunction_t	*scope;		// function the var/param was defined in, or NULL

	gofs_t		ofs;
	int			initialized;	// 1 for constants or functions with a body or builtins
	int			is_param;  // TEMPORARY

	struct def_s	*next;
	struct def_s	*search_next;	// for finding faster
} def_t;

// pr_loc.h -- program local defs

#define	MAX_ERRORS		10

#define	MAX_NAME		64		// chars long

#define	MAX_REGS		16384


extern	int		type_size[8];

extern	type_t	type_void, type_string, type_float, type_vector, type_entity, type_field, type_function, type_pointer, type_floatfield;


//============================================================================

// vm_lex

typedef enum
{
	tt_eof,			// end of file reached
	tt_name, 		// an alphanumeric name token
	tt_punct, 		// code punctuation
	tt_immediate,	// string, float, vector

} token_type_t;


extern	bool	pr_dumpasm;


extern	char			pr_token[2048];
extern	token_type_t	pr_token_type;

extern	type_t		*pr_immediate_type;
extern  float		 pr_immediate_float[3];
extern	char		 pr_immediate_string[2048];

extern  char  pr_parm_names[MAX_PARMS][MAX_NAME];


void PR_PrintStatement (dstatement_t *s);

// reads the next token into pr_token and classifies its type
void PR_Lex (void);

void PR_NewLine (void);
void PR_SkipToSemicolon (void);

char *PR_ParseName (void);

type_t *PR_ParseType (void);
type_t *PR_ParseFieldType (void);
type_t *PR_ParseAltFuncType (bool seen_first_bracket);

bool PR_Check (const char *string);
bool PR_CheckNameColon (void);
void PR_Expect (const char *string);
void PR_ParseError (const char *error, ...);


extern	jmp_buf		pr_parse_abort;		// longjump with this on parse error
extern	int			pr_source_line;
extern	char		*pr_file_p;


// vm_compile

bool PR_CompileFile (char *string, const char *filename);


//=============================================================================

#define	MAX_STRINGS		500000
#define	MAX_GLOBALS		16384
#define	MAX_FIELDS		1024
#define	MAX_STATEMENTS	65536
#define	MAX_FUNCTIONS	8192

#define	MAX_SOUNDS		1024
#define	MAX_MODELS		1024
#define	MAX_FILES		1024
#define	MAX_DATA_PATH	64

typedef struct
{
	char		strings[MAX_STRINGS];
	int			strofs;

	dstatement_t	statements[MAX_STATEMENTS];
	int			numstatements;

	dfunction_t	functions[MAX_FUNCTIONS];
	int			numfunctions;

	globaldef_t	globals[MAX_GLOBALS];
	int			numglobaldefs;

	fielddef_t	fields[MAX_FIELDS];
	int			numfielddefs;

	kval_t		registers[MAX_REGS];
	int			numregisters;
}
mem_progs_t;

extern  mem_progs_t  mpr;

#define	G_FLOAT(o)    (mpr.registers[o]._float)
#define	G_INT(o)      (mpr.registers[o]._int)
#define	G_STRING(o)   (mpr.registers[o]._string)
#define	G_FUNCTION(o) (mpr.registers[o]._func)


//============================================================================

// vm_builtin

typedef struct
{
	const char *name;

	void (*func) (void);

} builtin_t;

extern builtin_t  all_builtins[];


//============================================================================

// vm_execute

#define	MAX_FIELD_LEN	64

#define MAX_CALL_STACK  256

#define MAX_LOCAL_STACK  2048

typedef struct
{
	int  ip;

	const dfunction_t  *func;

	int frame;

} call_stack_t;


typedef struct
{
	bool  trace;

	int		argc;

	const dfunction_t	* x_func;
	int					  x_ip;

	// call stack
	call_stack_t  call_stack[MAX_CALL_STACK];
	int           call_depth;

	// stack for local variables (including parameters)
	kval_t	stack[MAX_LOCAL_STACK];
	int		stack_top;
	int		frame;  // start for current function
	int     next_frame;  // start for about-to-be-called func

} execution_info_t;


extern execution_info_t  exec;


void PR_RunError (const char *error, ...);


int          PR_Param_Int(int offset);
float        PR_Param_Float(int offset);
const char * PR_Param_String(int offset);
void         PR_Param_Vector(int offset, float *vec);
edict_t *    PR_Param_Entity(int offset);
int          PR_Param_EdictNum(int offset);


void PR_Return_Int(int val);
void PR_Return_Float(float val);
void PR_Return_Vector(float *val);
void PR_Return_String(const char *str);
void PR_Return_Entity(edict_t * ent);


#endif /* __VM_LOCAL_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
