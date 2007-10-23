%{
/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2000 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include "imap_csim_parse.h"

#ifdef FLEX_SCANNER
#define YY_NO_UNPUT
#endif /* FLEX_SCANNER */

%}

%option noyywrap

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
   				   strcpy(csim_lval.id, yytext);
				   csim_lval.id[yyleng - 1] = '\0';
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
				   *csim_lval.id = 0;
				}

<quoted_string>\"		{
				   BEGIN(INITIAL);
				   return STRING;
				}

<quoted_string>[^\"]*		strcpy(csim_lval.id, yytext);

-?{DIGIT}*"."?{DIGIT}*([Ee][-+]?{DIGIT}*)? {
                                   csim_lval.value = g_ascii_strtod (yytext, NULL);
				   return FLOAT;
				}

{WS}				; /* Eat white space */

.				return *yytext;

%%

