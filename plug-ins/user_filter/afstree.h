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
#ifndef AFSTREE_H

typedef struct _afstree {
	int op_type;			/* symbol for operation */
	int value;			/* constant for OP_CONST or number of nodes */
	struct _afstree **nodes;	/* nodes, if neccessary */
} s_afstree;

/* operators */
#define OP_CONST	0	
#define OP_VAR_r	1
#define OP_VAR_g	2
#define OP_VAR_b	3
#define OP_VAR_a	4
#define OP_VAR_c	5
#define OP_VAR_x	6
#define OP_VAR_y	7
#define OP_VAR_z	8
#define OP_VAR_X	9
#define OP_VAR_Y	10
#define OP_VAR_Z	11
#define OP_VAR_i	12
#define OP_VAR_u	13
#define OP_VAR_v	14
#define OP_VAR_d	15
#define OP_VAR_D	16
#define OP_VAR_m	17
#define OP_VAR_M	18
#define OP_VAR_dmin	19
#define OP_VAR_mmin	20
#define OP_ADD		21
#define OP_SUB		22
#define OP_MUL		23
#define OP_DIV		24
#define OP_MOD		25
#define OP_B_AND	26
#define OP_B_OR		27
#define OP_B_XOR	28
#define OP_B_NOT	29
#define OP_COND		30
#define OP_AND		31
#define OP_OR		32
#define OP_NOT		33
#define OP_EQ		34
#define OP_NEQ		35
#define OP_LE		36
#define OP_BE		37
#define OP_LT		38
#define OP_BT		39
#define OP_SHL		41
#define OP_SHR		42
#define F_SRC		43
#define F_RAD		44
#define F_CNV		45
#define F_CTL		46
#define F_ABS		47
#define F_DIF		48
#define F_SCL		49
#define F_VAL		50
#define F_ADD		51
#define F_SUB		52
#define F_MIX		53
#define F_RND		54
#define F_C2D		55
#define F_C2M		56
#define F_R2X		57
#define F_R2Y		58
#define F_MIN		59
#define F_MAX		60
#define F_PUT		61
#define F_GET		62
#define F_SIN		63
#define F_COS		64
#define F_TAN		65
#define F_SQR		66
#define OP_COMMA	67

#endif