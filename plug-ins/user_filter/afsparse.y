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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "afstree.h"
#include "afsparse.h"

#define YYSTYPE s_afstree *

s_afstree *afs_root;

int yyerror(char *s)
{
	return 0;
}

s_afstree *get_afstree(char *s)
{
	do_parse(s);
	return afs_root;
}
%}

%left t_comma

%right '?' ':'

%right t_abs
%right t_add
%right t_cnv
%right t_ctl
%right t_dif
%right t_get
%right t_put
%right t_max
%right t_min
%right t_mix
%right t_rnd
%right t_scl
%right t_sqr
%right t_src
%right t_sub
%right t_val
%right t_c2d
%right t_c2m
%right t_cos
%right t_r2x
%right t_r2y
%right t_rad
%right t_sin
%right t_tan

%left '&' '|' '^'
%right t_and
%right t_or
%right t_eq
%right t_neq
%right t_le
%right t_be
%right t_shl
%right t_shr

%left LE BE NE '<' '>'
%left '+' '-'
%left '*' '/' '%'
%left ','
%token '(' ')'

%left t_sign '!' '~'

%token 'r' 'g' 'b' 'a' 'c' 'x' 'y' 'X' 'Y' 'i' 'u' 'v' 'z' 'Z' 'd' 'D' 'm' 'M' t_mmin t_dmin

%token v_int

%%

/****************************************************************************/

input:  { 
		afs_root=NULL; 
		YYABORT; 
	}
	| exp {
		afs_root=$1;
		YYACCEPT;
	}
	;


exp:    v_int			{ $$ = $1; }
	| 'r' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_r;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'g' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_g;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'b' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_b;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'a' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_a;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'c' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_c;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'x' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_x;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'y' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_y;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'z' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_z;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'X' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_X;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'Y' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_Y;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'Z' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_Z;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'i' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_i;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'u' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_u;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'v' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_v;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'd' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_d;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'D' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_D;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'm' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_m;
		$$->value=0;
		$$->nodes=NULL;
	}
	| 'M' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_M;
		$$->value=0;
		$$->nodes=NULL;
	}
	| t_dmin { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_dmin;
		$$->value=0;
		$$->nodes=NULL;
	}
	| t_mmin { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_VAR_mmin;
		$$->value=0;
		$$->nodes=NULL;
	}
        | exp '+' exp		{ 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_ADD;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
        | exp '-' exp		{ 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_SUB;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
        | exp '*' exp		{ 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_MUL;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
        | exp '/' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_DIV;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	} 
        | exp '%' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_MOD;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	} 
        | exp '^' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_B_XOR;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	} 
        | exp '&' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_B_AND;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	} 
        | exp '|' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_B_OR;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	} 
        | '~' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_B_NOT;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$2;
	} 
        | '-' exp %prec t_sign	{ 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_SUB;
		$$->value=2;
		$$->nodes=malloc(2*sizeof(s_afstree *));
		$$->nodes[0]=malloc(sizeof(s_afstree));
		$$->nodes[0]->op_type=OP_CONST;
		$$->nodes[0]->value=0;
		$$->nodes[0]->nodes=NULL;
		$$->nodes[1]=$2;
	}         
        | exp '?' exp ':' exp   { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_COND;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
		$$->nodes[2]=$5;
	}
	| exp t_and exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_AND;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_or exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_OR;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_eq exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_EQ;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_neq exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_NEQ;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_le exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_LE;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_be exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_BE;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp '<' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_LT;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp '>' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_BT;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| '!' exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_NOT;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$2;
	}
	| exp t_shl exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_SHL;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_shr exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_SHR;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| exp t_comma exp { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=OP_COMMA ;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$1;
		$$->nodes[1]=$3;
	}
	| t_src '(' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_SRC;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
	}
	| t_rad '(' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_RAD;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
	}
	| t_cnv '(' exp ',' exp ',' exp ',' exp ',' exp ',' exp ',' exp ',' exp ',' exp ',' exp  ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_CNV;
		$$->value=10;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
		$$->nodes[3]=$9;
		$$->nodes[4]=$11;
		$$->nodes[5]=$13;
		$$->nodes[6]=$15;
		$$->nodes[7]=$17;
		$$->nodes[8]=$19;
		$$->nodes[9]=$21;
	}
	| t_ctl '(' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_CTL;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}
	| t_abs '(' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_ABS;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}
	| t_dif '(' exp ',' exp  ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_DIF;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_scl '(' exp ',' exp ','exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_SCL;
		$$->value=5;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
		$$->nodes[3]=$9;
		$$->nodes[4]=$11;
	}
	| t_val '(' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_VAL;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
	}
	| t_add '(' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_ADD;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
	}
	| t_sub '(' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_SUB;
		$$->value=3;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
	}
 	| t_mix '(' exp ',' exp ',' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_MIX;
		$$->value=4;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
		$$->nodes[2]=$7;
		$$->nodes[3]=$9;
	}
	| t_rnd '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_RND;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_c2d '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_C2D;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_c2m '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_C2M;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_r2x '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_R2X;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_r2y '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_R2Y;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_min '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_MIN;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_max '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_MAX;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_put '(' exp ',' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_PUT;
		$$->value=2;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
		$$->nodes[1]=$5;
	}
	| t_get '(' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_GET;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}
        | t_sin '(' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_SIN;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}            
        | t_cos '(' exp ')' { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_COS;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}            
        | t_tan '(' exp ')'    { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_TAN;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}            
        | t_sqr '(' exp ')'    { 
		$$ = malloc(sizeof(s_afstree));
		$$->op_type=F_SQR;
		$$->value=1;
		$$->nodes=malloc(($$->value)*sizeof(s_afstree *));
		$$->nodes[0]=$3;
	}
        | '(' exp ')' 		{ $$=$2; }
	;

%%

void free_tree(s_afstree *a)
{
	int i;
	if (a==NULL) return;
	if (a->op_type==OP_CONST) return;
	for (i=0; i<a->value; i++) {
		free_tree(a->nodes[i]);
	}
	free(a->nodes);
	free(a);
	return;
}
