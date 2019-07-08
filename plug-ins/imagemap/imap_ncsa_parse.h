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

#ifndef NCSA_Y_TAB_H
# define NCSA_Y_TAB_H
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int ncsa_debug;
#endif

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
/* Tokens.  */
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



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2049 of yacc.c  */
#line 45 "imap_ncsa.y"

   int val;
   double value;
   char *id;


/* Line 2049 of yacc.c  */
#line 90 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE ncsa_lval;

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int ncsa_parse (void *YYPARSE_PARAM);
#else
int ncsa_parse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int ncsa_parse (void);
#else
int ncsa_parse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !NCSA_Y_TAB_H  */
