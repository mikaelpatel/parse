/* $Id: parse.c 5 2009-08-21 21:47:38Z Mikael Patel $
 * ----------------------------------------------------------------------
 * Name
 *   parse - top down parse machine
 *  
 * Description:	
 *   General back-tracking top down parse machine. Supports meta grammar
 *   with terminal matching, symmetric non-terminals, zero or one, zero
 *   or many, and one or many non-terminals.
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

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <time.h>

#include "parse.h"
#include "bnf.h"

#define CLK_TCK CLOCKS_PER_SEC

/* 
 * ----------------------------------------------------------------------
 * Section: String (pointer and byte count)
 * ----------------------------------------------------------------------
 */

void string_print(STRING *str)
{
  char *s;
  int n;
  if (str == NULL)
    return;
  s = str->buffer;
  n = str->count;
  while (n--)
    putchar(*s++);
}

/* 
 * ----------------------------------------------------------------------
 * Section: Generic Value (pointer, string, integer (long), float, etc.
 * ----------------------------------------------------------------------
 */

char *value_type2str(VALUE_TYPE type)
{
  static char *type2str[] = {
    "unknown",
    "undefined",
    "any",
    "null-terminated string",
    "integer",
    "float",
    "string",
    "symbol",
    "type"
  };

  if (type < VALUE_UNDEFINED_TYPE && type > VALUE_TYPE_TYPE)
    type = VALUE_UNKNOWN_TYPE;

  return (type2str[type]);
}

void value_print(VALUE *v)
{
  switch (v->type) {
    case VALUE_TYPE_TYPE:
      printf("%s", value_type2str(v->view.as_type));
      break;
    case VALUE_PTR_TYPE:
      printf("%p", v->view.as_ptr);
      break;
    case VALUE_STR_TYPE:  
      printf("%s", v->view.as_str);
      break;
    case VALUE_LONG_TYPE:
      printf("%ld", v->view.as_long);
      break;
    case VALUE_DOUBLE_TYPE:
      printf("%f", v->view.as_double);
      break;
    case VALUE_STRING_TYPE:
      printf("\"");
      string_print(&v->view.as_string);
      printf("\"");
      break;
    case VALUE_SYMBOL_TYPE:
      printf("%s", v->view.as_symbol->name);
      break;
    case VALUE_UNDEFINED_TYPE:
      printf("undefined");
      break;
    default:
      printf("unknown");
  }
}

void value_bind(VALUE *v, VALUE **output)
{
  **output = *v;
  *output = *output + 1;
}

/* 
 * ----------------------------------------------------------------------
 * Section: Symbol table 
 * ----------------------------------------------------------------------
 */

SYMBOL *symbol_lookup(char *name, int *id, int append, DICTIONARY *dictionary)
{
  SYMBOL *symbol;

  /* Run through the dictionary and look for the symbol */
  for (symbol = *dictionary; symbol != NULL; symbol = symbol->next)
    if (!strcmp(symbol->name, name)) {
      return (symbol);
    }

  if (!append)
    return (NULL);
  
  symbol = (SYMBOL *) malloc(sizeof(SYMBOL));
  symbol->next = *dictionary;
  symbol->name = (char*) strdup(name);
  if (isalpha(name[0]) || name[0] == '_')
    symbol->id = 0;
  else {
    symbol->id = *id;
    *id = *id + 1;
  }
  symbol->parse = NULL;
  symbol->syntax = NULL;
  symbol->semantic = NULL;
  *dictionary = symbol;

  return (symbol);
}

void symbol_print_name(SYMBOL *symbol)
{
  if (symbol->id == 0)
    printf("symbol_%s", symbol->name);
  else
    printf("symbol_token_%d", symbol->id);
}

void symbol_bind(SYMBOL *symbol, VALUE **output)
{
  VALUE v;

  if (symbol == NULL)
    return;
  
  v.type = VALUE_SYMBOL_TYPE;
  v.view.as_symbol = symbol;

  value_bind(&v, output);
}

/* 
 * ----------------------------------------------------------------------
 * Section: Semantic virtual machine
 * ----------------------------------------------------------------------
 */

int parse_executing;

void parse_execute(ENVIRONMENT *env)
{
  if (env == NULL)
    return;

  for (parse_executing = TRUE; parse_executing;) {
    switch (env->ip->type) {
      case VALUE_PTR_TYPE:
      case VALUE_STR_TYPE:  
      case VALUE_LONG_TYPE:
      case VALUE_DOUBLE_TYPE:
      case VALUE_STRING_TYPE:
	value_push(env, env->ip);
	env->ip++;
	break;
      case VALUE_SYMBOL_TYPE:
	if (env->ip->view.as_symbol->semantic != NULL &&
	    env->ip->view.as_symbol->semantic != (SEMANTIC) 1)
	  env->ip->view.as_symbol->semantic(env);
	else
	  printf("%s: undefined semantic\n", env->ip->view.as_symbol->name);
	env->ip++;
	break;
      case VALUE_UNDEFINED_TYPE:
	parse_executing = FALSE;
	break;
      default:
	parse_executing = FALSE;
	printf("%d: unknown data type\n", env->ip->type);
	break;
    }
  }
}

/* 
 * ----------------------------------------------------------------------
 * Section: Error handler
 * ----------------------------------------------------------------------
 */

jmp_buf parse_catch_buf;
SYMBOL *parse_error_symbol;
char *parse_error_input;
int parse_tracing = FALSE;
int parse_timing = FALSE;
int parse_warning = FALSE;
int parse_indent = 0;
int parse_cutting = FALSE;
#define INDENT_STEP 2
static char *start_input;
static VALUE *start_output;

void parse_error(void)
{
  int i = parse_error_input - start_input;
  while (i--)
    putchar(' ');
  if (parse_error_symbol->syntax != NULL)
    printf("^- <%s> expected\n", parse_error_symbol->name);
  else 
    printf("^- \"%s\" expected\n", parse_error_symbol->name);
}

/* 
 * ----------------------------------------------------------------------
 * Section: Top down parser
 * ----------------------------------------------------------------------
 */

int parse_symbol(SYMBOL *symbol, char **input, VALUE **output)
{
  register char *ip;
  register char *np;
  
  /* Parse white space */
  parse_space(symbol, input, output);
  if (**input == 0)
    return (FALSE);

  /* Match with the name string */
  ip = *input;
  np = symbol->name;
  while (*np != 0 && *ip == *np) {
    ip++;
    np++;
  }

  /* If the match failed */
  if (*np != 0) 
    return (FALSE);

  *input = ip;
  return (TRUE);
}

int parse_syntax(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE *old_output;
  char *old_input;
  PRODUCT *product;
  TERM *term;
  int cutting;
  int run;
  
  /* Check that it at least has some products */
  if (symbol->syntax == NULL) {
    printf("<%s>: undefined\n", symbol->name);
    parse_warning = TRUE;
    return (FALSE);
  }

  /* Check for trace and step up indentation */
  if (parse_tracing)
    parse_indent += INDENT_STEP;

  /* Check each product. Backtrack if the product fails and no cut */
  cutting = FALSE;
  for (product = symbol->syntax; *product != NULL && !cutting; product++) {
    old_input = *input;
    old_output = *output;
    
    /* Check each term in the product */
    for (run = TRUE, term = *product; run; term++) {

      /* Check for trace of parse */
      if (parse_tracing && term->type != TERM_PRODUCT_END_TYPE) {
	int n = parse_indent;
	while (n--)
	  putchar(' ');
	if (term->type == TERM_TERMINAL_TYPE)
	  printf("\"%s\"\n", term->symbol->name);
	else 
	  printf("<%s>\n", term->symbol->name);
      }
      
      /* Decode type of term and apply */
      switch (term->type) {
	case TERM_TERMINAL_TYPE:
	  run = parse_symbol(term->symbol, input, output);
	  break;
	case TERM_NON_TERMINAL_TYPE:
	  if (term->symbol->parse == NULL)
	    return (FALSE);
	  run = term->symbol->parse(term->symbol, input, output);
	  break;
	case TERM_ZERO_OR_ONE_TYPE:
	  if (term->symbol->parse != NULL)
	    term->symbol->parse(term->symbol, input, output);
	  break;
	case TERM_ZERO_OR_MANY_TYPE:
	  if (term->symbol->parse == NULL)
	    return (FALSE);
	  while (term->symbol->parse(term->symbol, input, output));
	  break;
	case TERM_ONE_OR_MANY_TYPE:
	  if (term->symbol->parse == NULL)
	    return (FALSE);
	  run = term->symbol->parse(term->symbol, input, output);
	  if (run)
	    while (term->symbol->parse(term->symbol, input, output));
	  break;
	case TERM_PRODUCT_END_TYPE:
	  symbol_bind(term->symbol, output);
	  /* Step back indentation */
	  if (parse_tracing)
	    parse_indent -= INDENT_STEP;
	  /* Parse was found */
	  return (TRUE);
      }

      /* Check for cut */
      if (parse_cutting) {
	parse_cutting = FALSE;
	cutting = TRUE;
      }

      /* Capture error position */
      if (*input >= parse_error_input) {
	parse_error_input = *input;
	parse_error_symbol = term->symbol;
      }
    }

    /* Back-track and try next product */
    *input = old_input;
    *output = old_output;
  }

  /* Step back indentation */
  if (parse_tracing)
    parse_indent -= INDENT_STEP;

  /* Parse failed */
  return (FALSE);
}

int parse_undefined(SYMBOL *symbol, char **input, VALUE **output)
{
  printf("<%s> undefined\n", symbol->name);
  parse_warning = TRUE;
  return (FALSE);
}

int parse_empty(SYMBOL *symbol, char **input, VALUE **output)
{
  return (TRUE);
}

int parse_eoln(SYMBOL *symbol, char **input, VALUE **output)
{
  /* Parse white space */
  parse_space(symbol, input, output);
  return (**input == 0);
}

int parse_integer(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  char *token;
  char *endptr;

  /* Scan for an integer value in input */
  parse_space(symbol, input, output);
  token = *input;
  v.type = VALUE_LONG_TYPE;
  v.view.as_long = strtol(token, &endptr, 0);

  /* If not found reject parse */
  if (token == endptr) 
    return (FALSE);

  /* Bind value for semantic function */
  *input = endptr;
  value_bind(&v, output);

  return (TRUE);
}

int parse_float(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  char *token;
  char *endptr;

  /* Scan for an integer value in input */
  parse_space(symbol, input, output);
  token = *input;
  v.type = VALUE_LONG_TYPE;
  v.view.as_long = strtol(token, &endptr, 0);

  /* If not found reject parse */
  if (*endptr != '.' && *endptr != 'e') 
    return (FALSE);

  /* Scan for a floating point value */
  token = *input;
  v.type = VALUE_DOUBLE_TYPE;
  v.view.as_double = strtod(token, &endptr);

  /* If not found reject parse */
  if (token == endptr) 
    return (FALSE);

  /* Bind value for semantic function */
  *input = endptr;
  value_bind(&v, output);

  return (TRUE);
}

int parse_string(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  char *ip;
  char *tp;
  char end;
  char c;
  int n;
  
  /* Check start character */
  parse_space(symbol, input, output);
  ip = *input;
  c = *ip++;
  if (c != '"' && c != '\'')
    return (FALSE);

  /* Scan the string. No modification! */
  n = 0;
  end = c;
  tp = ip;
  do {
    n++;
    if (c == '\\') {
      c = *ip++;
      n++;
      if (c != 0) {
	c = *ip++;
	n++;
      }
    }
  } while ((c = *ip++) && (c != end));

  /* What out for end of string */
  if (c == 0)
    return (FALSE);

  n--;
  *input = ip;

  /* Bind unprocessed string value for semantic function */
  v.type = VALUE_STRING_TYPE;
  v.view.as_string.count = n;
  v.view.as_string.buffer = tp;
  value_bind(&v, output);

  return (TRUE);
}

int parse_identifier(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  char *ip = *input;
  char *tp;
  char c;
  int n;
  
  /* Check for an alpha character or underscore */
  parse_space(symbol, input, output);
  ip = *input;
  c = *ip++;
  if (!isalpha(c) && (c != '_'))
    return (FALSE);

  /* Scan identifier: alpha, number or underscore sequence */
  tp = ip - 1;
  n = 0;
  do {
    n++;
  } while ((c = *ip++) && (isalnum(c) || c == '_'));
  *input = ip - 1;
  
  /* Bind value for semantic function */
  v.type = VALUE_STRING_TYPE;
  v.view.as_string.count = n;
  v.view.as_string.buffer = tp;
  value_bind(&v, output);

  return (TRUE);
}

int parse_token(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  char *ip;
  char *tp;
  char c;
  int n;
  
  /* Check that there was something left */
  parse_space(symbol, input, output);
  ip = *input;
  c = *ip++;
  if (c == 0)
    return (FALSE);

  /* Scan anything until white space */
  tp = ip - 1;
  n = 0;
  do {
    n++;
  } while ((c = *ip++) && (c > ' '));
  *input = ip - 1;

  /* Bind value for semantic function */
  v.type = VALUE_STRING_TYPE;
  v.view.as_string.count = n;
  v.view.as_string.buffer = tp;
  value_bind(&v, output);

  return (TRUE);
}

int parse_nospace(SYMBOL *symbol, char **input, VALUE **output)
{
  return (**input > ' ' || **input == 0);
}

int parse_space(SYMBOL *symbol, char **input, VALUE **output)
{
  char *ip = *input;
  char *tp;
  char c;
  
  /* No space */
  if (*ip > ' ' || *ip == 0)
    return (FALSE);

  /* Skip space */
  do {
    c = *ip++;
  } while (c <= ' ' && c != 0);
  *input = ip - 1;
  
  return (TRUE);
}

int parse_throw(SYMBOL *symbol, char **input, VALUE **output)
{
  longjmp(parse_catch_buf, 1);
  return (FALSE);
}

int parse_cut(SYMBOL *symbol, char **input, VALUE **output)
{
  parse_cutting = TRUE;
  return (TRUE);
}

int parse_run(SYMBOL *symbol, char **input, VALUE **output)
{
  ENVIRONMENT env;
  VALUE stack[64];

  (*output)->type = VALUE_UNDEFINED_TYPE;
  env.sp = stack;
  env.ip = start_output;
  parse_execute(&env);
  *output = start_output;
  return (!parse_executing);
}

int parse_pos(SYMBOL *symbol, char **input, VALUE **output)
{
  VALUE v;
  
  /* Bind current position for semantic function */
  v.type = VALUE_STR_TYPE;
  v.view.as_str = *input;
  value_bind(&v, output);

  return (TRUE);
}

int parse_input(SYMBOL *symbol, char **input, VALUE **output)
{
  clock_t start;
  int ok;
  
  /* Watch out for rookie programmers */
  if (symbol == NULL || input == NULL || output == NULL)
    return (FALSE);

  /* Are we doing timing? */
  if (parse_timing)
    start = clock();

  /* Setup error capture environment */
  start_input = parse_error_input = *input;
  start_output = *output;
  parse_error_symbol = symbol;
  parse_indent = 0;
  parse_warning = FALSE;
  
  /* Capture parse error mark */
  if (setjmp(parse_catch_buf) == 0) {
    
    /* Parse the input string */
    if (symbol->parse == NULL)
      ok = parse_syntax(symbol, input, output);
    else
      ok = symbol->parse(symbol, input, output);
  } else {
    ok = FALSE;
  }
  
  /* Timing? when display the result */
  if (parse_timing)
    printf("parse_input: %ld ms\n", ((clock() - start) * 1000)/CLK_TCK);

  /* Check parse result and just leave if no parse */
  if (!ok)
    return (FALSE);

  /* Append an execute halting value if ok */
  (*output)->type = VALUE_UNDEFINED_TYPE;
  return (TRUE);
}

/* 
 * ----------------------------------------------------------------------
 * Section: Generic grammar elements:
 *
 *  <empty>	  Always true
 *  <eoln>	  True if no more input
 *  <integer>	  True if an integer is next input. Binds integer value.
 *  <float>	  True if a floating pointer is next. Binds value.
 *  <string>      True if a string literal is next. Binds string value.
 *  <token>       True if a token is next. Binds string value.
 *  <identifier>  True if an identifier is next. Binds string value.
 *  <space>       True if white space is next.
 *  <nospace>     True if next is not white space.
 *  <error>       True always and terminates parse.
 *  <cut>         Always true. Do not back-track and try other products.
 *  <execute>     Perform semantics. 
 * ----------------------------------------------------------------------
 */

SYMBOL symbol_empty = {
  NULL, "empty", 0, NULL, parse_empty, NULL
};

SYMBOL symbol_eoln = {
  &symbol_empty, "eoln", 0, NULL, parse_eoln, NULL
};

SYMBOL symbol_integer = {
  &symbol_eoln, "integer", 0, NULL, parse_integer, NULL
};

SYMBOL symbol_float = {
  &symbol_integer, "float", 0, NULL, parse_float, NULL
};

SYMBOL symbol_string = {
  &symbol_float, "string", 0, NULL, parse_string, NULL
};

SYMBOL symbol_token = {
  &symbol_string, "token", 0, NULL, parse_token, NULL
};

SYMBOL symbol_identifier = {
  &symbol_token, "identifier", 0, NULL, parse_identifier, NULL
};

SYMBOL symbol_nospace = {
  &symbol_identifier, "nospace", 0, NULL, parse_nospace, NULL
};

SYMBOL symbol_space = {
  &symbol_nospace, "space", 0, NULL, parse_space, NULL
};

SYMBOL symbol_cut = {
  &symbol_space, "cut", 0, NULL, parse_cut, NULL
};

SYMBOL symbol_error = {
  &symbol_cut, "error", 0, NULL, parse_throw, NULL
};

SYMBOL symbol_execute = {
  &symbol_error, "execute", 0, NULL, parse_run, NULL
};

SYMBOL symbol_pos = {
  &symbol_execute, "pos", 0, NULL, parse_pos, NULL
};

SYMBOL symbol_value_add = {
  &symbol_pos, "value_add", 0, NULL, parse_syntax, semantic_value_add
};

SYMBOL symbol_value_sub = {
  &symbol_value_add, "value_sub", 0, NULL, parse_syntax, semantic_value_sub
};

SYMBOL symbol_value_mul = {
  &symbol_value_sub, "value_mul", 0, NULL, parse_syntax, semantic_value_mul
};

SYMBOL symbol_value_div = {
  &symbol_value_mul, "value_div", 0, NULL, parse_syntax, semantic_value_div
};

SYMBOL symbol_value_mod = {
  &symbol_value_div, "value_mod", 0, NULL, parse_syntax, semantic_value_mod
};

SYMBOL symbol_value_typeof = {
  &symbol_value_mod, "value_typeof", 0, NULL, parse_syntax, semantic_value_typeof
};

SYMBOL symbol_value_asinteger = {
  &symbol_value_typeof, "value_asinteger", 0, NULL, parse_syntax, semantic_value_asinteger
};

SYMBOL symbol_value_asfloat = {
  &symbol_value_asinteger, "value_asfloat", 0, NULL, parse_syntax, semantic_value_asfloat
};

SYMBOL symbol_value_print = {
  &symbol_value_asfloat, "value_print", 0, NULL, parse_syntax, semantic_value_print
};

DICTIONARY parse_dictionary = &PARSE_LAST_SYMBOL;

#define VALUE_BINARY_OP(env,op) \
{ \
  VALUE *x, *y; \
  value_pop(env, y); \
  value_tos(env, x); \
  switch (y->type) { \
    case VALUE_LONG_TYPE: \
      switch (x->type) { \
	case VALUE_LONG_TYPE: \
	  x->view.as_long = x->view.as_long op y->view.as_long; \
	  return; \
	case VALUE_DOUBLE_TYPE: \
	  x->view.as_double = x->view.as_double op y->view.as_long; \
	  return; \
      } \
      break; \
    case VALUE_DOUBLE_TYPE: \
      switch (x->type) { \
	case VALUE_LONG_TYPE: \
	  x->view.as_double = x->view.as_long op y->view.as_double; \
	  x->type = VALUE_DOUBLE_TYPE; \
	  return; \
	case VALUE_DOUBLE_TYPE: \
	  x->view.as_double = x->view.as_double op y->view.as_double; \
	  return; \
      } \
  } \
  x->type = VALUE_UNDEFINED_TYPE; \
}

void semantic_value_add(ENVIRONMENT *env) VALUE_BINARY_OP(env,+)
void semantic_value_sub(ENVIRONMENT *env) VALUE_BINARY_OP(env,-)
void semantic_value_mul(ENVIRONMENT *env) VALUE_BINARY_OP(env,*)
void semantic_value_div(ENVIRONMENT *env) VALUE_BINARY_OP(env,/)
void semantic_value_mod(ENVIRONMENT *env)
{ 
  VALUE *x, *y; 
  value_pop(env, y); 
  value_tos(env, x); 
  switch (y->type) { 
    case VALUE_LONG_TYPE: 
      switch (x->type) { 
	case VALUE_LONG_TYPE: 
	  x->view.as_long = x->view.as_long % y->view.as_long; 
	  return; 
      } 
      break; 
  } 
  x->type = VALUE_UNDEFINED_TYPE;
}

void semantic_value_asinteger(ENVIRONMENT *env)
{
  char *startptr;
  char *endptr;
  VALUE *v;
  
  value_tos(env, v);
  switch (v->type) { 
    case VALUE_LONG_TYPE: 
      return;
    case VALUE_DOUBLE_TYPE:
      v->type = VALUE_LONG_TYPE;
      v->view.as_long = v->view.as_double + 0.5;
      return;
    case VALUE_STR_TYPE:
      startptr = v->view.as_str;
    case VALUE_STRING_TYPE:
      if (v->type == VALUE_STRING_TYPE)
	startptr = v->view.as_string.buffer;
      v->view.as_long = strtol(startptr, &endptr, 0);
      if (endptr == startptr)
	break;
      v->type = VALUE_LONG_TYPE;
      return;
  } 
  v->type = VALUE_UNDEFINED_TYPE;
}

void semantic_value_asfloat(ENVIRONMENT *env)
{
  char *startptr;
  char *endptr;
  VALUE *v;
  
  value_tos(env, v);
  switch (v->type) { 
    case VALUE_DOUBLE_TYPE:
      return;
    case VALUE_LONG_TYPE: 
      v->type = VALUE_DOUBLE_TYPE;
      v->view.as_double = v->view.as_long;
      return;
    case VALUE_STR_TYPE:
      startptr = v->view.as_str;
    case VALUE_STRING_TYPE:
      if (v->type == VALUE_STRING_TYPE)
	startptr = v->view.as_string.buffer;
      v->view.as_double = strtod(startptr, &endptr);
      if (endptr == startptr)
	break;
      v->type = VALUE_DOUBLE_TYPE;
      return;
  } 
  v->type = VALUE_UNDEFINED_TYPE;
}

void semantic_value_print(ENVIRONMENT *env)
{
  VALUE *v;
  value_pop(env, v);
  value_print(v);
  printf("\n");
}

void semantic_value_typeof(ENVIRONMENT *env)
{
  VALUE *v;
  value_tos(env, v);
  if (v->type != VALUE_UNDEFINED_TYPE && v->type != VALUE_UNKNOWN_TYPE) {
    v->view.as_type = v->type;
    v->type = VALUE_TYPE_TYPE;
  }
}

/* 
 * ----------------------------------------------------------------------
 * Section: Test program (the parser)
 * ----------------------------------------------------------------------
 */

#if defined(TEST)

int main(int argc, char **argv)
{
  FILE *inf = NULL;
  char source[512];
  char *input;
  ENVIRONMENT env;
  VALUE code[128];
  VALUE *output;
  VALUE stack[64];
  VALUE *sp;
  int compile;
  int arg;
  
  if (argc > 1) {
    if (!strcmp(argv[1], "-h")) {
      printf("usage: parse [-c] [file...]\n");
      return (1);
    }
    if (!strcmp(argv[1], "-c")) {
      compile = TRUE;
      arg = 2;
    } else {
      compile = FALSE;
      arg = 1;
    }
    inf = fopen(argv[arg], "r");
    if (inf == NULL)
      printf("%s: unknown file\n", argv[arg]);
    arg++;
  }
  if (inf == NULL)
    inf = stdin;
  
  main_symbol = &symbol_bnf;
  for (;;) {
    char *s = source;

    /* Prompt for input */
    if (isatty(fileno(inf)))
	printf("%s> ", main_symbol->name);

    /* Multi-line read */
    s = fgets(s, 256, inf);
    if (s == NULL) {
      if (inf == stdin) 
	break;
      fclose(inf);
      inf = NULL;
      while (arg < argc && inf == NULL) {
	inf = fopen(argv[arg], "r");
	if (inf == NULL)
	  printf("%s: unknown file\n", argv[arg]);
	arg++;
      }
      if (inf == NULL) {
	inf = stdin;
	if (compile) {
	  bnf_compile();
	  return (0);
	}
	if (!isatty(fileno(stdout)))
	  return (0);
	if (isatty(fileno(inf))) {
	  printf("%s> ", main_symbol->name);
	}
      }
      s = source;
      s = fgets(s, 256, inf);
      if (feof(inf))
	break;
    }
    s[strlen(s) - 1] = 0;
    while (s[strlen(s) - 1] == '\\') {
      s = s + strlen(s) - 1;
      if ((s = fgets(s, 256, inf)) == NULL || *s == 0)
	break;
      s[strlen(s) - 1] = 0;
    };

    /* Empty line then read again */
    if (*source == 0)
      continue;

    /* Set up and parse input to output. If successful execute parse */
    input = source;
    output = code;
    if (!compile && !isatty(fileno(inf)))
      printf("# %s\n", source);
    if (parse_input(main_symbol, &input, &output)) {
      env.sp = stack;
      env.ip = code;
      parse_execute(&env);
    } else {
      if (!strcmp(source, "!shell"))
	break;
      else if (!strcmp(source, "!bnf"))
	main_symbol = &symbol_bnf;
      else {
	if (isatty(fileno(inf)) && !parse_tracing && !parse_warning) {
	  int n = strlen(main_symbol->name) + 2;
	  while (n--)
	    putchar(' ');
	} else 
	  printf("%s\n", source);
	parse_error();
      }
    }
  }

  return (0);
}

#endif /* TEST */
