/* A Bison parser, made by GNU Bison 2.6.1.  */

/* Bison interface for Yacc-like parsers in C

      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef CSIM_Y_TAB_H
# define CSIM_Y_TAB_H
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int csim_debug;
#endif

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
     ONCLICK = 278,
     ACCESSKEY = 279,
     TABINDEX = 280,
     AUTHOR = 281,
     DESCRIPTION = 282,
     BEGIN_COMMENT = 283,
     END_COMMENT = 284,
     FLOAT = 285,
     STRING = 286
   };
#endif
/* Tokens.  */
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
#define ONCLICK 278
#define ACCESSKEY 279
#define TABINDEX 280
#define AUTHOR 281
#define DESCRIPTION 282
#define BEGIN_COMMENT 283
#define END_COMMENT 284
#define FLOAT 285
#define STRING 286



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2049 of yacc.c  */
#line 49 "imap_csim.y"

  int val;
  double value;
  char *id;


/* Line 2049 of yacc.c  */
#line 120 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE csim_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int csim_parse (void *YYPARSE_PARAM);
#else
int csim_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int csim_parse (void);
#else
int csim_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !CSIM_Y_TAB_H  */
