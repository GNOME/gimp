#ifndef __VARS_H__
#define __VARS_H__

#define VAR_MAX_LENGTH     32

typedef struct _variable
{
    char name[VAR_MAX_LENGTH];
    double value;

    struct _variable *next;
} variable;

variable* register_variable (char *name);

void clear_all_variables (void);

#endif
