
/*  A Bison parser, made from imap_csim.y
 by  GNU Bison version 1.25
  */

#define YYBISON 1  /* Identify Bison output.  */

#define yyparse csim_parse
#define yylex csim_lex
#define yyerror csim_error
#define yylval csim_lval
#define yychar csim_char
#define yydebug csim_debug
#define yynerrs csim_nerrs
#define	IMG	258
#define	SRC	259
#define	WIDTH	260
#define	HEIGHT	261
#define	BORDER	262
#define	USEMAP	263
#define	START_MAP	264
#define	END_MAP	265
#define	NAME	266
#define	AREA	267
#define	SHAPE	268
#define	COORDS	269
#define	ALT	270
#define	HREF	271
#define	NOHREF	272
#define	TARGET	273
#define	ONMOUSEOVER	274
#define	ONMOUSEOUT	275
#define	ONFOCUS	276
#define	ONBLUR	277
#define	AUTHOR	278
#define	DESCRIPTION	279
#define	BEGIN_COMMENT	280
#define	END_COMMENT	281
#define	FLOAT	282
#define	STRING	283

#line 1 "imap_csim.y"

/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdlib.h>
#include <strings.h>

#include "gtk/gtk.h"

#include "imap_circle.h"
#include "imap_file.h"
#include "imap_main.h"
#include "imap_polygon.h"
#include "imap_rectangle.h"
#include "imap_string.h"

extern int csim_lex();
extern int csim_restart();
static void csim_error(char* s);

static enum {UNDEFINED, RECTANGLE, CIRCLE, POLYGON} current_type;
static Object_t *current_object;
static MapInfo_t *_map_info;


#line 47 "imap_csim.y"
typedef union {
   int val;
   double value;
   char id[256];
} YYSTYPE;
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		98
#define	YYFLAG		-32768
#define	YYNTBASE	32

#define YYTRANSLATE(x) ((unsigned)(x) <= 283 ? yytranslate[x] : 59)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    29,
    30,    31,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     6,    14,    15,    18,    20,    22,    26,    30,    34,
    38,    42,    49,    50,    53,    55,    57,    59,    63,    67,
    71,    72,    75,    80,    81,    84,    86,    88,    90,    92,
    94,    96,    98,   100,   102,   104,   108,   112,   116,   118,
   122,   126,   130,   134,   138,   142
};

static const short yyrhs[] = {    33,
    38,    39,    44,    58,     0,    29,     3,     4,    30,    28,
    34,    31,     0,     0,    34,    35,     0,    36,     0,    37,
     0,     7,    30,    27,     0,     8,    30,    28,     0,    15,
    30,    28,     0,     5,    30,    27,     0,     6,    30,    27,
     0,    29,     9,    11,    30,    28,    31,     0,     0,    39,
    40,     0,    42,     0,    43,     0,    41,     0,    25,    28,
    26,     0,    23,    28,    26,     0,    24,    28,    26,     0,
     0,    44,    45,     0,    29,    12,    46,    31,     0,     0,
    46,    47,     0,    48,     0,    49,     0,    50,     0,    51,
     0,    52,     0,    53,     0,    54,     0,    55,     0,    56,
     0,    57,     0,    13,    30,    28,     0,    14,    30,    28,
     0,    16,    30,    28,     0,    17,     0,    15,    30,    28,
     0,    18,    30,    28,     0,    19,    30,    28,     0,    20,
    30,    28,     0,    21,    30,    28,     0,    22,    30,    28,
     0,    29,    10,    31,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    62,    65,    71,    72,    75,    76,    77,    78,    79,    82,
    88,    94,   100,   101,   104,   105,   106,   109,   114,   121,
   131,   132,   135,   142,   143,   146,   147,   148,   149,   150,
   151,   152,   153,   154,   155,   158,   175,   232,   242,   247,
   253,   259,   265,   271,   277,   283
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","IMG","SRC",
"WIDTH","HEIGHT","BORDER","USEMAP","START_MAP","END_MAP","NAME","AREA","SHAPE",
"COORDS","ALT","HREF","NOHREF","TARGET","ONMOUSEOVER","ONMOUSEOUT","ONFOCUS",
"ONBLUR","AUTHOR","DESCRIPTION","BEGIN_COMMENT","END_COMMENT","FLOAT","STRING",
"'<'","'='","'>'","csim_file","image","image_tags","image_tag","image_width",
"image_height","start_map","comment_lines","comment_line","real_comment","author_line",
"description_line","area_list","area","tag_list","tag","shape_tag","coords_tag",
"href_tag","nohref_tag","alt_tag","target_tag","onmouseover_tag","onmouseout_tag",
"onfocus_tag","onblur_tag","end_map", NULL
};
#endif

static const short yyr1[] = {     0,
    32,    33,    34,    34,    35,    35,    35,    35,    35,    36,
    37,    38,    39,    39,    40,    40,    40,    41,    42,    43,
    44,    44,    45,    46,    46,    47,    47,    47,    47,    47,
    47,    47,    47,    47,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    58
};

static const short yyr2[] = {     0,
     5,     7,     0,     2,     1,     1,     3,     3,     3,     3,
     3,     6,     0,     2,     1,     1,     1,     3,     3,     3,
     0,     2,     4,     0,     2,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     3,     3,     3,     1,     3,
     3,     3,     3,     3,     3,     3
};

static const short yydefact[] = {     0,
     0,     0,     0,     0,    13,     0,     0,    21,     0,     0,
     0,     0,     0,    14,    17,    15,    16,     0,     3,     0,
     0,     0,     0,     0,    22,     1,     0,     0,    19,    20,
    18,     0,    24,     0,     0,     0,     0,     0,     2,     4,
     5,     6,    12,    46,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,    39,     0,     0,     0,     0,     0,
    23,    25,    26,    27,    28,    29,    30,    31,    32,    33,
    34,    35,    10,    11,     7,     8,     9,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    36,    37,    40,    38,
    41,    42,    43,    44,    45,     0,     0,     0
};

static const short yydefgoto[] = {    96,
     2,    27,    40,    41,    42,     5,     8,    14,    15,    16,
    17,    18,    25,    45,    62,    63,    64,    65,    66,    67,
    68,    69,    70,    71,    72,    26
};

static const short yypact[] = {   -21,
    18,    -7,    19,    15,-32768,     0,    14,   -19,    -1,     1,
     4,     5,     6,-32768,-32768,-32768,-32768,     7,-32768,     9,
     2,    12,    13,    -3,-32768,-32768,    -5,    10,-32768,-32768,
-32768,    11,-32768,    16,    17,    20,    21,    22,-32768,-32768,
-32768,-32768,-32768,-32768,    -2,     8,    26,    27,    28,    29,
    25,    30,    31,    32,-32768,    33,    34,    35,    36,    37,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,    40,    41,    42,
    43,    44,    45,    46,    47,    48,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    49,    58,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768
};


#define	YYLAST		76


static const short yytable[] = {    34,
    35,    36,    37,    11,    12,    13,    32,     1,    33,    38,
    51,    52,    53,    54,    55,    56,    57,    58,    59,    60,
     3,     4,     6,     7,    10,    39,    19,    29,    61,     9,
    20,    21,    22,    23,    73,    24,    28,    30,    31,     0,
    43,    44,     0,     0,     0,    46,    47,     0,    97,    48,
    49,    50,    74,    75,    78,    76,    77,    98,     0,    79,
    80,    81,    82,    83,    84,    85,    86,    87,    88,    89,
    90,    91,    92,    93,    94,    95
};

static const short yycheck[] = {     5,
     6,     7,     8,    23,    24,    25,    10,    29,    12,    15,
    13,    14,    15,    16,    17,    18,    19,    20,    21,    22,
     3,    29,     4,     9,    11,    31,    28,    26,    31,    30,
    30,    28,    28,    28,    27,    29,    28,    26,    26,    -1,
    31,    31,    -1,    -1,    -1,    30,    30,    -1,     0,    30,
    30,    30,    27,    27,    30,    28,    28,     0,    -1,    30,
    30,    30,    30,    30,    30,    30,    30,    28,    28,    28,
    28,    28,    28,    28,    28,    28
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 196 "bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 2:
#line 66 "imap_csim.y"
{
		   g_strreplace(&_map_info->image_name, yyvsp[-2].id);
		;
    break;}
case 7:
#line 77 "imap_csim.y"
{;
    break;}
case 8:
#line 78 "imap_csim.y"
{;
    break;}
case 9:
#line 79 "imap_csim.y"
{;
    break;}
case 10:
#line 83 "imap_csim.y"
{
		   _map_info->old_image_width = (gint) yyvsp[0].value;
		;
    break;}
case 11:
#line 89 "imap_csim.y"
{
		   _map_info->old_image_height = (gint) yyvsp[0].value;
		;
    break;}
case 12:
#line 95 "imap_csim.y"
{
		   g_strreplace(&_map_info->title, yyvsp[-1].id);
		;
    break;}
case 18:
#line 110 "imap_csim.y"
{
		;
    break;}
case 19:
#line 115 "imap_csim.y"
{
		   g_strreplace(&_map_info->author, yyvsp[-1].id);

		;
    break;}
case 20:
#line 122 "imap_csim.y"
{
		   gchar *description;

		   description = g_strconcat(_map_info->description, yyvsp[-1].id, "\n", 
					     NULL);
		   g_strreplace(&_map_info->description, description);
		;
    break;}
case 23:
#line 136 "imap_csim.y"
{
		   if (current_type != UNDEFINED)
		      add_shape(current_object);
		;
    break;}
case 36:
#line 159 "imap_csim.y"
{
		   if (!g_strcasecmp(yyvsp[0].id, "RECT")) {
		      current_object = create_rectangle(0, 0, 0, 0);
		      current_type = RECTANGLE;
		   } else if (!g_strcasecmp(yyvsp[0].id, "CIRCLE")) {
		      current_object = create_circle(0, 0, 0);
		      current_type = CIRCLE;
		   } else if (!g_strcasecmp(yyvsp[0].id, "POLY")) {
		      current_object = create_polygon(NULL);
		      current_type = POLYGON;
		   } else if (!g_strcasecmp(yyvsp[0].id, "DEFAULT")) {
		      current_type = UNDEFINED;
		   }
		;
    break;}
case 37:
#line 176 "imap_csim.y"
{
		   char *p;
		   if (current_type == RECTANGLE) {
		      Rectangle_t *rectangle;

		      rectangle = ObjectToRectangle(current_object);
		      p = strtok(yyvsp[0].id, ",");
		      rectangle->x = atoi(p);
		      p = strtok(NULL, ",");
		      rectangle->y = atoi(p);
		      p = strtok(NULL, ",");
		      rectangle->width = atoi(p) - rectangle->x;
		      p = strtok(NULL, ",");
		      rectangle->height = atoi(p) - rectangle->y;
		   } else if (current_type == CIRCLE) {
		      Circle_t *circle;

		      circle = ObjectToCircle(current_object);
		      p = strtok(yyvsp[0].id, ",");
		      circle->x = atoi(p);
		      p = strtok(NULL, ",");
		      circle->y = atoi(p);
		      p = strtok(NULL, ",");
		      circle->r = atoi(p);
		   } else if (current_type == POLYGON) {
		      Polygon_t *polygon = ObjectToPolygon(current_object);
		      GList *points;
		      GdkPoint *point, *first;
		      gint x, y;

		      p = strtok(yyvsp[0].id, ",");
		      x = atoi(p);
		      p = strtok(NULL, ",");
		      y = atoi(p);
		      point = new_point(x, y);
		      points = g_list_append(NULL, (gpointer) point);

		      while(1) {
			 p = strtok(NULL, ",");
			 if (!p)
			    break;
			 x = atoi(p);
			 p = strtok(NULL, ",");
			 y = atoi(p);
			 point = new_point(x, y);
			 g_list_append(points, (gpointer) point);
		      }
		      /* Remove last point if duplicate */
		      first = (GdkPoint*) points->data;
		      if (first->x == point->x && first->y == point->y)
			 polygon_remove_last_point(polygon);
		      polygon->points = points;
		   }
		;
    break;}
case 38:
#line 233 "imap_csim.y"
{
		   if (current_type == UNDEFINED) {
		      g_strreplace(&_map_info->default_url, yyvsp[0].id);
		   } else {
		      object_set_url(current_object, yyvsp[0].id);
		   }
		;
    break;}
case 39:
#line 243 "imap_csim.y"
{
		;
    break;}
case 40:
#line 248 "imap_csim.y"
{
		   object_set_comment(current_object, yyvsp[0].id);
		;
    break;}
case 41:
#line 254 "imap_csim.y"
{
		   object_set_target(current_object, yyvsp[0].id);
		;
    break;}
case 42:
#line 260 "imap_csim.y"
{
		   object_set_mouse_over(current_object, yyvsp[0].id);
		;
    break;}
case 43:
#line 266 "imap_csim.y"
{
		   object_set_mouse_out(current_object, yyvsp[0].id);
		;
    break;}
case 44:
#line 272 "imap_csim.y"
{
		   object_set_focus(current_object, yyvsp[0].id);
		;
    break;}
case 45:
#line 278 "imap_csim.y"
{
		   object_set_blur(current_object, yyvsp[0].id);
		;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 498 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 286 "imap_csim.y"


static void 
csim_error(char* s)
{
   extern FILE *csim_in;
   csim_restart(csim_in);
}

gboolean
load_csim(const char* filename)
{
   gboolean status;
   extern FILE *csim_in;
   csim_in = fopen(filename, "r");
   if (csim_in) {
      _map_info = get_map_info();
      status = !csim_parse();
      fclose(csim_in);
   } else {
      status = FALSE;
   }
   return status;
}
