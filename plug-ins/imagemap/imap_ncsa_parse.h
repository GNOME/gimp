/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     RECTANGLE = 258,
     POLYGON = 259,
     CIRCLE = 260,
     DEFAULT = 261,
     AUTHOR = 262,
     TITLE = 263,
     DESCRIPTION = 264,
     BEGIN_COMMENT = 265,
     FLOAT = 266,
     LINK = 267,
     COMMENT = 268
   };
#endif
#define RECTANGLE 258
#define POLYGON 259
#define CIRCLE 260
#define DEFAULT 261
#define AUTHOR 262
#define TITLE 263
#define DESCRIPTION 264
#define BEGIN_COMMENT 265
#define FLOAT 266
#define LINK 267
#define COMMENT 268




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 46 "imap_ncsa.y"
typedef union YYSTYPE {
   int val;
   double value;
   char id[256];
} YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 69 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE ncsa_lval;



