/* $Id: test.c 8 2009-08-24 22:27:55Z Mikael Patel $
 * ----------------------------------------------------------------------
 * Name
 *   test - test program for the top down parser
 *  
 * Description:	
 *   Example of an application compiled from a grammar. The grammar file,
 *   test.bnf, is compile to a source file, test.g, and included. This
 *   file contains the grammar as a set of structures.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "parse.h"
#include "test.g"

typedef struct VARIABLE VARIABLE;

struct VARIABLE {
  VARIABLE *next;
  char *name;
  VALUE value;
};

VARIABLE *variable_lookup(char *name, int append, VARIABLE **dictionary)
{
  VARIABLE *variable;

  for (variable = *dictionary; variable != NULL; variable = variable->next)
    if (!strcmp(variable->name, name)) {
      return (variable);
    }

  if (!append)
    return (NULL);
  
  variable = (VARIABLE *) malloc(sizeof(VARIABLE));
  variable->next = *dictionary;
  variable->name = strdup(name);
  variable->value.type = VALUE_UNDEFINED_TYPE;
  *dictionary = variable;

  return (variable);
}

VARIABLE *dictionary = NULL;

#define BINARY(op) \
{ \
  VALUE *x, *y; \
  value_pop(env, x); \
  value_tos(env, y); \
  y->view.as_long = y->view.as_long op x->view.as_long; \
}

void semantic_modulo(ENVIRONMENT *env) BINARY(%)
void semantic_divide(ENVIRONMENT *env) BINARY(/)
void semantic_multiply(ENVIRONMENT *env) BINARY(*)
void semantic_addition(ENVIRONMENT *env) BINARY(+)
void semantic_subtract(ENVIRONMENT *env) BINARY(-)
void semantic_shift_right(ENVIRONMENT *env) BINARY(>>)
void semantic_shift_left(ENVIRONMENT *env) BINARY(<<)
void semantic_less_than(ENVIRONMENT *env) BINARY(<)
void semantic_greater_than(ENVIRONMENT *env) BINARY(>)
void semantic_less_or_equal(ENVIRONMENT *env) BINARY(<=)
void semantic_greater_or_equal(ENVIRONMENT *env) BINARY(>=)
void semantic_equal(ENVIRONMENT *env) BINARY(==)
void semantic_not_equal(ENVIRONMENT *env) BINARY(!=)
void semantic_bitwise_and(ENVIRONMENT *env) BINARY(&)
void semantic_bitwise_or(ENVIRONMENT *env) BINARY(|)
void semantic_bitwise_xor(ENVIRONMENT *env) BINARY(^)
void semantic_logical_and(ENVIRONMENT *env) BINARY(&&)
void semantic_logical_or(ENVIRONMENT *env) BINARY(||)

void semantic_condition(ENVIRONMENT *env)
{
  VALUE *x, *y, *cond;

  value_pop(env, y);
  value_pop(env, x);
  value_tos(env, cond);
  cond->view.as_long = cond->view.as_long ? x->view.as_long : y->view.as_long;
}

void semantic_display(ENVIRONMENT *env)
{
  VALUE *x;
  value_pop(env, x);
  value_print(x);
  printf("\n");
}

void semantic_get(ENVIRONMENT *env)
{
  char name[128];
  VARIABLE *variable;
  VALUE *v;
  
  value_pop(env, v);
  name[0] = 0;
  strncat(name, v->view.as_string.buffer, v->view.as_string.count);
  variable = variable_lookup(name, FALSE, &dictionary);
  if (variable != NULL) {
    value_push(env, &variable->value);
  } else {
    printf("%s: undefined variable\n", name);
    parse_executing = FALSE;
  }
}

void semantic_put(ENVIRONMENT *env)
{
  char name[128];
  VARIABLE *variable;
  VALUE *v;
  VALUE *n;
  
  value_pop(env, v);
  value_pop(env, n);
  name[0] = 0;
  strncat(name, n->view.as_string.buffer, n->view.as_string.count);
  variable = variable_lookup(name, TRUE, &dictionary);
  variable->value = *v;
  value_push(env, v);
}

int main(int argc, char **argv)
{
  ENVIRONMENT env;
  char source[512];
  char *input;
  VALUE code[128];
  VALUE *output;
  VALUE stack[64];
  VALUE *sp;

  for (;;) {
    char *s = source;

    /* Prompt for input */
    if (isatty(fileno(stdin)))
	printf("test> ");

    /* Read input */
    s = fgets(s, 512, stdin);
    if (s == NULL)
      return (0);
    s[strlen(s) - 1] = 0;

    /* Empty line then read again */
    if (*source == 0)
      continue;

    /* Set up and parse input to output. If successful execute parse */
    input = source;
    output = code;
    if (parse_input(&symbol_test, &input, &output)) {
      env.sp = stack;
      env.ip = code;
      parse_execute(&env);
    } else {
      if (isatty(fileno(stdin)) && !parse_tracing && !parse_warning) {
    	  printf("      ");
      } else 
    	  printf("%s\n", source);
      parse_error();
    }
  }

  return (0);
}

