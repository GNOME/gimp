#include <stdio.h>
#include <math.h>

#include "postfix.h"

extern double currentX,
    currentY,
    currentR,
    currentA,
    imageR,
    imageX,
    imageY;
extern int imageWidth,
    imageHeight;

#define STACKSIZE     128
#define EXPRSIZE      256

double stack[STACKSIZE];
int stackp;

postfix expression[EXPRSIZE];
int exprp,
    exprlen;

void
stack_push (double arg)
{
    stack[stackp++] = arg;
}

void
stack_add (double arg)
{
    stack[stackp - 2] = stack[stackp - 2] + stack[stackp - 1];
    --stackp;
}

void
stack_add_i (double arg)
{
    stack[stackp - 1] += arg;
}

void
stack_sub (double arg)
{
    stack[stackp - 2] = stack[stackp - 2] - stack[stackp - 1];
    --stackp;
}

void
stack_sub_i (double arg)
{
    stack[stackp - 1] -= arg;
}

void
stack_neg (double arg)
{
    stack[stackp - 1] = -stack[stackp - 1];
}

void
stack_mul (double arg)
{
    stack[stackp - 2] = stack[stackp - 2] * stack[stackp - 1];
    --stackp;
}

void
stack_mul_i (double arg)
{
    stack[stackp - 1] *= arg;
}

void
stack_div (double arg)
{
    stack[stackp - 2] = stack[stackp - 2] / stack[stackp - 1];
    --stackp;
}

void
stack_div_i (double arg)
{
    stack[stackp - 1] /= arg;
}

void
stack_mod (double arg)
{
    stack[stackp - 2] = fmod(stack[stackp - 2], stack[stackp - 1]);
    --stackp;
}

void
stack_mod_i (double arg)
{
    stack[stackp - 1] = fmod(stack[stackp - 1], arg);
}

void
stack_var_x (double arg)
{
    stack[stackp++] = currentX;
}

void
stack_var_y (double arg)
{
    stack[stackp++] = currentY;
}

void
stack_var_r (double arg)
{
    stack[stackp++] = currentR;
}

void
stack_var_a (double arg)
{
    stack[stackp++] = currentA;
}

void
stack_var_w (double arg)
{
    stack[stackp++] = imageWidth;
}

void
stack_var_h (double arg)
{
    stack[stackp++] = imageHeight;
}

void
stack_var_big_r (double arg)
{
    stack[stackp++] = imageR;
}

void
stack_var_big_x (double arg)
{
    stack[stackp++] = imageX;
}

void
stack_var_big_y (double arg)
{
    stack[stackp++] = imageY;
}

void
make_postfix_recursive (exprtree *tree)
{
    switch (tree->type)
    {
	case EXPR_NUMBER :
	    expression[exprp].func = stack_push;
	    expression[exprp].arg = tree->val.number;
	    ++exprp;
	    break;

	case EXPR_ADD :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_add_i;
		expression[exprp].arg = tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_add;
	    }
	    ++exprp;
	    break;

	case EXPR_SUB :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_sub_i;
		expression[exprp].arg = tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_sub;
	    }
	    ++exprp;
	    break;

	case EXPR_NEG :
	    if (tree->val.operator.left->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_push;
		expression[exprp].arg = -tree->val.operator.left->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.left);
		expression[exprp].func = stack_neg;
	    }
	    ++exprp;
	    break;

	case EXPR_MUL :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_mul_i;
		expression[exprp].arg = tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_mul;
	    }
	    ++exprp;
	    break;

	case EXPR_DIV :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_div_i;
		expression[exprp].arg = tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_div;
	    }
	    ++exprp;
	    break;

	case EXPR_MOD :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = stack_mod_i;
		expression[exprp].arg = tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_mod;
	    }
	    ++exprp;
	    break;

	case EXPR_VAR_X :
	    expression[exprp++].func = stack_var_x;
	    break;

	case EXPR_VAR_Y :
	    expression[exprp++].func = stack_var_y;
	    break;
	 
	case EXPR_VAR_R :
	    expression[exprp++].func = stack_var_r;
	    break;

	case EXPR_VAR_A :
	    expression[exprp++].func = stack_var_a;
	    break;
	 
	case EXPR_VAR_W :
	    expression[exprp++].func = stack_var_w;
	    break;

	case EXPR_VAR_H :
	    expression[exprp++].func = stack_var_h;
	    break;

	case EXPR_VAR_BIG_R :
	    expression[exprp++].func = stack_var_big_r;
	    break;

	case EXPR_VAR_BIG_X :
	    expression[exprp++].func = stack_var_big_x;
	    break;

	case EXPR_VAR_BIG_Y :
	    expression[exprp++].func = stack_var_big_y;
	    break;

	case EXPR_FUNC :
	    {
		exprtree *arg = tree->val.func.args;

		while (arg != 0)
		{
		    make_postfix_recursive(arg);
		    arg = arg->next;
		}

		expression[exprp++].func = tree->val.func.routine;
		break;
	    }

	default :
	    fprintf(stderr, "illegal expr\n");
    }
}

void
make_postfix (exprtree *tree)
{
    exprp = 0;
    make_postfix_recursive(tree);
    exprlen = exprp;
}

double
eval_postfix (void)
{
    stackp = 0;
    for (exprp = 0; exprp < exprlen; ++exprp)
	expression[exprp].func(expression[exprp].arg);
    return stack[0];
}
