#include <stdio.h>
#include <stdlib.h>

#include "exprtree.h"
#include "builtins.h"

extern double currentX,
    currentY,
    currentR,
    currentA;
extern int imageWidth,
    imageHeight;
extern int usesRA;
extern int intersamplingEnabled;

exprtree*
make_number (double num)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_NUMBER;
    tree->val.number = num;

    tree->next = 0;

    return tree;
}

exprtree*
make_var (char *name)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    if (strcmp(name, "x") == 0)
	tree->type = EXPR_VAR_X;
    else if (strcmp(name, "y") == 0)
	tree->type = EXPR_VAR_Y;
    else if (strcmp(name, "r") == 0)
    {
	tree->type = EXPR_VAR_R;
	usesRA = 1;
    }
    else if (strcmp(name, "a") == 0)
    {
	tree->type = EXPR_VAR_A;
	usesRA = 1;
    }
    else if (strcmp(name, "W") == 0)
	tree->type = EXPR_VAR_W;
    else if (strcmp(name, "H") == 0)
	tree->type = EXPR_VAR_H;
    else if (strcmp(name, "R") == 0)
	tree->type = EXPR_VAR_BIG_R;
    else if (strcmp(name, "X") == 0)
	tree->type = EXPR_VAR_BIG_X;
    else if (strcmp(name, "Y") == 0)
	tree->type = EXPR_VAR_BIG_Y;
    else
    {
	tree->type = EXPR_VARIABLE;
	tree->val.var = register_variable(name);
    }

    tree->next = 0;

    return tree;
}

exprtree*
make_operator (int type, exprtree *left, exprtree *right)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = type;
    tree->val.operator.left = left;
    tree->val.operator.right = right;

    tree->next = 0;

    return tree;
}

exprtree*
make_function (char *name, exprtree *args)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    if (strcmp(name, "sin") == 0)
    {
	tree->val.func.routine = builtin_sin;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "cos") == 0)
    {
	tree->val.func.routine = builtin_cos;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "tan") == 0)
    {
	tree->val.func.routine = builtin_tan;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "asin") == 0)
    {
	tree->val.func.routine = builtin_asin;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "acos") == 0)
    {
	tree->val.func.routine = builtin_acos;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "atan") == 0)
    {
	tree->val.func.routine = builtin_atan;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "abs") == 0)
    {
	tree->val.func.routine = builtin_abs;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "sign") == 0)
    {
	tree->val.func.routine = builtin_sign;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "min") == 0)
    {
	tree->val.func.routine = builtin_min;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "max") == 0)
    {
	tree->val.func.routine = builtin_max;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "or") == 0)
    {
	tree->val.func.routine = builtin_or;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "and") == 0)
    {
	tree->val.func.routine = builtin_and;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "less") == 0)
    {
	tree->val.func.routine = builtin_less;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "inintv") == 0)
    {
	tree->val.func.routine = builtin_inintv;
	tree->val.func.numArgs = 3;
    }
    else if (strcmp(name, "rand") == 0)
    {
	tree->val.func.routine = builtin_rand;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "origValXY") == 0)
    {
	if (intersamplingEnabled)
	    tree->val.func.routine = builtin_origValXYIntersample;
	else
	    tree->val.func.routine = builtin_origValXY;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "origValRA") == 0)
    {
	if (intersamplingEnabled)
	    tree->val.func.routine = builtin_origValRAIntersample;
	else
	    tree->val.func.routine = builtin_origValRA;
	tree->val.func.numArgs = 2;
    }
    else if (strcmp(name, "red") == 0)
    {
	tree->val.func.routine = builtin_red;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "green") == 0)
    {
	tree->val.func.routine = builtin_green;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "blue") == 0)
    {
	tree->val.func.routine = builtin_blue;
	tree->val.func.numArgs = 1;
    }
    else if (strcmp(name, "rgbColor") == 0)
    {
	tree->val.func.routine = builtin_rgbColor;
	tree->val.func.numArgs = 3;
    }
    else if (strcmp(name, "grayColor") == 0)
    {
	tree->val.func.routine = builtin_grayColor;
	tree->val.func.numArgs = 1;
    }
    else
    {
	tree->type = EXPR_NUMBER;
	tree->val.number = 0.0;

	tree->next = 0;

	return tree;
    }
    
    tree->type = EXPR_FUNC;
    tree->val.func.args = args;

    tree->next = 0;

    return tree;
}

exprtree*
make_sequence (exprtree *left, exprtree *right)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_SEQUENCE;
    tree->val.operator.left = left;
    tree->val.operator.right = right;

    tree->next = 0;

    return tree;
}

exprtree*
make_assignment (char *name, exprtree *value)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));
    variable *var = register_variable(name);

    tree->type = EXPR_ASSIGNMENT;
    tree->val.assignment.var = var;
    tree->val.assignment.value = value;

    tree->next = 0;

    return tree;
}

exprtree*
make_if_then (exprtree *condition, exprtree *consequence)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_IF_THEN;
    tree->val.ifExpr.condition = condition;
    tree->val.ifExpr.consequence = consequence;

    tree->next = 0;

    return tree;
}

exprtree*
make_if_then_else (exprtree *condition, exprtree *consequence, exprtree *alternative)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_IF_THEN_ELSE;
    tree->val.ifExpr.condition = condition;
    tree->val.ifExpr.consequence = consequence;
    tree->val.ifExpr.alternative = alternative;

    tree->next = 0;

    return tree;
}

exprtree*
make_while (exprtree *invariant, exprtree *body)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_WHILE;
    tree->val.whileExpr.invariant = invariant;
    tree->val.whileExpr.body = body;

    tree->next = 0;

    return tree;
}

exprtree*
make_do_while (exprtree *body, exprtree *invariant)
{
    exprtree *tree = (exprtree*)malloc(sizeof(exprtree));

    tree->type = EXPR_DO_WHILE;
    tree->val.whileExpr.invariant = invariant;
    tree->val.whileExpr.body = body;

    tree->next = 0;

    return tree;
}

exprtree*
arglist_append (exprtree *list1, exprtree *list2)
{
    exprtree *tree = list1;

    while (tree->next != 0)
	tree = tree->next;

    tree->next = list2;

    return list1;
}
