/* -*- c -*- */

#ifndef __BUILTINS_H__
#define __BUILTINS_H__

#define MAX_BUILTIN_LENGTH     63

typedef void (*builtinFunction) (void*);

typedef struct _builtin
{
    char name[MAX_BUILTIN_LENGTH + 1];
    builtinFunction function;
    int numParams;
    struct _builtin *next;
} builtin;

double color_to_double (unsigned int red, unsigned int green,
			unsigned int blue, unsigned int alpha);
void double_to_color (double val, unsigned int *red, unsigned int *green,
		      unsigned int *blue, unsigned int *alpha);

builtin* builtin_with_name (const char *name);

void init_builtins (void);

#endif
