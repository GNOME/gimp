%option noyywrap

%{
#include "gcg.h"	
#include "parser.h"
%}

ident		[A-Za-z][A-Za-z0-9-]*
header		<[[:alnum:]/.]+>
ws		[ \n\t\r] 
comment		\/\/[^\n]*\n
string		\"(([^\"]*)|\\\"|\\\\)*\"

%%

class return T_CLASS;
public return T_PUBLIC;
static return T_STATIC;
protected return T_PROTECTED;
pre-emit return T_PRE_EMIT;
post-emit return T_POST_EMIT;
read-only return T_READ_ONLY;
read-write return T_READ_WRITE;
private return T_PRIVATE;
const return T_CONST;
abstract return T_ABSTRACT;
direct return T_DIRECT;
type return T_TYPE;
attribute return T_ATTRIBUTE;
module return T_MODULE;
import return T_IMPORT;
header return T_HEADER;
opaque return T_OPAQUE;

void return T_VOID;
enum return T_ENUM;
lags return T_FLAGS;
@int return T_INT;
real return T_DOUBLE;
boxed return T_BOXED;


{ident} {
	yylval.id=g_quark_to_string(g_quark_from_string(yytext));
	return T_IDENT;
}

{comment} {
}

{ws} {
}

{header} {
	yylval.id=g_quark_to_string(g_quark_from_string(yytext));
	return T_HEADERNAME;
}

{string} {
	yylval.str=g_string_new(yytext);
	return T_STRING;
}

\. return T_SCOPE;
\* return T_POINTER;
\& return T_NOTNULLPTR;
\; return T_END;
\{ return T_OPEN_B;
\} return T_CLOSE_B;
\( return T_OPEN_P;
\) return T_CLOSE_P;
\, return T_COMMA;
\: return T_INHERITANCE;
. return yytext[0];

%%

