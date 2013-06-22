//----------------------------------------------------------------------
//  VM : LEXING
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


const char *pr_source_file = "XXX";
int			pr_source_line;

char		*pr_file_p;
char		*pr_line_start;		// start of current source line

int			pr_bracelevel;

char			pr_token[2048];
token_type_t	pr_token_type;

type_t		*pr_immediate_type;
float		 pr_immediate_float[3];
char		 pr_immediate_string[2048];

int		pr_error_count;

const char	*pr_punctuation[] =
// longer symbols must be before a shorter partial match
{"&&", "||", "<=", ">=","==", "!=", "...", ":=", "^^",
 ":", ";", ",", "!", "*", "/", "%", "-", "+", "=", "^", "<", ">", 
 "(", ")", "[", "]", "{", "}", ".", "#", "&", "|", "@", "~", "?",
 NULL};

// simple types.  function types are dynamically allocated
type_t	type_void = { ev_void };
type_t	type_nil  = { ev_nil };
type_t	type_string = { ev_string };
type_t	type_float  = { ev_float };
type_t	type_vector = { ev_vector };
type_t	type_entity = { ev_entity };
type_t	type_function = { ev_function, &type_void, 0 };
type_t	type_pointer = { ev_pointer };

type_t  type_linedef = { ev_linedef };
type_t  type_sidedef = { ev_sidedef };
type_t  type_sector  = { ev_sector };
type_t  type_thing   = { ev_thing };
type_t  type_vertex  = { ev_vertex };

type_t  type_set_linedef = { ev_set, &type_linedef, 0 };
type_t  type_set_sidedef = { ev_set, &type_sidedef, 0 };
type_t  type_set_sector  = { ev_set, &type_sector,  0 };
type_t  type_set_thing   = { ev_set, &type_thing,   0 };
type_t  type_set_vertex  = { ev_set, &type_vertex,  0 };

static type_t * custom_types;

void PR_LexWhitespace (void);


/*
==============
PR_PrintNextLine
==============
*/
void PR_PrintNextLine (void)
{
	char	*t;

	printf ("%3i:",pr_source_line);
	for (t=pr_line_start ; *t && *t != '\n' ; t++)
		printf ("%c",*t);
	printf ("\n");
}

/*
==============
PR_NewLine

Call at start of file and when *pr_file_p == '\n'
==============
*/
void PR_NewLine (void)
{
	bool	m;
	
	if (*pr_file_p == '\n')
	{
		pr_file_p++;
		m = true;
	}
	else
		m = false;

	pr_source_line++;
	pr_line_start = pr_file_p;

//	if (pr_dumpasm)
//		PR_PrintNextLine ();
	if (m)
		pr_file_p--;
}

/*
==============
PR_LexString

Parses a quoted string
==============
*/
void PR_LexString (void)
{
	int		c;
	int		len;
	
	len = 0;
	pr_file_p++;
	do
	{
		c = *pr_file_p++;
		if (!c)
			PR_ParseError ("EOF inside quote");
		if (c=='\n')
			PR_ParseError ("newline inside quote");
		if (c=='\\')
		{	// escape char
			c = *pr_file_p++;
			if (!c)
				PR_ParseError ("EOF inside quote");
			if (c == 'n')
				c = '\n';
			else if (c == '"')
				c = '"';
			else if (isdigit(c))
			{
				// andrewj: support octal notation (as per C)

				// one, two or three octal digits
				c = (c - '0');

				if (isdigit(*pr_file_p))
					c = c * 8 + (*pr_file_p++ - '0');

				if (isdigit(*pr_file_p))
					c = c * 8 + (*pr_file_p++ - '0');
			}
			else
				PR_ParseError ("Unknown escape char");
		}
		else if (c=='\"')
		{
			pr_token[len] = 0;
			pr_token_type = tt_immediate;
			pr_immediate_type = &type_string;
			strcpy (pr_immediate_string, pr_token);
			return;
		}
		pr_token[len] = c;
		len++;
	} while (1);
}

/*
==============
PR_LexNumber
==============
*/
float PR_LexNumber (void)
{
	int		c;
	int		len;
	
	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ((c >= '0' && c<= '9') || c == '.');
	pr_token[len] = 0;
	return atof (pr_token);
}

/*
==============
PR_LexVector

Parses a single quoted vector
==============
*/
void PR_LexVector (void)
{
	int		i;
	
	pr_file_p++;
	pr_token_type = tt_immediate;
	pr_immediate_type = &type_vector;
	for (i=0 ; i<3 ; i++)
	{
		pr_immediate_float[i] = PR_LexNumber ();
		PR_LexWhitespace ();
	}
	if (*pr_file_p != '\'')
		PR_ParseError ("Bad vector");
	pr_file_p++;
}

/*
==============
PR_LexName

Parses an identifier
==============
*/
void PR_LexName (void)
{
	int		c;
	int		len;
	
	len = 0;
	c = *pr_file_p;
	do
	{
		pr_token[len] = c;
		len++;
		pr_file_p++;
		c = *pr_file_p;
	} while ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' 
	|| (c >= '0' && c <= '9'));
	pr_token[len] = 0;
	pr_token_type = tt_name;
}

/*
==============
PR_LexPunctuation
==============
*/
void PR_LexPunctuation (void)
{
	int		i;
	int		len;
	const char	*p;
	
	pr_token_type = tt_punct;
	
	for (i=0 ; (p = pr_punctuation[i]) != NULL ; i++)
	{
		len = strlen(p);
		if (!strncmp(p, pr_file_p, len) )
		{
			strcpy (pr_token, p);
			if (p[0] == '{')
				pr_bracelevel++;
			else if (p[0] == '}')
				pr_bracelevel--;
			pr_file_p += len;
			return;
		}
	}
	
	PR_ParseError ("Unknown punctuation");
}

		
/*
==============
PR_LexWhitespace
==============
*/
void PR_LexWhitespace (void)
{
	int		c;
	
	while (1)
	{
	// skip whitespace
		while ( (c = *pr_file_p) <= ' ')
		{
			if (c=='\n')
				PR_NewLine ();
			if (c == 0)
				return;		// end of file
			pr_file_p++;
		}
		
	// skip // comments
		if (c=='/' && pr_file_p[1] == '/')
		{
			while (*pr_file_p && *pr_file_p != '\n')
				pr_file_p++;
			PR_NewLine();
			pr_file_p++;
			continue;
		}
		
	// skip /* */ comments
		if (c=='/' && pr_file_p[1] == '*')
		{
			do
			{
				pr_file_p++;
				if (pr_file_p[0]=='\n')
					PR_NewLine();
				if (pr_file_p[1] == 0)
					return;
			} while (pr_file_p[-1] != '*' || pr_file_p[0] != '/');
			pr_file_p++;
			continue;
		}
		
		break;		// a real character has been found
	}
}


//============================================================================

/*
==============
PR_Lex

Sets pr_token, pr_token_type, and possibly pr_immediate and pr_immediate_type
==============
*/
void PR_Lex (void)
{
	int		c;

	pr_token[0] = 0;
	
	if (!pr_file_p)
	{
		pr_token_type = tt_eof;
		return;
	}

	PR_LexWhitespace ();

	c = *pr_file_p;
		
	if (!c)
	{
		pr_token_type = tt_eof;
		return;
	}

// handle quoted strings as a unit
	if (c == '\"')
	{
		PR_LexString ();
		return;
	}

// handle quoted vectors as a unit
	if (c == '\'')
	{
		PR_LexVector ();
		return;
	}

// if the first character is a valid identifier, parse until a non-id
// character is reached
	if ( (c >= '0' && c <= '9') || ( c=='-' && pr_file_p[1]>='0' && pr_file_p[1] <='9') )
	{
		pr_token_type = tt_immediate;
		pr_immediate_type = &type_float;
		pr_immediate_float[0] = PR_LexNumber ();
		return;
	}
	
	if ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' )
	{
		PR_LexName ();
		return;
	}
	
// parse symbol strings until a non-symbol is found
	PR_LexPunctuation ();
}

//=============================================================================

/*
============
PR_ParseError

Aborts the current file load
============
*/
void PR_ParseError (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsprintf (string,error,argptr);
	va_end (argptr);

	printf ("%s:%i: %s\n", pr_source_file, pr_source_line, string);

	longjmp (pr_parse_abort, 1);
}


/*
=============
PR_Expect

Issues an error if the current token isn't equal to string
Gets the next token
=============
*/
void PR_Expect (const char *string)
{
	if (strcmp (string, pr_token))
		PR_ParseError ("expected %s found %s",string, pr_token);
	PR_Lex ();
}


/*
=============
PR_Check

Returns true and gets the next token if the current token equals string
Returns false and does nothing otherwise
=============
*/
bool PR_Check (const char *string)
{
	if (strcmp (string, pr_token))
		return false;
		
	PR_Lex ();
	return true;
}


// andrewj: check next token is a name followed by ':'
bool PR_CheckNameColon (void)
{
	char *p;

	if (pr_token_type != tt_name)
		return false;
	
	for (p = pr_file_p; *p ; p++)
	{
		if (*p <= ' ')
			continue;

		return (*p == ':');
	}

	return false;  // EOF
}


/*
============
PR_ParseName

Checks to see if the current token is a valid name
============
*/
char *PR_ParseName (void)
{
	static char	ident[MAX_NAME];
	
	if (pr_token_type != tt_name)
		PR_ParseError ("not a name");
	if (strlen(pr_token) >= MAX_NAME-1)
		PR_ParseError ("name too long");
	strcpy (ident, pr_token);
	PR_Lex ();
	
	return ident;
}

/*
============
PR_FindType

Returns a preexisting complex type that matches the parm, or allocates
a new one and copies it out.
============
*/
type_t *PR_FindType (type_t *type)
{
	type_t	*check;
	int		i;
	
	for (check = custom_types ; check ; check = check->next)
	{
		if (check->kind != type->kind
		|| check->aux_type != type->aux_type
		|| check->num_parms != type->num_parms)
			continue;
	
		for (i=0 ; i< type->num_parms ; i++)
			if (check->parm_types[i] != type->parm_types[i])
				break;
			
		if (i == type->num_parms)
			return check;	
	}

// allocate a new one
	check = new type_t;
	
	*check = *type;

	check->next = custom_types;
	custom_types = check;

	return check;
}


/*
============
PR_SkipToSemicolon

For error recovery, also pops out of nested braces
============
*/
void PR_SkipToSemicolon (void)
{
	do
	{
		if (!pr_bracelevel && PR_Check (";"))
			return;
		PR_Lex ();
	} while (pr_token[0]);	// eof will return a null token
}


static type_t * PR_ParseSetType()
{
	PR_Expect("[");

	type_t * result;

	if (PR_Check("linedef"))
		result = &type_set_linedef;
	else if (PR_Check("sidedef"))
		result = &type_set_sidedef;
	else if (PR_Check("sector"))
		result = &type_set_sector;
	else if (PR_Check("thing"))
		result = &type_set_thing;
	else if (PR_Check("vertex"))
		result = &type_set_vertex;
	else
	{
		PR_ParseError ("invalid set type");
		return NULL;
	}

	PR_Expect("]");

	return result;
}


/*
============
PR_ParseType

Parses a variable type, including field and functions types
============
*/
char	pr_parm_names[MAX_PARMS][MAX_NAME];

type_t *PR_ParseType (void)
{
	type_t *type;

	if (PR_Check ("."))
		PR_ParseError("unexpected field syntax");

	if (PR_Check ("function"))
		return PR_ParseAltFuncType(false);

	if (PR_Check ("set"))
		return PR_ParseSetType();

	if (!strcmp (pr_token, "float") )
		type = &type_float;
	else if (!strcmp (pr_token, "vector") )
		type = &type_vector;
	else if (!strcmp (pr_token, "entity") )
		type = &type_entity;
	else if (!strcmp (pr_token, "string") )
		type = &type_string;

	else if (!strcmp (pr_token, "linedef") )
		type = &type_linedef;
	else if (!strcmp (pr_token, "sidedef") )
		type = &type_sidedef;
	else if (!strcmp (pr_token, "sector") )
		type = &type_sector;
	else if (!strcmp (pr_token, "thing") )
		type = &type_thing;
	else if (!strcmp (pr_token, "vertex") )
		type = &type_vertex;
	else
	{
		PR_ParseError ("\"%s\" is not a type", pr_token);
		return NULL;	// shut up compiler warning
	}
	PR_Lex ();

	if (PR_Check("("))
		PR_ParseError("WTF bad type syntax");

	return type;

}


type_t * PR_ParseAltFuncType (bool seen_first_bracket)
{
	type_t	newbie;
	type_t	*type;
	char	*name;
	
	if (! seen_first_bracket)
		PR_Expect("(");

// function type
	memset (&newbie, 0, sizeof(newbie));

	newbie.kind = ev_function;
	newbie.num_parms = 0;

	if (! PR_Check (")"))
	{
		do
		{
			if (newbie.num_parms >= MAX_PARMS)
				PR_ParseError("too many parameters for function (%d is limit)", MAX_PARMS);

			name = PR_ParseName ();

			PR_Expect(":");

			type = PR_ParseType ();

			strcpy (pr_parm_names[newbie.num_parms], name);
			newbie.parm_types[newbie.num_parms] = type;
			newbie.num_parms++;

		} while (PR_Check (","));
	
		PR_Expect (")");
	}

// return type
	if (PR_Check(":"))
		newbie.aux_type = PR_ParseType();
	else
		newbie.aux_type = &type_void;

	return PR_FindType (&newbie);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
