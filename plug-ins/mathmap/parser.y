%{
#include <stdio.h>

#include "exprtree.h"

extern exprtree *theExprtree;
%}

%union {
    ident ident;
    exprtree *exprtree;
}

%token T_IDENT T_NUMBER
%token T_IF T_THEN T_ELSE T_END
%token T_WHILE T_DO

%right ';'
%right '='
%left '+' '-'
%left '*' '/' '%'
%left NEG

%%

start :   expr              { theExprtree = $<exprtree>1; }
        ;

expr :   T_NUMBER            { $<exprtree>$ = $<exprtree>1; }
       | T_IDENT             { $<exprtree>$ = make_var($<ident>1); }
       | expr '+' expr       { $<exprtree>$ = make_operator(EXPR_ADD,
                                                            $<exprtree>1, $<exprtree>3); }
       | expr '-' expr       { $<exprtree>$ = make_operator(EXPR_SUB,
                                                            $<exprtree>1, $<exprtree>3); }
       | expr '*' expr       { $<exprtree>$ = make_operator(EXPR_MUL,
                                                            $<exprtree>1, $<exprtree>3); }
       | expr '/' expr       { $<exprtree>$ = make_operator(EXPR_DIV,
                                                            $<exprtree>1, $<exprtree>3); }
       | expr '%' expr       { $<exprtree>$ = make_operator(EXPR_MOD,
                                                            $<exprtree>1, $<exprtree>3); }
       | '-' expr %prec NEG  { $<exprtree>$ = make_operator(EXPR_NEG, $<exprtree>2, 0); }
       | '(' expr ')'        { $<exprtree>$ = $<exprtree>2; };
       | T_IDENT '(' arglist ')'
                             { $<exprtree>$ = make_function($<ident>1, $<exprtree>3); }
       | T_IDENT '=' expr    { $<exprtree>$ = make_assignment($<ident>1, $<exprtree>3); }
       | expr ';' expr       { $<exprtree>$ = make_sequence($<exprtree>1, $<exprtree>3); }
       | T_IF expr T_THEN expr T_END
                             { $<exprtree>$ = make_if_then($<exprtree>2, $<exprtree>4); }
       | T_IF expr T_THEN expr T_ELSE expr T_END
                             { $<exprtree>$ = make_if_then_else($<exprtree>2,
								$<exprtree>4,
								$<exprtree>6); }
       | T_WHILE expr T_DO expr T_END
                             { $<exprtree>$ = make_while($<exprtree>2, $<exprtree>4); }
       | T_DO expr T_WHILE expr T_END
                             { $<exprtree>$ = make_do_while($<exprtree>2, $<exprtree>4); }
       ;

arglist :                    { $<exprtree>$ = 0; }
          | args             { $<exprtree>$ = $<exprtree>1; }
          ;

args :   expr                { $<exprtree>$ = $<exprtree>1; }
       | args ',' expr       { $<exprtree>$ = arglist_append($<exprtree>1, $<exprtree>3); }
       ;

%%

int
yyerror (char *s)
{
    fprintf(stderr, "%s\n", s);

    return 0;
}
