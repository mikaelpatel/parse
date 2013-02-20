/* $Id: parse.h 5 2009-08-21 21:47:38Z Mikael Patel $
 * ----------------------------------------------------------------------
 * Name
 *   parse - top down parse machine
 *  
 * Description:	
 *   General back-tracking top down parse machine. Supports meta grammar
 *   with terminal matching, symmetric non-terminals, zero or one, zero
 *   or many, and one or many non-terminals. Additional support for
 *   parse cutting and error signalling.
 *
 * Copyright (C) 1984-1994, 2013, Mikael Patel.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA  02111-1307  USA
 * ----------------------------------------------------------------------
 */

#if !defined(PARSE_H)
#define PARSE_H

#if !defined(TRUE)
#define TRUE (1)
#define FALSE (0)
#endif

typedef struct TERM TERM;
typedef struct TERM *PRODUCT;
typedef struct SYMBOL SYMBOL;
typedef struct SYMBOL *DICTIONARY;
typedef struct STRING STRING;
typedef struct VALUE VALUE;
typedef struct ENVIRONMENT ENVIRONMENT;
typedef void (*SEMANTIC)(ENVIRONMENT*);
typedef int (*PARSE)(SYMBOL*, char**, VALUE**);

struct STRING {
  int count;
  char *buffer;
};

typedef enum {
  VALUE_UNKNOWN_TYPE,
  VALUE_UNDEFINED_TYPE,
  VALUE_PTR_TYPE,
  VALUE_STR_TYPE,
  VALUE_LONG_TYPE,
  VALUE_DOUBLE_TYPE,
  VALUE_STRING_TYPE,
  VALUE_SYMBOL_TYPE,
  VALUE_TYPE_TYPE
} VALUE_TYPE;

struct VALUE {
  VALUE_TYPE type;
  union {
    void *as_ptr;
    char *as_str;
    long as_long;
    double as_double;
    STRING as_string;
    SYMBOL *as_symbol;
    VALUE_TYPE as_type;
  } view;
};

struct ENVIRONMENT {
  VALUE *sp;
  VALUE *ip;
  void *ep;
};

struct SYMBOL {
  SYMBOL *next;
  char *name;
  int id;
  PRODUCT *syntax;
  PARSE parse;
  SEMANTIC semantic;
};

typedef enum {
  TERM_TERMINAL_TYPE,
  TERM_NON_TERMINAL_TYPE,
  TERM_ZERO_OR_ONE_TYPE,
  TERM_ZERO_OR_MANY_TYPE,
  TERM_ONE_OR_MANY_TYPE,
  TERM_PRODUCT_END_TYPE	
} TERM_TYPE;

struct TERM {
  TERM_TYPE type;
  SYMBOL *symbol;
};


/* String print function */
void string_print(STRING *str);

/* Universal value print function and binding */
void value_print(VALUE *value);
void value_bind(VALUE *value, VALUE **output);
#define value_tos(env, v) (v = env->sp)
#define value_push(env,v) (env->sp++, *env->sp = *v)
#define value_pop(env,v) (v = env->sp, env->sp--, v) 

/* Symbol functions */
SYMBOL *symbol_lookup(char *name, int *id, int append, DICTIONARY *dictionary);
void symbol_print_name(SYMBOL *symbol);
void symbol_bind(SYMBOL *symbol, VALUE **output);

/* Parse machine variables and functions */
extern SYMBOL *parse_error_symbol;
extern char *parse_error_input;
extern int parse_tracing;
extern int parse_timing;
extern int parse_warning;
extern int parse_indent;
extern int parse_cutting;
extern int parse_executing;

/* Execute result of parse and error function */
void parse_execute(ENVIRONMENT *env);
void parse_error(void);

/* Parse functions */
int parse_symbol(SYMBOL *symbol, char **input, VALUE **output);
int parse_syntax(SYMBOL *symbol, char **input, VALUE **output);
int parse_undefined(SYMBOL *symbol, char **input, VALUE **output);
int parse_empty(SYMBOL *symbol, char **input, VALUE **output);
int parse_eoln(SYMBOL *symbol, char **input, VALUE **output);
int parse_integer(SYMBOL *symbol, char **input, VALUE **output);
int parse_float(SYMBOL *symbol, char **input, VALUE **output);
int parse_string(SYMBOL *symbol, char **input, VALUE **output);
int parse_identifier(SYMBOL *symbol, char **input, VALUE **output);
int parse_token(SYMBOL *symbol, char **input, VALUE **output);
int parse_nospace(SYMBOL *symbol, char **input, VALUE **output);
int parse_space(SYMBOL *symbol, char **input, VALUE **output);
int parse_cut(SYMBOL *symbol, char **input, VALUE **output);
int parse_throw(SYMBOL *symbol, char **input, VALUE **output);
int parse_run(SYMBOL *symbol, char **input, VALUE **output);
int parse_pos(SYMBOL *symbol, char **input, VALUE **output);

/* Top level parse function */
int parse_input(SYMBOL *symbol, char **input, VALUE **output);

/* Primitive semantic action on values */
extern void semantic_value_add(ENVIRONMENT*);
extern void semantic_value_sub(ENVIRONMENT*);
extern void semantic_value_mul(ENVIRONMENT*);
extern void semantic_value_div(ENVIRONMENT*);
extern void semantic_value_mod(ENVIRONMENT*);
extern void semantic_value_typeof(ENVIRONMENT*);
extern void semantic_value_asinteger(ENVIRONMENT*);
extern void semantic_value_asfloat(ENVIRONMENT*);
extern void semantic_value_print(ENVIRONMENT*);

/* Primitive grammar symbols */
extern SYMBOL symbol_identifier;
extern SYMBOL symbol_token;
extern SYMBOL symbol_empty;
extern SYMBOL symbol_eoln;
extern SYMBOL symbol_integer;
extern SYMBOL symbol_float;
extern SYMBOL symbol_string;
extern SYMBOL symbol_space;
extern SYMBOL symbol_nospace;
extern SYMBOL symbol_execute;
extern SYMBOL symbol_pos;
extern SYMBOL symbol_cut;
extern SYMBOL symbol_error;
extern SYMBOL symbol_value_add;
extern SYMBOL symbol_value_sub;
extern SYMBOL symbol_value_mul;
extern SYMBOL symbol_value_div;
extern SYMBOL symbol_value_mod;
extern SYMBOL symbol_value_typeof;
extern SYMBOL symbol_value_asinteger;
extern SYMBOL symbol_value_asfloat;
extern SYMBOL symbol_value_print;
#define PARSE_LAST_SYMBOL symbol_value_print

/* Primitive dictionary */
extern DICTIONARY parse_dictionary;

#endif /* PARSE_H */


