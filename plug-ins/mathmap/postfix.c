#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "builtins.h"
#include "vars.h"

#include "postfix.h"

extern double currentX,
    currentY,
    currentT,
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
stack_push (double *arg)
{
    stack[stackp++] = *arg;
}

void
stack_pop (void *arg)
{
    --stackp;
}

void
stack_jmp (int *arg)
{
    exprp = *arg - 1;
}

void
stack_jez (int *arg)
{
    if (stack[--stackp] == 0.0)
	exprp = *arg - 1;
}

void
stack_jnez (int *arg)
{
    if (stack[--stackp] != 0.0)
	exprp = *arg - 1;
}

void
stack_add (void *arg)
{
    stack[stackp - 2] = stack[stackp - 2] + stack[stackp - 1];
    --stackp;
}

void
stack_add_i (double *arg)
{
    stack[stackp - 1] += *arg;
}

void
stack_sub (void *arg)
{
    stack[stackp - 2] = stack[stackp - 2] - stack[stackp - 1];
    --stackp;
}

void
stack_sub_i (double *arg)
{
    stack[stackp - 1] -= *arg;
}

void
stack_neg (void *arg)
{
    stack[stackp - 1] = -stack[stackp - 1];
}

void
stack_mul (void *arg)
{
    stack[stackp - 2] = stack[stackp - 2] * stack[stackp - 1];
    --stackp;
}

void
stack_mul_i (double *arg)
{
    stack[stackp - 1] *= *arg;
}

void
stack_div (void *arg)
{
    stack[stackp - 2] = stack[stackp - 2] / stack[stackp - 1];
    --stackp;
}

void
stack_div_i (double *arg)
{
    stack[stackp - 1] /= *arg;
}

void
stack_mod (void *arg)
{
    stack[stackp - 2] = fmod(stack[stackp - 2], stack[stackp - 1]);
    --stackp;
}

void
stack_mod_i (double *arg)
{
    stack[stackp - 1] = fmod(stack[stackp - 1], *arg);
}

void
stack_var_x (void *arg)
{
    stack[stackp++] = currentX;
}

void
stack_var_y (void *arg)
{
    stack[stackp++] = currentY;
}

void
stack_var_t (void *arg)
{
    stack[stackp++] = currentT;
}

void
stack_var_r (void *arg)
{
    stack[stackp++] = currentR;
}

void
stack_var_a (void *arg)
{
    stack[stackp++] = currentA;
}

void
stack_var_w (void *arg)
{
    stack[stackp++] = imageWidth;
}

void
stack_var_h (void *arg)
{
    stack[stackp++] = imageHeight;
}

void
stack_var_big_r (void *arg)
{
    stack[stackp++] = imageR;
}

void
stack_var_big_x (void *arg)
{
    stack[stackp++] = imageX;
}

void
stack_var_big_y (void *arg)
{
    stack[stackp++] = imageY;
}

void
stack_variable (variable *var)
{
    stack[stackp++] = var->value;
}

void
stack_assign (variable *var)
{
    var->value = stack[stackp - 1];
}

void
make_postfix_recursive (exprtree *tree)
{
    static double theZeroValue = 0.0;

    switch (tree->type)
    {
	case EXPR_NUMBER :
	    expression[exprp].func = (stackfunc)stack_push;
	    expression[exprp].arg = &tree->val.number;
	    ++exprp;
	    break;

	case EXPR_ADD :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = (stackfunc)stack_add_i;
		expression[exprp].arg = &tree->val.operator.right->val.number;
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
		expression[exprp].func = (stackfunc)stack_sub_i;
		expression[exprp].arg = &tree->val.operator.right->val.number;
	    }
	    else
	    {
		make_postfix_recursive(tree->val.operator.right);
		expression[exprp].func = stack_sub;
	    }
	    ++exprp;
	    break;

	case EXPR_NEG :
	    /*
	    if (tree->val.operator.left->type == EXPR_NUMBER)
	    {
		expression[exprp].func = (stackfunc)stack_push;
		expression[exprp].arg = -tree->val.operator.left->val.number;
	    }
	    else
	    { */
		make_postfix_recursive(tree->val.operator.left);
		expression[exprp].func = stack_neg;
		/*	    } */
	    ++exprp;
	    break;

	case EXPR_MUL :
	    make_postfix_recursive(tree->val.operator.left);
	    if (tree->val.operator.right->type == EXPR_NUMBER)
	    {
		expression[exprp].func = (stackfunc)stack_mul_i;
		expression[exprp].arg = &tree->val.operator.right->val.number;
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
		expression[exprp].func = (stackfunc)stack_div_i;
		expression[exprp].arg = &tree->val.operator.right->val.number;
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
		expression[exprp].func = (stackfunc)stack_mod_i;
		expression[exprp].arg = &tree->val.operator.right->val.number;
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
	 
	case EXPR_VAR_T :
	    expression[exprp++].func = stack_var_t;
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

	case EXPR_VARIABLE :
	    expression[exprp].func = (stackfunc)stack_variable;
	    expression[exprp].arg = tree->val.var;
	    ++exprp;
	    break;

	case EXPR_ASSIGNMENT :
	    make_postfix_recursive(tree->val.assignment.value);
	    expression[exprp].func = (stackfunc)stack_assign;
	    expression[exprp].arg = tree->val.assignment.var;
	    ++exprp;
	    break;

	case EXPR_SEQUENCE :
	    make_postfix_recursive(tree->val.operator.left);
	    expression[exprp++].func = stack_pop;
	    make_postfix_recursive(tree->val.operator.right);
	    break;

	case EXPR_IF_THEN :
	    make_postfix_recursive(tree->val.ifExpr.condition);
	    expression[exprp].func = (stackfunc)stack_jez;
	    expression[exprp].arg = &tree->val.ifExpr.label1;
	    ++exprp;
	    make_postfix_recursive(tree->val.ifExpr.consequence);
	    expression[exprp].func = (stackfunc)stack_jmp;
	    expression[exprp].arg = &tree->val.ifExpr.label2;
	    ++exprp;
	    tree->val.ifExpr.label1 = exprp;
	    expression[exprp].func = (stackfunc)stack_push;
	    expression[exprp].arg = &theZeroValue;
	    ++exprp;
	    tree->val.ifExpr.label2 = exprp;
	    break;

	case EXPR_IF_THEN_ELSE :
	    make_postfix_recursive(tree->val.ifExpr.condition);
	    expression[exprp].func = (stackfunc)stack_jez;
	    expression[exprp].arg = &tree->val.ifExpr.label1;
	    ++exprp;
	    make_postfix_recursive(tree->val.ifExpr.consequence);
	    expression[exprp].func = (stackfunc)stack_jmp;
	    expression[exprp].arg = &tree->val.ifExpr.label2;
	    ++exprp;
	    tree->val.ifExpr.label1 = exprp;
	    make_postfix_recursive(tree->val.ifExpr.alternative);
	    tree->val.ifExpr.label2 = exprp;
	    break;

	case EXPR_WHILE :
	    tree->val.whileExpr.label1 = exprp;
	    make_postfix_recursive(tree->val.whileExpr.invariant);
	    expression[exprp].func = (stackfunc)stack_jez;
	    expression[exprp].arg = &tree->val.whileExpr.label2;
	    ++exprp;
	    make_postfix_recursive(tree->val.whileExpr.body);
	    expression[exprp++].func = stack_pop;
	    expression[exprp].func = (stackfunc)stack_jmp;
	    expression[exprp].arg = &tree->val.whileExpr.label1;
	    ++exprp;
	    tree->val.whileExpr.label2 = exprp;
	    expression[exprp].func = (stackfunc)stack_push;
	    expression[exprp].arg = &theZeroValue;
	    ++exprp;
	    break;

	case EXPR_DO_WHILE :
	    tree->val.whileExpr.label1 = exprp;
	    make_postfix_recursive(tree->val.whileExpr.body);
	    expression[exprp++].func = stack_pop;
	    make_postfix_recursive(tree->val.whileExpr.invariant);
	    expression[exprp].func = (stackfunc)stack_jnez;
	    expression[exprp].arg = &tree->val.whileExpr.label1;
	    ++exprp;
	    expression[exprp].func = (stackfunc)stack_push;
	    expression[exprp].arg = &theZeroValue;
	    ++exprp;
	    break;

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

void
output_postfix (void)
{
    int i;

    printf("-------------------------\n");

    for (i = 0; i < exprlen; ++i)
    {
	printf("%d   ", i);
	if (expression[i].func == (stackfunc)stack_push)
	    printf("push %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_pop)
	    printf("pop\n");
	else if (expression[i].func == (stackfunc)stack_jmp)
	    printf("jmp %d\n", *(int*)expression[i].arg);
	else if (expression[i].func == (stackfunc)stack_jez)
	    printf("jez %d\n", *(int*)expression[i].arg);
	else if (expression[i].func == (stackfunc)stack_jnez)
	    printf("jnez %d\n", *(int*)expression[i].arg);
	else if (expression[i].func == stack_add)
	    printf("add\n");
	else if (expression[i].func == (stackfunc)stack_add_i)
	    printf("addi %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_sub)
	    printf("sub\n");
	else if (expression[i].func == (stackfunc)stack_sub_i)
	    printf("subi %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_neg)
	    printf("neg\n");
	else if (expression[i].func == stack_mul)
	    printf("mul\n");
	else if (expression[i].func == (stackfunc)stack_mul_i)
	    printf("muli %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_div)
	    printf("div\n");
	else if (expression[i].func == (stackfunc)stack_div_i)
	    printf("divi %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_mod)
	    printf("mod\n");
	else if (expression[i].func == (stackfunc)stack_mod_i)
	    printf("modi %f\n", *(double*)expression[i].arg);
	else if (expression[i].func == stack_var_x)
	    printf("push x\n");
	else if (expression[i].func == stack_var_y)
	    printf("push y\n");
	else if (expression[i].func == stack_var_t)
	    printf("push t\n");
	else if (expression[i].func == stack_var_r)
	    printf("push r\n");
	else if (expression[i].func == stack_var_a)
	    printf("push a\n");
	else if (expression[i].func == stack_var_w)
	    printf("push w\n");
	else if (expression[i].func == stack_var_h)
	    printf("push h\n");
	else if (expression[i].func == stack_var_big_r)
	    printf("push R\n");
	else if (expression[i].func == stack_var_big_x)
	    printf("push X\n");
	else if (expression[i].func == stack_var_big_y)
	    printf("push Y\n");
	else if (expression[i].func == (stackfunc)stack_variable)
	    printf("push %s\n", ((variable*)expression[i].arg)->name);
	else if (expression[i].func == (stackfunc)stack_assign)
	    printf("sto %s\n", ((variable*)expression[i].arg)->name);
	/*
	else if (expression[i].func == builtin_sin)
	    printf("sin\n");
	else if (expression[i].func == builtin_cos)
	    printf("cos\n");
	else if (expression[i].func == builtin_tan)
	    printf("tan\n");
	else if (expression[i].func == builtin_asin)
	    printf("asin\n");
	else if (expression[i].func == builtin_acos)
	    printf("acos\n");
	else if (expression[i].func == builtin_atan)
	    printf("atan\n");
	else if (expression[i].func == builtin_sign)
	    printf("sign\n");
	else if (expression[i].func == builtin_min)
	    printf("min\n");
	else if (expression[i].func == builtin_max)
	    printf("max\n");
	else if (expression[i].func == builtin_inintv)
	    printf("inintv\n");
	else if (expression[i].func == builtin_rand)
	    printf("rand\n");
	else if (expression[i].func == builtin_origValXY)
	    printf("origValXY\n");
	else if (expression[i].func == builtin_origValXYIntersample)
	    printf("origValXY\n");
	else if (expression[i].func == builtin_origValRA)
	    printf("origValRA\n");
	else if (expression[i].func == builtin_origValRAIntersample)
	    printf("origValRA\n");
	else if (expression[i].func == builtin_grayColor)
	    printf("grayColor\n");
	    */
	else
	    printf("unknown opcode\n");
    }
}

double
eval_postfix (void)
{
    stackp = 0;
    for (exprp = 0; exprp < exprlen; ++exprp)
	expression[exprp].func(expression[exprp].arg);
    return stack[0];
}
