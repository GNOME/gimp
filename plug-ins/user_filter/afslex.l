%{
/*
 * Written 1997 Jens Ch. Restemeier <jchrr@hrz.uni-bielefeld.de>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <string.h>
#include <ctype.h>

#include "afstree.h"
#include "afsparse.h"
#include "afsparse.tab.h"

int depth;
%}

%%

"&&"	return t_and;
"||"	return t_or;
"=="	return t_eq;
"!="	return t_neq;
"<="	return t_le;
">="	return t_be;
"<<"	return t_shl;
">>"	return t_shr;

"dmin"	return t_dmin;
"mmin"	return t_mmin;

"abs"	return t_abs;
"add"	return t_add;
"cnv"	return t_cnv;
"ctl"	return t_ctl;
"dif"	return t_dif;
"get"	return t_get;
"put"	return t_put;
"max"	return t_max;
"min"	return t_min;
"mix"	return t_mix;
"rnd"	return t_rnd;
"scl"	return t_scl;
"sqr"	return t_sqr;
"src"	return t_src;
"sub"	return t_sub;
"val"	return t_val;
"c2d"	return t_c2d;
"c2m"	return t_c2m;
"cos"	return t_cos;
"r2x"	return t_r2x;
"r2y"	return t_r2y;
"rad"	return t_rad;
"sin"	return t_sin;
"tan"	return t_tan;

[\r\n\t" "]	/* ignore NL/CR/TAB/SPACE */

[0-9]+  {
		yylval=malloc(sizeof(s_afstree));
		yylval->op_type=OP_CONST;
		yylval->value=atoi(yytext); 
		yylval->nodes=NULL;
		return v_int; 
	}

"("	{ depth++; return yytext[0]; }
")"	{ if (depth>0) depth--; return yytext[0]; }
","	{ if (depth>0) return yytext[0]; else return t_comma; }
.	return yytext[0];

%%

void do_parse(char *s)  
{
        YY_BUFFER_STATE yy_buffer_state;
	depth=0;
        yy_buffer_state=yy_scan_string(s);
        yyparse();
        yy_delete_buffer(yy_buffer_state);
}
