#include "exprtree.h"

typedef void (*stackfunc) (void*);

typedef struct _postfix
{
    stackfunc func;
    void *arg;
} postfix;

void make_postfix (exprtree *tree);
void output_postfix (void);
double eval_postfix (void);
