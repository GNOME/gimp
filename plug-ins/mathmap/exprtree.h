#ifndef __EXPRTREE_H__
#define __EXPRTREE_H__

#ifdef USE_TREE
typedef double (*function) (double*);
#else
typedef void (*function) (double);
#endif

typedef char ident[64];

typedef struct _exprtree
{
    int type;

    union
    {
	double number;
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
    } val;

    struct _exprtree *next;
} exprtree;

#define EXPR_NUMBER     1
#define EXPR_ADD        2
#define EXPR_SUB        3
#define EXPR_MUL        4
#define EXPR_DIV        5
#define EXPR_MOD        6
#define EXPR_NEG        7
#define EXPR_FUNC       8
#define EXPR_VAR_X      9
#define EXPR_VAR_Y     10
#define EXPR_VAR_R     11
#define EXPR_VAR_A     12
#define EXPR_VAR_W     13
#define EXPR_VAR_H     14
#define EXPR_VAR_BIG_R 15
#define EXPR_VAR_BIG_X 16
#define EXPR_VAR_BIG_Y 17

exprtree* make_number (double num);
exprtree* make_var (char *name);
exprtree* make_operator (int type, exprtree *left, exprtree *right);
exprtree* make_function (char *name, exprtree *args);

exprtree* arglist_append (exprtree *list1, exprtree *list2);

double eval_exprtree (exprtree *tree);

#endif
