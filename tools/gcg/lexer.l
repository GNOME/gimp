%option noyywrap
%option yylineno

%{
#include "gcg.h"
#include "parser.h"
#include "parse.h"
%}


ident		[A-Za-z][A-Za-z0-9-]*
header		<[[:alnum:]/.]+>
ws		[ \n\t\r] 
comment		\/\/[^\n]*\n
string		\"(([^\"]*)|\\\"|\\\\)*\"
preproc         ^#.*\n



%%

public return T_PUBLIC;
static return T_STATIC;
protected return T_PROTECTED;
pre-emit return T_PRE_EMIT;
post-emit return T_POST_EMIT;
mid-emit return T_DUAL_EMIT;
read-only return T_READ_ONLY;
read-write return T_READ_WRITE;
private return T_PRIVATE;
const return T_CONST;
virtual return T_ABSTRACT;
direct return T_DIRECT;
type return T_TYPE;
attribute return T_ATTRIBUTE;
module return T_MODULE;
import return T_IMPORT;
header return T_HEADER;
opaque return T_OPAQUE;
package return T_PACKAGE;
void return T_VOID;
root return T_ROOT;

class return T_CLASS;
enum return T_ENUM;
flags return T_FLAGS;
integer return T_INT;
double return T_DOUBLE;
boxed return T_BOXED;
foreign return T_FOREIGN;
character return T_CHAR;

{ident} {
	yylval.id=g_quark_to_string(g_quark_from_string(yytext));
	return T_IDENT;
}

{comment} {
}

{ws} {
}

{preproc} {
}

{header} {
	gchar* tmp = g_strndup(&yytext[1], yyleng-2);
	yylval.id=g_quark_to_string(g_quark_from_string(tmp));
	g_free (tmp);
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

