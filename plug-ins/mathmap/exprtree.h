#ifndef __EXPRTREE_H__
#define __EXPRTREE_H__

#include "vars.h"
#include "builtins.h"

#ifdef USE_TREE
typedef double (*function) (double*);
#else
typedef void (*function) (void*);
#endif

#define MAX_IDENT_LENGTH    63

typedef char ident[MAX_IDENT_LENGTH + 1];

typedef struct _exprtree
{
    int type;

    union
    {
	double number;
	variable *var;
	struct
	{
	    int numArgs;
	    function routine;
	    struct _exprtree *args;
	} func;
	struct
	{
	    struct _exprtree *left;
	    struct _exprtree *right;
	} operator;
	struct
	{
	    variable *var;
	    struct _exprtree *value;
	} assignment;
	struct
	{
	    struct _exprtree *condition;
	    struct _exprtree *consequence;
	    struct _exprtree *alternative;
	    int label1;
	    int label2;
	} ifExpr;
	struct
	{
	    struct _exprtree *invariant;
	    struct _exprtree *body;
	    int label1;
	    int label2;
	} whileExpr;
    } val;

    struct _exprtree *next;
} exprtree;

#define EXPR_NUMBER        1
#define EXPR_ADD           2
#define EXPR_SUB           3
#define EXPR_MUL           4
#define EXPR_DIV           5
#define EXPR_MOD           6
#define EXPR_NEG           7
#define EXPR_FUNC          8
#define EXPR_VAR_X         9
#define EXPR_VAR_Y        10
#define EXPR_VAR_T        11
#define EXPR_VAR_R        12
#define EXPR_VAR_A        13
#define EXPR_VAR_W        14
#define EXPR_VAR_H        15
#define EXPR_VAR_BIG_R    16
#define EXPR_VAR_BIG_X    17
#define EXPR_VAR_BIG_Y    18
#define EXPR_SEQUENCE     19
#define EXPR_ASSIGNMENT   20
#define EXPR_VARIABLE     21
#define EXPR_IF_THEN      22
#define EXPR_IF_THEN_ELSE 23
#define EXPR_WHILE        24
#define EXPR_DO_WHILE     25

exprtree* make_number (double num);
exprtree* make_var (char *name);
exprtree* make_operator (int type, exprtree *left, exprtree *right);
exprtree* make_function (builtin *theBuiltin, exprtree *args);
exprtree* make_sequence (exprtree *left, exprtree *right);
exprtree* make_assignment (char *name, exprtree *value);
exprtree* make_if_then (exprtree *condition, exprtree *conclusion);
exprtree* make_if_then_else (exprtree *condition, exprtree *conclusion, exprtree *alternative);
exprtree* make_while (exprtree *invariant, exprtree *body);
exprtree* make_do_while (exprtree *body, exprtree *invariant);

exprtree* arglist_append (exprtree *list1, exprtree *list2);

double eval_exprtree (exprtree *tree);

#endif
