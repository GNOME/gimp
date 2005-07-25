/* A Bison parser, made by GNU Bison 2.0.  */

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
     IMG = 258,
     SRC = 259,
     WIDTH = 260,
     HEIGHT = 261,
     BORDER = 262,
     USEMAP = 263,
     START_MAP = 264,
     END_MAP = 265,
     NAME = 266,
     AREA = 267,
     SHAPE = 268,
     COORDS = 269,
     ALT = 270,
     HREF = 271,
     NOHREF = 272,
     TARGET = 273,
     ONMOUSEOVER = 274,
     ONMOUSEOUT = 275,
     ONFOCUS = 276,
     ONBLUR = 277,
     AUTHOR = 278,
     DESCRIPTION = 279,
     BEGIN_COMMENT = 280,
     END_COMMENT = 281,
     FLOAT = 282,
     STRING = 283
   };
#endif
#define IMG 258
#define SRC 259
#define WIDTH 260
#define HEIGHT 261
#define BORDER 262
#define USEMAP 263
#define START_MAP 264
#define END_MAP 265
#define NAME 266
#define AREA 267
#define SHAPE 268
#define COORDS 269
#define ALT 270
#define HREF 271
#define NOHREF 272
#define TARGET 273
#define ONMOUSEOVER 274
#define ONMOUSEOUT 275
#define ONFOCUS 276
#define ONBLUR 277
#define AUTHOR 278
#define DESCRIPTION 279
#define BEGIN_COMMENT 280
#define END_COMMENT 281
#define FLOAT 282
#define STRING 283




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 49 "imap_csim.y"
typedef union YYSTYPE {
  int val;
  double value;
  char id[1024];		/* Large enough to hold all polygon points! */
} YYSTYPE;
/* Line 1318 of yacc.c.  */
#line 99 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE csim_lval;



