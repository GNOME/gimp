%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2000 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include "imap_csim_parse.h"

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

%x quoted_string
%x comment

%%

\<!--\ #$AUTHOR:	       	{
				   BEGIN(comment);
				   return AUTHOR;
				}

\<!--\ #$DESCRIPTION:	       	{
				   BEGIN(comment);
				   return DESCRIPTION;
				}

\<!-- 				{
				   BEGIN(comment);
				   return BEGIN_COMMENT;
				}

<comment>--\>			{
				   BEGIN(INITIAL);
				   return END_COMMENT;
			        }

<comment>.*/--\> 		{
				   csim_lval.id = g_strndup (yytext, yyleng);
   				   return STRING;
				}

IMG				return IMG;

SRC				return SRC;

WIDTH			     	return WIDTH;

HEIGHT			       	return HEIGHT;

BORDER				return BORDER;

USEMAP				return USEMAP;

MAP				return START_MAP;

\/MAP		       		return END_MAP;

NAME				return NAME;

AREA				return AREA;

SHAPE				return SHAPE;

COORDS				return COORDS;

TARGET				return TARGET;

ONMOUSEOVER			return ONMOUSEOVER;

ONMOUSEOUT			return ONMOUSEOUT;

ONFOCUS				return ONFOCUS;

ONBLUR				return ONBLUR;

ALT				return ALT;

HREF				return HREF;

NOHREF				return NOHREF;

\"				{
				   BEGIN(quoted_string);
				}

<quoted_string>\"		{
				   BEGIN(INITIAL);
				   return STRING;
				}

<quoted_string>[^\"]*		{
				  csim_lval.id = g_strndup (yytext, yyleng);
				}

-?{DIGIT}*"."?{DIGIT}*([Ee][-+]?{DIGIT}*)? {
                                   csim_lval.value = g_ascii_strtod (yytext, NULL);
				   return FLOAT;
				}

{WS}				; /* Eat white space */

.				return *yytext;

%%

