#include <assert.h>

#include "vars.h"

variable *firstVariable = 0;

variable*
register_variable (char *name)
{
    variable *var;

    assert(strlen(name) < VAR_MAX_LENGTH);

    for (var = firstVariable; var != 0; var = var->next)
	if (strcmp(name, var->name) == 0)
	    return var;

    var = (variable*)malloc(sizeof(variable));
    strcpy(var->name, name);
    var->value = 0.0;
    var->next = firstVariable;
    firstVariable = var;

    return var;
}

void
clear_all_variables (void)
{
    variable *var = firstVariable;

    while (var != 0)
    {
	variable *next = var->next;

	free(var);
	var = next;
    }

    firstVariable = 0;
}
