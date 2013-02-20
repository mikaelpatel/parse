/* $Id: bnf.h 5 2009-08-21 21:47:38Z Mikael Patel $
 * ----------------------------------------------------------------------
 * Name
 *   bnf - extended bnf for the top down parse machine
 *  
 * Description:	
 *   General top down parse machine with meta grammar for Extended
 *   Backus Naur Form (EBNF). 
 *
 * Copyright (C) 1984-1994, Mikael Patel, Linkoping, Sweden.
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
 * ---------------------------------------------------------------------
 */

#if !defined(BNF_H)
#define BNF_H

extern SYMBOL *main_symbol;

extern SYMBOL symbol_bnf;
extern SYMBOL symbol_ebnf;
extern SYMBOL symbol_xbnf;
extern SYMBOL symbol_yacc;

#endif /* BNF_H */
