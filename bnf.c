/* $Id: bnf.c 11 2009-08-26 07:59:30Z Mikael Patel $
 * ----------------------------------------------------------------------
 * Name
 *   bnf - extended bnf for the top down parse machine
 *  
 * Description:	
 *   General top down parse machine with meta grammar for Extended
 *   Backus Naur Form (EBNF).
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
#include <stdlib.h>
#include <stdio.h>

#include "parse.h"
#include "bnf.h"

/* 
 * ----------------------------------------------------------------------
 * Grammar: Backus Naur Form (BNF)
 *
 * <bnf> ::= < <identifier> > ::= <bnf_product> <eoln> @ bnf_first_product
 *        |  | <bnf_product> <eoln> @ bnf_next_product
 *        |  <bnf_cmd> <eoln>
 *        |  # 
 *
 * <bnf_product> ::= <bnf_term+>
 *          
 * <bnf_term> ::= < <identifier> > @ bnf_non_terminal
 *             |  ' <identifier> @ bnf_terminal
 *             |  <identifier> @ bnf_terminal
 *             |  @ <identifier> @ bnf_semantic
 *             |  ' ' <token> @ bnf_terminal
 *             |  <token> @ bnf_terminal
 *
 * <bnf_cmd> ::= ! <identifier> @ bnf_execute
 *            |  ? <identifier> @ bnf_display
 *            |  trace on @ bnf_trace_on
 *            |  trace off @ bnf_trace_off
 *            |  timing on @ bnf_timing_on
 *            |  timing off @ bnf_timing_off
 *	      |  compile @ bnf_compile
 *            |  ? ? @ bnf_list
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Grammar: Extended Backus Naur Form (EBNF)
 *
 * <ebnf> ::= < <identifier> > ::= <ebnf_product> <eoln> @ bnf_first_product
 *         |  | <ebnf_product> <eoln> @ bnf_next_product
 *         |  <bnf_cmd> <eoln>
 *         |  #
 *
 * <ebnf_product> ::= <ebnf_term+>
 *
 * <ebnf_term> ::= < <identifier> > @ bnf_non_terminal
 *              |  [ < <identifier> > ] @ bnf_zero_or_one
 *              |  { < <identifier> > } @ bnf_zero_or_many
 *              |  ' ' <identifier> @ bnf_terminal
 *              |  <identifier> @ bnf_terminal
 *              |  @ <identifier> @ bnf_semantic
 *              |  ' ' <token> @ bnf_terminal
 *              |  <token> @ bnf_terminal
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Grammar: eXtended Backus Naur Form (XBNF)
 *
 * <xbnf> ::= < <identifier> > ::= <xbnf_product> <eoln> @ bnf_first_product
 *         |  | <xbnf_product> <eoln> @ bnf_next_product
 *         |  <bnf_cmd> <eoln>
 *         |  # 
 *
 * <xbnf_product> ::= <xbnf_term+>
 *          
 * <xbnf_term> ::= < <identifier> > @ bnf_non_terminal
 *             |  < <identifier> ? > @ bnf_zero_or_one
 *             |  < <identifier> * > @ bnf_zero_or_many
 *             |  < <identifier> + > @ bnf_one_or_many
 *             |  ' <identifier> @ bnf_terminal
 *             |  <identifier> @ bnf_terminal
 *             |  @ <identifier> @ bnf_semantic
 *             |  ' ' <token> @ bnf_terminal
 *             |  <token> @ bnf_terminal
 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * Grammar: YACC Meta Grammar
 *
 * <yacc> ::= <identifier> : <yacc_product> <eoln> @ bnf_first_product
 *         |  ' | <yacc_product> <eoln> @ bnf_next_product
 *         |  ; <eoln> 
 *         |  <bnf_cmd> <eoln>
 *         |  #
 *
 * <yacc_product> ::= <yacc_term+>
 * 
 * <yacc_term> ::= <string> @ bnf_terminal
 *	        |  ' @ <identifier> @ bnf_semantic
 *              |  <identifier> ? @ bnf_zero_or_one
 *              |  <identifier> * @ bnf_zero_or_many
 *              |  <identifier> + @ bnf_one_or_many
 *              |  <identifier> @ bnf_non_terminal
 *
 * ----------------------------------------------------------------------
 */

extern PRODUCT syntax_bnf[];
extern TERM product_bnf_1[];
extern TERM product_bnf_2[];
extern TERM product_bnf_3[];
extern TERM product_bnf_4[];

extern PRODUCT syntax_bnf_product[];
extern TERM product_bnf_product_1[];

extern PRODUCT syntax_bnf_term[];
extern TERM product_bnf_term_1[];
extern TERM product_bnf_term_2[];
extern TERM product_bnf_term_3[];
extern TERM product_bnf_term_4[];
extern TERM product_bnf_term_5[];
extern TERM product_bnf_term_6[];

extern PRODUCT syntax_bnf_cmd[];
extern TERM product_bnf_cmd_1[];
extern TERM product_bnf_cmd_2[];
extern TERM product_bnf_cmd_3[];
extern TERM product_bnf_cmd_4[];
extern TERM product_bnf_cmd_5[];
extern TERM product_bnf_cmd_6[];
extern TERM product_bnf_cmd_7[];
extern TERM product_bnf_cmd_8[];

extern PRODUCT syntax_ebnf[];
extern TERM product_ebnf_1[];
extern TERM product_ebnf_2[];
extern TERM product_ebnf_3[];
extern TERM product_ebnf_4[];

extern PRODUCT syntax_ebnf_product[];
extern TERM product_ebnf_product_1[];

extern PRODUCT syntax_ebnf_term[];
extern TERM product_ebnf_term_1[];
extern TERM product_ebnf_term_2[];
extern TERM product_ebnf_term_3[];
extern TERM product_ebnf_term_4[];
extern TERM product_ebnf_term_5[];
extern TERM product_ebnf_term_6[];
extern TERM product_ebnf_term_7[];
extern TERM product_ebnf_term_8[];

extern PRODUCT syntax_xbnf[];
extern TERM product_xbnf_1[];
extern TERM product_xbnf_2[];
extern TERM product_xbnf_3[];
extern TERM product_xbnf_4[];

extern PRODUCT syntax_xbnf_product[];
extern TERM product_xbnf_product_1[];

extern PRODUCT syntax_xbnf_term[];
extern TERM product_xbnf_term_1[];
extern TERM product_xbnf_term_2[];
extern TERM product_xbnf_term_3[];
extern TERM product_xbnf_term_4[];
extern TERM product_xbnf_term_5[];
extern TERM product_xbnf_term_6[];
extern TERM product_xbnf_term_7[];
extern TERM product_xbnf_term_8[];
extern TERM product_xbnf_term_9[];

extern PRODUCT syntax_yacc[];
extern TERM product_yacc_1[];
extern TERM product_yacc_2[];
extern TERM product_yacc_3[];
extern TERM product_yacc_4[];

extern PRODUCT syntax_yacc_product[];
extern TERM product_yacc_product_1[];

extern PRODUCT syntax_yacc_term[];
extern TERM product_yacc_term_1[];
extern TERM product_yacc_term_2[];
extern TERM product_yacc_term_3[];
extern TERM product_yacc_term_4[];
extern TERM product_yacc_term_5[];
extern TERM product_yacc_term_6[];

extern void semantic_bnf_first_product(ENVIRONMENT*);
extern void semantic_bnf_next_product(ENVIRONMENT*);
extern void semantic_bnf_non_terminal(ENVIRONMENT*);
extern void semantic_bnf_zero_or_one(ENVIRONMENT*);
extern void semantic_bnf_zero_or_many(ENVIRONMENT*);
extern void semantic_bnf_one_or_many(ENVIRONMENT*);
extern void semantic_bnf_terminal(ENVIRONMENT*);
extern void semantic_bnf_semantic(ENVIRONMENT*);
extern void semantic_bnf_execute(ENVIRONMENT*);
extern void semantic_bnf_display(ENVIRONMENT*);
extern void semantic_bnf_list(ENVIRONMENT*);
extern void semantic_bnf_compile(ENVIRONMENT*);
extern void semantic_bnf_trace_on(ENVIRONMENT*);
extern void semantic_bnf_trace_off(ENVIRONMENT*);
extern void semantic_bnf_timing_on(ENVIRONMENT*);
extern void semantic_bnf_timing_off(ENVIRONMENT*);

SYMBOL symbol_yacc = {
  &PARSE_LAST_SYMBOL, "yacc", 0, syntax_yacc, parse_syntax, NULL
};

SYMBOL symbol_xbnf = {
  &symbol_yacc, "xbnf", 0, syntax_xbnf, parse_syntax, NULL
};

SYMBOL symbol_ebnf = {
  &symbol_xbnf, "ebnf", 0, syntax_ebnf, parse_syntax, NULL
};

SYMBOL symbol_bnf = {
  &symbol_ebnf, "bnf", 0, syntax_bnf, parse_syntax, NULL
};

#define symbol_assignment symbol_token_128
SYMBOL symbol_assignment = {
  &symbol_bnf, "::=", 128, NULL, parse_syntax, NULL
};

#define symbol_or symbol_token_129
SYMBOL symbol_or = {
  &symbol_assignment, "|", 129, NULL, parse_syntax, NULL
};

#define symbol_less_than symbol_token_130
SYMBOL symbol_less_than = {
  &symbol_or, "<", 130, NULL, parse_syntax, NULL
};

#define symbol_greater_than symbol_token_131
SYMBOL symbol_greater_than = {
  &symbol_less_than, ">", 131, NULL, parse_syntax, NULL
};

#define symbol_at symbol_token_132
SYMBOL symbol_at = {
  &symbol_greater_than, "@", 132, NULL, parse_syntax, NULL
};

#define symbol_quote symbol_token_133
SYMBOL symbol_quote = {
  &symbol_at, "'", 133, NULL, parse_syntax, NULL
};

#define symbol_question symbol_token_134
SYMBOL symbol_question = {
  &symbol_quote, "?", 134, NULL, parse_syntax, NULL
};

#define symbol_plus symbol_token_135
SYMBOL symbol_plus = {
  &symbol_question, "+", 135, NULL, parse_syntax, NULL
};

#define symbol_times symbol_token_136
SYMBOL symbol_times = {
  &symbol_plus, "*", 136, NULL, parse_syntax,
  NULL
};

#define symbol_exclamation symbol_token_137
SYMBOL symbol_exclamation = {
  &symbol_times, "!", 137, NULL, parse_syntax, NULL
};

#define symbol_sharp symbol_token_138
SYMBOL symbol_sharp = {
  &symbol_exclamation, "#", 138, NULL, parse_syntax, NULL
};

#define symbol_left_bracket symbol_token_139
SYMBOL symbol_left_bracket = {
  &symbol_sharp, "[", 139, NULL, parse_syntax, NULL
};

#define symbol_right_bracket symbol_token_140
SYMBOL symbol_right_bracket = {
  &symbol_left_bracket, "]", 140, NULL, parse_syntax, NULL
};

#define symbol_left_curle symbol_token_141
SYMBOL symbol_left_curle = {
  &symbol_right_bracket, "{", 141, NULL, parse_syntax, NULL
};

#define symbol_right_curle symbol_token_142
SYMBOL symbol_right_curle = {
  &symbol_left_curle, "}", 142, NULL, parse_syntax, NULL
};

#define symbol_colon symbol_token_143
SYMBOL symbol_colon = {
  &symbol_right_curle, ":", 143, NULL, parse_syntax, NULL
};

#define symbol_semicolon symbol_token_144
SYMBOL symbol_semicolon = {
  &symbol_colon, ";", 144, NULL, parse_syntax, NULL
};

SYMBOL symbol_trace = {
  &symbol_semicolon, "trace", 0, NULL, parse_syntax, NULL
};

SYMBOL symbol_timing = {
  &symbol_trace, "timing", 0, NULL, parse_syntax, NULL
};

SYMBOL symbol_on = {
  &symbol_timing, "on", 0, NULL, parse_syntax, NULL
};

SYMBOL symbol_off = {
  &symbol_on, "off", 0, NULL, parse_syntax, NULL
};

SYMBOL symbol_compile = {
  &symbol_off, "compile", 0, NULL, parse_syntax, NULL
};

SYMBOL symbol_yacc_product = {
  &symbol_compile, "yacc_product", 0, syntax_yacc_product, parse_syntax, NULL
};

SYMBOL symbol_yacc_term = {
  &symbol_yacc_product, "yacc_term", 0, syntax_yacc_term, parse_syntax, NULL
};

SYMBOL symbol_xbnf_product = {
  &symbol_yacc_term, "xbnf_product", 0, syntax_xbnf_product, parse_syntax, NULL
};

SYMBOL symbol_xbnf_term = {
  &symbol_xbnf_product, "xbnf_term", 0, syntax_xbnf_term, parse_syntax, NULL
};

SYMBOL symbol_ebnf_product = {
  &symbol_xbnf_term, "ebnf_product", 0, syntax_ebnf_product, parse_syntax, NULL
};

SYMBOL symbol_ebnf_term = {
  &symbol_ebnf_product, "ebnf_term", 0, syntax_ebnf_term, parse_syntax, NULL
};

SYMBOL symbol_bnf_product = {
  &symbol_ebnf_term, "bnf_product", 0, syntax_bnf_product, parse_syntax, NULL
};

SYMBOL symbol_bnf_term = {
  &symbol_bnf_product, "bnf_term", 0, syntax_bnf_term, parse_syntax, NULL
};

SYMBOL symbol_bnf_cmd = {
  &symbol_bnf_term, "bnf_cmd", 0, syntax_bnf_cmd, parse_syntax, NULL
};

SYMBOL symbol_bnf_first_product = {
  &symbol_bnf_cmd, "bnf_first_product", 0, NULL, NULL, semantic_bnf_first_product
};

SYMBOL symbol_bnf_next_product = {
  &symbol_bnf_first_product, "bnf_next_product", 0, NULL, NULL, semantic_bnf_next_product
};

SYMBOL symbol_bnf_non_terminal = {
  &symbol_bnf_next_product, "bnf_non_terminal", 0, NULL, NULL, semantic_bnf_non_terminal
};

SYMBOL symbol_bnf_zero_or_one = {
  &symbol_bnf_non_terminal, "bnf_zero_or_one", 0, NULL, NULL, semantic_bnf_zero_or_one
};

SYMBOL symbol_bnf_zero_or_many = {
  &symbol_bnf_zero_or_one, "bnf_zero_or_many", 0, NULL, NULL, semantic_bnf_zero_or_many
};

SYMBOL symbol_bnf_one_or_many = {
  &symbol_bnf_zero_or_many, "bnf_one_or_many", 0, NULL, NULL, semantic_bnf_one_or_many
};

SYMBOL symbol_bnf_terminal = {
  &symbol_bnf_one_or_many, "bnf_terminal", 0, NULL, NULL, semantic_bnf_terminal
};

SYMBOL symbol_bnf_semantic = {
  &symbol_bnf_terminal, "bnf_semantic", 0, NULL, NULL, semantic_bnf_semantic
};

SYMBOL symbol_bnf_execute = {
  &symbol_bnf_semantic, "bnf_execute", 0, NULL, NULL, semantic_bnf_execute
};

SYMBOL symbol_bnf_display = {
  &symbol_bnf_execute, "bnf_display", 0, NULL, NULL, semantic_bnf_display
};

SYMBOL symbol_bnf_list = {
  &symbol_bnf_display, "bnf_list", 0, NULL, NULL, semantic_bnf_list
};

SYMBOL symbol_bnf_compile = {
  &symbol_bnf_list, "bnf_compile", 0, NULL, NULL, semantic_bnf_compile
};

SYMBOL symbol_bnf_trace_on = {
  &symbol_bnf_compile, "bnf_trace_on", 0, NULL, NULL, semantic_bnf_trace_on
};

SYMBOL symbol_bnf_trace_off = {
  &symbol_bnf_trace_on, "bnf_trace_off", 0, NULL, NULL, semantic_bnf_trace_off
};

SYMBOL symbol_bnf_timing_on = {
  &symbol_bnf_trace_off, "bnf_timing_on", 0, NULL, NULL, semantic_bnf_timing_on
};

SYMBOL symbol_bnf_timing_off = {
  &symbol_bnf_timing_on, "bnf_timing_off", 0, NULL, NULL, semantic_bnf_timing_off
};

#define BNF_LAST_SYMBOL symbol_bnf_timing_off

/* 
 * ----------------------------------------------------------------------
 * Grammar: Backus Naur Form (BNF)
 *
 * <bnf> ::= < <identifier> > ::= <bnf_product> <eoln> @ bnf_first_product
 *        |  | <bnf_product> <eoln> @ bnf_next_product
 *        |  <bnf_cmd> <eoln>
 *        |  # 
 *
 * <bnf_product> ::= <bnf_term+>
 *          
 * <bnf_term> ::= < <identifier> > @ bnf_non_terminal
 *             |  ' <identifier> @ bnf_terminal
 *             |  <identifier> @ bnf_terminal
 *             |  @ <identifier> @ bnf_semantic
 *             |  ' ' <token> @ bnf_terminal
 *             |  <token> @ bnf_terminal
 *
 * <bnf_cmd> ::= ! <identifier> @ bnf_execute
 *            |  ? <identifier> @ bnf_display
 *            |  trace on @ bnf_trace_on
 *            |  trace off @ bnf_trace_off
 *            |  timing on @ bnf_timing_on
 *            |  timing off @ bnf_timing_off
 *	      |  compile @ bnf_compile
 *            |  ? ? @ bnf_list
 *
 * ----------------------------------------------------------------------
 */

PRODUCT syntax_bnf[] = {
  product_bnf_1,
  product_bnf_2,
  product_bnf_3,
  product_bnf_4,
  NULL
};

TERM product_bnf_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_TERMINAL_TYPE, &symbol_assignment },
  { TERM_NON_TERMINAL_TYPE, &symbol_bnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_first_product }
};

TERM product_bnf_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_or },
  { TERM_NON_TERMINAL_TYPE, &symbol_bnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_next_product }
};

TERM product_bnf_3[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_bnf_cmd },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, NULL }
};

TERM product_bnf_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_sharp },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_bnf_product[] = {
  product_bnf_product_1,
  NULL
};
      
TERM product_bnf_product_1[] = {
  { TERM_ONE_OR_MANY_TYPE, &symbol_bnf_term },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_bnf_term[] = {
  product_bnf_term_1,
  product_bnf_term_2,
  product_bnf_term_3,
  product_bnf_term_4,
  product_bnf_term_5,
  product_bnf_term_6,
  NULL
};
      
TERM product_bnf_term_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_non_terminal }
};

TERM product_bnf_term_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_bnf_term_3[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_bnf_term_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_at },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_semantic }
};

TERM product_bnf_term_5[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_bnf_term_6[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

PRODUCT syntax_bnf_cmd[] = {
  product_bnf_cmd_1,
  product_bnf_cmd_2,
  product_bnf_cmd_3,
  product_bnf_cmd_4,
  product_bnf_cmd_5,
  product_bnf_cmd_6,
  product_bnf_cmd_7,
  product_bnf_cmd_8,
  NULL
};

TERM product_bnf_cmd_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_exclamation },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_execute }
};

TERM product_bnf_cmd_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_question },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_display }
};

TERM product_bnf_cmd_3[] = {
  { TERM_TERMINAL_TYPE, &symbol_trace },
  { TERM_TERMINAL_TYPE, &symbol_on },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_trace_on }
};

TERM product_bnf_cmd_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_trace },
  { TERM_TERMINAL_TYPE, &symbol_off },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_trace_off }
};

TERM product_bnf_cmd_5[] = {
  { TERM_TERMINAL_TYPE, &symbol_timing },
  { TERM_TERMINAL_TYPE, &symbol_on },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_timing_on }
};

TERM product_bnf_cmd_6[] = {
  { TERM_TERMINAL_TYPE, &symbol_timing },
  { TERM_TERMINAL_TYPE, &symbol_off },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_timing_off }
};

TERM product_bnf_cmd_7[] = {
  { TERM_TERMINAL_TYPE, &symbol_compile },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_compile }
};

TERM product_bnf_cmd_8[] = {
  { TERM_TERMINAL_TYPE, &symbol_question },
  { TERM_TERMINAL_TYPE, &symbol_question },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_list }
};

/* 
 * ----------------------------------------------------------------------
 * Grammar: Extended Backus Naur Form (EBNF)
 *
 * <ebnf> ::= < <identifier> > ::= <ebnf_product> <eoln> @ bnf_first_product
 *         |  | <ebnf_product> <eoln> @ bnf_next_product
 *         |  <bnf_cmd> <eoln>
 *         |  #
 *
 * <ebnf_product> ::= <ebnf_term+>
 *
 * <ebnf_term> ::= < <identifier> > @ bnf_non_terminal
 *              |  [ < <identifier> > ] @ bnf_zero_or_one
 *              |  { < <identifier> > } @ bnf_zero_or_many
 *              |  ' ' <identifier> @ bnf_terminal
 *              |  <identifier> @ bnf_terminal
 *              |  @ <identifier> @ bnf_semantic
 *              |  ' ' <token> @ bnf_terminal
 *              |  <token> @ bnf_terminal
 *
 * ----------------------------------------------------------------------
 */

PRODUCT syntax_ebnf[] = {
  product_ebnf_1,
  product_ebnf_2,
  product_ebnf_3,
  product_ebnf_4,
  NULL
};

TERM product_ebnf_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_TERMINAL_TYPE, &symbol_assignment },
  { TERM_NON_TERMINAL_TYPE, &symbol_ebnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_first_product }
};

TERM product_ebnf_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_or },
  { TERM_NON_TERMINAL_TYPE, &symbol_ebnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_next_product }
};

TERM product_ebnf_3[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_bnf_cmd },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, NULL }
};

TERM product_ebnf_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_sharp },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_ebnf_product[] = {
  product_ebnf_product_1,
  NULL
};
      
TERM product_ebnf_product_1[] = {
  { TERM_ONE_OR_MANY_TYPE, &symbol_ebnf_term },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_ebnf_term[] = {
  product_ebnf_term_1,
  product_ebnf_term_2,
  product_ebnf_term_3,
  product_ebnf_term_4,
  product_ebnf_term_5,
  product_ebnf_term_6,
  product_ebnf_term_7,
  product_ebnf_term_8,
  NULL
};
      
TERM product_ebnf_term_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_non_terminal }
};

TERM product_ebnf_term_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_left_bracket },
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_TERMINAL_TYPE, &symbol_right_bracket },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_zero_or_one }
};

TERM product_ebnf_term_3[] = {
  { TERM_TERMINAL_TYPE, &symbol_left_curle },
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_TERMINAL_TYPE, &symbol_right_curle },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_zero_or_many }
};

TERM product_ebnf_term_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_ebnf_term_5[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_ebnf_term_6[] = {
  { TERM_TERMINAL_TYPE, &symbol_at },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_semantic }
};

TERM product_ebnf_term_7[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_ebnf_term_8[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

/* 
 * ----------------------------------------------------------------------
 * Grammar: eXtended Backus Naur Form (XBNF)
 *
 * <xbnf> ::= < <identifier> > ::= <xbnf_product> <eoln> @ bnf_first_product
 *         |  | <xbnf_product> <eoln> @ bnf_next_product
 *         |  <bnf_cmd> <eoln>
 *         |  # 
 *
 * <xbnf_product> ::= <xbnf_term+>
 *          
 * <xbnf_term> ::= < <identifier> > @ bnf_non_terminal
 *             |  < <identifier> ? > @ bnf_zero_or_one
 *             |  < <identifier> * > @ bnf_zero_or_many
 *             |  < <identifier> + > @ bnf_one_or_many
 *             |  ' <identifier> @ bnf_terminal
 *             |  <identifier> @ bnf_terminal
 *             |  @ <identifier> @ bnf_semantic
 *             |  ' ' <token> @ bnf_terminal
 *             |  <token> @ bnf_terminal
 *
 * ----------------------------------------------------------------------
 */

PRODUCT syntax_xbnf[] = {
  product_xbnf_1,
  product_xbnf_2,
  product_xbnf_3,
  product_xbnf_4,
  NULL
};

TERM product_xbnf_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_TERMINAL_TYPE, &symbol_assignment },
  { TERM_NON_TERMINAL_TYPE, &symbol_xbnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_first_product }
};

TERM product_xbnf_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_or },
  { TERM_NON_TERMINAL_TYPE, &symbol_xbnf_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_next_product }
};

TERM product_xbnf_3[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_bnf_cmd },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, NULL }
};

TERM product_xbnf_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_sharp },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_xbnf_product[] = {
  product_xbnf_product_1,
  NULL
};
      
TERM product_xbnf_product_1[] = {
  { TERM_ONE_OR_MANY_TYPE, &symbol_xbnf_term },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_xbnf_term[] = {
  product_xbnf_term_1,
  product_xbnf_term_2,
  product_xbnf_term_3,
  product_xbnf_term_4,
  product_xbnf_term_5,
  product_xbnf_term_6,
  product_xbnf_term_7,
  product_xbnf_term_8,
  product_xbnf_term_9,
  NULL
};
      
TERM product_xbnf_term_1[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_non_terminal }
};

TERM product_xbnf_term_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_question },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_zero_or_one }
};

TERM product_xbnf_term_3[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_times },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_zero_or_many }
};

TERM product_xbnf_term_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_less_than },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_plus },
  { TERM_TERMINAL_TYPE, &symbol_greater_than },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_one_or_many }
};

TERM product_xbnf_term_5[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_xbnf_term_6[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_xbnf_term_7[] = {
  { TERM_TERMINAL_TYPE, &symbol_at },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_semantic }
};

TERM product_xbnf_term_8[] = {
  { TERM_TERMINAL_TYPE, &symbol_quote },
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_xbnf_term_9[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_token },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

/* 
 * ----------------------------------------------------------------------
 * Grammar: YACC Meta Grammar
 *
 * <yacc> ::= <identifier> : <yacc_product> <eoln> @ bnf_first_product
 *         |  ' | <yacc_product> <eoln> @ bnf_next_product
 *         |  ; <eoln> 
 *         |  <bnf_cmd> <eoln>
 *         |  #
 *
 * <yacc_product> ::= <yacc_term+>
 * 
 * <yacc_term> ::= <string> @ bnf_terminal
 *	        |  ' @ <identifier> @ bnf_semantic
 *              |  <identifier> ? @ bnf_zero_or_one
 *              |  <identifier> * @ bnf_zero_or_many
 *              |  <identifier> + @ bnf_one_or_many
 *              |  <identifier> @ bnf_non_terminal
 *
 * ----------------------------------------------------------------------
 */

PRODUCT syntax_yacc[] = {
  product_yacc_1,
  product_yacc_2,
  product_yacc_3,
  product_yacc_4,
  NULL
};

TERM product_yacc_1[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_colon },
  { TERM_NON_TERMINAL_TYPE, &symbol_yacc_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_first_product }
};

TERM product_yacc_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_or },
  { TERM_NON_TERMINAL_TYPE, &symbol_yacc_product },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_next_product }
};

TERM product_yacc_3[] = {
  { TERM_TERMINAL_TYPE, &symbol_semicolon },
  { TERM_NON_TERMINAL_TYPE, &symbol_eoln },
  { TERM_PRODUCT_END_TYPE, NULL }
};

TERM product_yacc_4[] = {
  { TERM_TERMINAL_TYPE, &symbol_sharp },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_yacc_product[] = {
  product_yacc_product_1,
  NULL
};
      
TERM product_yacc_product_1[] = {
  { TERM_ONE_OR_MANY_TYPE, &symbol_yacc_term },
  { TERM_PRODUCT_END_TYPE, NULL }
};

PRODUCT syntax_yacc_term[] = {
  product_yacc_term_1,
  product_yacc_term_2,
  product_yacc_term_3,
  product_yacc_term_4,
  product_yacc_term_5,
  product_yacc_term_6,
  NULL
};
      
TERM product_yacc_term_1[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_string },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_terminal }
};

TERM product_yacc_term_2[] = {
  { TERM_TERMINAL_TYPE, &symbol_at },
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_semantic }
};

TERM product_yacc_term_3[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_question },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_zero_or_one }
};

TERM product_yacc_term_4[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_plus },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_one_or_many }
};

TERM product_yacc_term_5[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_TERMINAL_TYPE, &symbol_plus },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_one_or_many }
};

TERM product_yacc_term_6[] = {
  { TERM_NON_TERMINAL_TYPE, &symbol_identifier },
  { TERM_PRODUCT_END_TYPE, &symbol_bnf_non_terminal }
};

/* 
 * ----------------------------------------------------------------------
 * Section: Extended Backup-Naur Form semantics and local variables
 * ----------------------------------------------------------------------
 */

static DICTIONARY bnf_dictionary = &symbol_bnf;
static SYMBOL *bnf_symbol = NULL;
static TERM bnf_term[32];
static int bnf_terms = 0;
static int bnf_compile_id = 256;

SYMBOL *bnf_symbol_lookup(ENVIRONMENT *env, DICTIONARY *dictionary)
{
  char name[128];
  SYMBOL *symbol;
  VALUE *v;
  
  value_pop(env, v);
  name[0] = 0;
  strncat(name, v->view.as_string.buffer, v->view.as_string.count);
  symbol = symbol_lookup(name, &bnf_compile_id, TRUE, dictionary);

  return (symbol);
}

SYMBOL *bnf_generate(TERM_TYPE type, ENVIRONMENT *env)
{
  SYMBOL *symbol;
  int n;

  symbol = bnf_symbol_lookup(env, &bnf_dictionary);
  if (type == TERM_NON_TERMINAL_TYPE && symbol->parse == NULL)
    symbol->parse = parse_undefined;
  n = bnf_terms;
  bnf_term[n].type = type;
  bnf_term[n].symbol = symbol;
  bnf_terms++;

  return (symbol);
}

void bnf_display(SYMBOL *symbol)
{
  PRODUCT *product;
  TERM *term;
  int indent;
  int n;
  
  /* Dump syntax definition */
  if (symbol->syntax != NULL) {
    printf("<%s> ::=", symbol->name);
    indent = strlen(symbol->name) + 4;
    for (product = symbol->syntax; *product; product++) {
      for (term = *product; term->type != TERM_PRODUCT_END_TYPE; term++) {
	switch (term->type) {
	  case TERM_TERMINAL_TYPE:
	    if (!strcmp(term->symbol->name, "@"))
	      printf(" '");
	    printf(" %s", term->symbol->name);
	    break;
	  case TERM_NON_TERMINAL_TYPE:
	    printf(" <%s>", term->symbol->name);
	    break;
	  case TERM_ZERO_OR_ONE_TYPE:
	    printf(" <%s?>", term->symbol->name);
	    break;
	  case TERM_ZERO_OR_MANY_TYPE:
	    printf(" <%s*>", term->symbol->name);
	    break;
	  case TERM_ONE_OR_MANY_TYPE:
	    printf(" <%s+>", term->symbol->name);
	    break;
	  default:
		break;
	}
      }
      if (term->symbol != NULL)
	printf(" @ %s", term->symbol->name);
      printf("\n");
      if (product[1] != NULL) {
	for (n = indent; n != 0; n--)
	  putchar(' ');
	printf("| ");
      }
    }	
  }

  /* Dump semantic definition */
  if (symbol->semantic != NULL)
    printf("extern void semantic_%s(ENVIRONMENT*);\n", symbol->name);

  /* Dump parse definition */
  if (symbol->parse != parse_syntax && symbol->parse != NULL)
    printf("extern int parse_%s(SYMBOL*, char**, VALUE**);\n", symbol->name);
}

void bnf_list(void)
{
  SYMBOL *symbol;
  int width;
  int len;
  int col;
  int i;
  
  for (width = 0, symbol = bnf_dictionary; symbol != NULL; symbol = symbol->next) {
    len = strlen(symbol->name);
    if (len > width)
     width = len;
  }

  col = 80 / (width + 2);
  symbol = bnf_dictionary;
  while (symbol != NULL) {
    for (i = 0; i < col && symbol != NULL; i++) {
      printf("%s", symbol->name);
      len = width + 2 - strlen(symbol->name);
      while (len--)
	putchar(' ');
      symbol = symbol->next;
    }
    putchar('\n');
  }
}

void bnf_compile(void)
{
  SYMBOL *symbol;
  PRODUCT *product;
  TERM *term;
  int n;
  
  /* The output might be used by C++ */
  printf("#if defined(__cplusplus)\n");
  printf("extern \"C\" {\n");
  printf("#endif\n");

  /* Compile forward references for all symbols */
  for (symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next) {
    printf("extern SYMBOL ");
    symbol_print_name(symbol);
    printf(";\n");
  }

  /* Compile forward references for grammar */
  for (symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next) {
    if (symbol->syntax != NULL) {
      printf("extern PRODUCT syntax_%s[];\n", symbol->name);
      for (n = 1, product = symbol->syntax; *product != NULL; product++, n++)
	printf("extern TERM product_%s_%d[];\n", symbol->name, n);
    }
  }

  /* Compile forward references on parse functions */
  for (symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next) {
    if (symbol->parse == parse_undefined) {
      printf("extern int parse_%s(SYMBOL*, char**, VALUE**);\n", symbol->name);
    }
  }

  /* Compile forward references on semantics */
  for (symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next) {
    if (symbol->semantic != NULL) {
      printf("extern void semantic_%s(ENVIRONMENT*);\n", symbol->name);
    }
  }

  /* Compile defined symbols */
  for (n= 1, symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next, n++) {
    printf("SYMBOL ");
    symbol_print_name(symbol);
    printf(" = {\n");
    printf("  NULL, \"%s\", ", symbol->name);
    printf("%d, ", symbol->id);
    if (symbol->syntax != NULL) {
      printf("syntax_%s, ", symbol->name);
      printf("parse_syntax, ");
    } else {
      printf("NULL, ");
      if (symbol->parse == parse_undefined)
	printf("parse_%s, ", symbol->name);
      else
	printf("parse_syntax, ");
    }
    if (symbol->semantic != NULL)
      printf("semantic_%s\n", symbol->name);
    else
      printf("NULL\n");
    printf("};\n");
  }

  /* Compile grammar */
  for (symbol = bnf_dictionary; symbol != &symbol_bnf; symbol = symbol->next) {
    if (symbol->syntax != NULL) {

      printf("PRODUCT syntax_%s[] = {\n", symbol->name);
      for (n = 1, product = symbol->syntax; *product != NULL; product++, n++)
	printf("  product_%s_%d,\n", symbol->name, n);
      printf("  NULL\n");
      printf("};\n");

      for (n = 1, product = symbol->syntax; *product != NULL; product++, n++) {
	printf("TERM product_%s_%d[] = {\n", symbol->name, n);
	for (term = *product; term->type != TERM_PRODUCT_END_TYPE; term++) {
	  switch (term->type) {
	    case TERM_TERMINAL_TYPE:
	      printf("  { TERM_TERMINAL_TYPE, &");
	      symbol_print_name(term->symbol);
	      printf(" }, \n");
	      break;
	    case TERM_NON_TERMINAL_TYPE:
	      printf("  { TERM_NON_TERMINAL_TYPE, &");
	      symbol_print_name(term->symbol);
	      printf(" }, \n");
	      break;
	    case TERM_ZERO_OR_ONE_TYPE:
	      printf("  { TERM_ZERO_OR_ONE_TYPE, &");
	      symbol_print_name(term->symbol);
	      printf(" }, \n");
	      break;
	    case TERM_ZERO_OR_MANY_TYPE:
	      printf("  { TERM_ZERO_OR_MANY_TYPE, &");
	      symbol_print_name(term->symbol);
	      printf(" }, \n");
	      break;
	    case TERM_ONE_OR_MANY_TYPE:
	      printf("  { TERM_ONE_OR_MANY_TYPE, &");
	      symbol_print_name(term->symbol);
	      printf(" }, \n");
	      break;
	  }
	}
	printf("  { TERM_PRODUCT_END_TYPE, ");
	if (term->symbol != NULL) {
	  printf("&");
	  symbol_print_name(term->symbol);
	} else
	  printf("NULL");
	printf(" }, \n");
	printf("};\n");
      }
    }
  }

  /* The output might be used by C++ */
  printf("#if defined(__cplusplus)\n");
  printf("};\n");
  printf("#endif\n");
}

void semantic_bnf_first_product(ENVIRONMENT *env)
{
  SYMBOL *symbol = bnf_symbol_lookup(env, &bnf_dictionary);
  PRODUCT *product;
  int n;
  
  /* Capture the definition symbol */
  bnf_symbol = symbol;
  if (symbol->syntax != NULL) {
    printf("%s: syntax redefined\n", symbol->name);
  }

  /* Allocate initial product vector */
  symbol->syntax = product = (PRODUCT *) malloc(2 * sizeof(PRODUCT));
  symbol->parse = parse_syntax;
  
  /* Terminate product without semantics */
  n = bnf_terms;
  if (bnf_term[n - 1].type != TERM_PRODUCT_END_TYPE) {
    bnf_term[n].type = TERM_PRODUCT_END_TYPE;
    bnf_term[n].symbol = NULL;
    bnf_terms++;
  }

  /* Build product */
  product[0] = (TERM *) malloc(bnf_terms * sizeof(TERM));
  memcpy(product[0], bnf_term, bnf_terms * sizeof(TERM));
  product[1] = NULL;
  bnf_terms = 0;
}

void semantic_bnf_next_product(ENVIRONMENT *env)
{
  SYMBOL *symbol = bnf_symbol;
  PRODUCT *product;
  int n;
  
  if (symbol == NULL) {
    return;
  }
  
  n = bnf_terms;
  if (bnf_term[n - 1].type != TERM_PRODUCT_END_TYPE) {
    bnf_term[n].type = TERM_PRODUCT_END_TYPE;
    bnf_term[n].symbol = NULL;
    bnf_terms++;
  }
  product = symbol->syntax;
  n = 1;
  if (product != NULL)
    while (*product != NULL) {
      product++;
      n++;
    }
  product = (PRODUCT *) realloc(symbol->syntax, (n + 1) * sizeof(PRODUCT));
  symbol->syntax = product;
  product[n - 1] = (TERM *) malloc(bnf_terms * sizeof(TERM));
  memcpy(product[n - 1], bnf_term, bnf_terms * sizeof(TERM));
  product[n] = NULL;
  bnf_terms = 0;
}

void semantic_bnf_non_terminal(ENVIRONMENT *env)
{
  bnf_generate(TERM_NON_TERMINAL_TYPE, env);
}

void semantic_bnf_zero_or_one(ENVIRONMENT *env)
{
  bnf_generate(TERM_ZERO_OR_ONE_TYPE, env);
}

void semantic_bnf_zero_or_many(ENVIRONMENT *env)
{
  bnf_generate(TERM_ZERO_OR_MANY_TYPE, env);
}

void semantic_bnf_one_or_many(ENVIRONMENT *env)
{
  bnf_generate(TERM_ONE_OR_MANY_TYPE, env);
}

void semantic_bnf_terminal(ENVIRONMENT *env)
{
  bnf_generate(TERM_TERMINAL_TYPE, env);
}

void semantic_bnf_semantic(ENVIRONMENT *env)
{
  SYMBOL *symbol = bnf_generate(TERM_PRODUCT_END_TYPE, env);
  if (symbol->semantic == NULL)
    symbol->semantic = (SEMANTIC) 1;
}

SYMBOL *main_symbol;

void semantic_bnf_execute(ENVIRONMENT *env)
{
  main_symbol = bnf_symbol_lookup(env, &bnf_dictionary);
  if (!strcmp(main_symbol->name, "shell"))
    exit(0);
}

void semantic_bnf_display(ENVIRONMENT *env)
{
  bnf_display(bnf_symbol_lookup(env, &bnf_dictionary));
}

void semantic_bnf_list(ENVIRONMENT *env)
{
  bnf_list();
}

void semantic_bnf_compile(ENVIRONMENT *env)
{
  bnf_compile();
}

void semantic_bnf_trace_on(ENVIRONMENT *env)
{
  parse_tracing = TRUE;
  parse_indent = 0;
}

void semantic_bnf_trace_off(ENVIRONMENT *env)
{
  parse_tracing = FALSE;
}

void semantic_bnf_timing_on(ENVIRONMENT *env)
{
  parse_timing = TRUE;
}

void semantic_bnf_timing_off(ENVIRONMENT *env)
{
  parse_timing = FALSE;
}


