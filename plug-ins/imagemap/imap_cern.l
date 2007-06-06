%{
/*
 * This is a plug-in for GIMP.
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

#include <string.h>

#include <glib.h>

#include "imap_cern_parse.h"

#ifdef FLEX_SCANNER
#define YY_NO_UNPUT
#endif /* FLEX_SCANNER */

%}

%option noyywrap

DIGIT	[0-9]
ID	[a-zA-Z_][a-zA-Z0-9_\-]*
WS	[ \t\n]+

%x comment

%%

RECT{WS}\(4096,4096\){WS}\(4096,4096\).*#\$AUTHOR: {
				   BEGIN(comment);
				   return AUTHOR;
				}

RECT{WS}\(4096,4096\){WS}\(4096,4096\).*#\$DESCRIPTION: {
				   BEGIN(comment);
				   return DESCRIPTION;
				}

RECT{WS}\(4096,4096\){WS}\(4096,4096\)   {
				   BEGIN(comment);
				   return BEGIN_COMMENT;
				}

<comment>.*$	 		{
				   BEGIN(INITIAL);
   				   strcpy(cern_lval.id, yytext);
   				   return COMMENT;
				}

RECT				return RECTANGLE;

CIRC				return CIRCLE;

POLY				return POLYGON;

DEFAULT				return DEFAULT;

[^ ,\t\n]+$			{
   				   strcpy(cern_lval.id, yytext);
				   return LINK;
				}

-?{DIGIT}*"."?{DIGIT}*([Ee][-+]?{DIGIT}*)? {
                                   cern_lval.value = g_ascii_strtod (yytext, NULL);
				   return FLOAT;
				}

{WS}				; /* Eat white space */

.				return *yytext;

%%


