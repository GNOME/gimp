#include "exprtree.h"

typedef void (*stackfunc) (double);

typedef struct _postfix
{
    stackfunc func;
    double arg;
} postfix;

void make_postfix (exprtree *tree);
double eval_postfix (void);
