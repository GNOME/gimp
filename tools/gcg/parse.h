#ifndef __PARSE_H__
#define __PARSE_H__

#include <stdio.h>
#include "gcg.h"


gint yylex(void);
gint yyerror(gchar* s);
gint yyparse(void);
extern int yydebug;
extern int yy_flex_debug;
extern FILE* yyin;

extern Module* current_module;

#endif
