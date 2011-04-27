%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <string.h>

#include <glib.h>

#include "imap_ncsa_parse.h"

#ifdef FLEX_SCANNER
#define YY_NO_UNPUT
#endif /* FLEX_SCANNER */

%}

%option noyywrap
%option noinput
%option nounput

DIGIT	[0-9]
ID	[a-zA-Z_][a-zA-Z0-9_\-]*
WS	[ \t\n]+

%x imap_link
%x comment

%%

#\$AUTHOR:	       		{
				   BEGIN(comment);
				   return AUTHOR;
				}

#\$TITLE:	       		{
				   BEGIN(comment);
				   return TITLE;
				}

#\$DESCRIPTION:	       		{
				   BEGIN(comment);
				   return DESCRIPTION;
				}

#				{
				   BEGIN(comment);
				   return BEGIN_COMMENT;
				}

<comment>.*	 		{
				   BEGIN(INITIAL);
   				   ncsa_lval.id = g_strndup (yytext, yyleng);
   				   return COMMENT;
				}

RECT				{
				   BEGIN(imap_link);
   				   return RECTANGLE;
				}

CIRCLE				{
				   BEGIN(imap_link);
				   return CIRCLE;
				}

POLY				{
				   BEGIN(imap_link);
				   return POLYGON;
				}

DEFAULT				{
				   BEGIN(imap_link);
				   return DEFAULT;
				}

<imap_link>[^ ,\t\n]+		{
   				   BEGIN(INITIAL);
   				   ncsa_lval.id = g_strndup (yytext, yyleng);
				   return LINK;
				}

-?{DIGIT}*"."?{DIGIT}*([Ee][-+]?{DIGIT}*)? {
                                   ncsa_lval.value = g_ascii_strtod (yytext, NULL);
				   return FLOAT;
				}

{WS}				; /* Eat white space */

.				return *yytext;

%%


