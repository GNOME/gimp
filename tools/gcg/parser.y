%{
#include "gcg.h"
#define YYDEBUG 1

static Module* current_module;
static Id current_header;
static ObjectDef* current_class;
static GSList* imports;
static Method* current_method;

static GSList* imports_list;
	
%}

%union {
	GSList* list;
	Id id;
	GString* str;
	PrimType* primtype;
	MethodKind methkind;
	DataMemberKind datakind;
	MethProtection methprot;
	DataProtection dataprot;
	MethodKind kind;
	Def* def;
	ObjectDef* class_def;
	Member* member;
	Param* param;
	EmitDef emit_def;
	gboolean bool;
	Type type;
	GtkFundamentalType fund_type;
};

%token T_MODULE
%token T_ENUM
%token T_FLAGS
%token T_READ_WRITE
%token T_READ_ONLY
%token T_PRIVATE
%token T_PROTECTED
%token T_PUBLIC
%token T_CLASS
%token T_PRE_EMIT
%token T_POST_EMIT
%token T_DUAL_EMIT
%token T_SCOPE
%token T_ABSTRACT
%token T_EMPTY
%token T_DIRECT
%token T_STATIC
%token T_CONST
%token T_TYPE
%token T_END
%token T_OPEN_B
%token T_POINTER
%token T_NOTNULLPTR
%token T_CLOSE_B
%token T_INHERITANCE
%token T_ATTRIBUTE
%token T_HEADER
%token T_OPEN_P
%token T_HEADER
%token T_CLOSE_P
%token T_COMMA
%token T_IMPORT
%token T_OPAQUE
%token T_VOID
%token T_INT
%token T_DOUBLE
%token T_BOXED
%token T_SIGNAL

%token<id> T_IDENT
%token<id> T_HEADERNAME
%token<str> T_STRING

%type<id> ident
%type<fund_type> fundtype
%type<type> type
%type<type> typeorvoid
%type<type> semitype
%type<primtype> primtype
%type<primtype> parent
%type<list> paramlist
%type<list> idlist
%type<methkind> methkind
%type<datakind> datakind
%type<methprot> methprot
%type<dataprot> dataprot
%type<list> classbody
%type<def> flagsdef
%type<def> enumdef
%type<def> classdef
%type<def> def
%type<list> paramtail
%type<param> param
%type<str> docstring
%type<member> member_def
%type<member> data_member_def
%type<member> method_def
%type<emit_def> emitdef
%type<bool> const_def;

%% /* Grammar rules and actions follow */

start_symbol: deffile ;

deffile: declarations definitions;

declarations: /* empty */ | declarations modulescope;

definitions: current_module_def deflist;

deflist: /* empty */ | deflist def {
	put_def($2);
};

import: T_IMPORT ident T_END {
	imports=g_slist_prepend(imports, (gpointer)($2));
}

modulescope: T_MODULE ident T_OPEN_B {
	current_module=get_mod($2);
} decllist T_CLOSE_B {
	current_module=NULL;
} T_END;

current_module_def: T_MODULE ident T_END {
	current_module=get_mod($2);
};

decllist: /* empty */ | decllist decl;

decl: simpledecl ;/* | classdecl | opaquedecl | protclassdecl;*/

fundtype: T_INT {
	$$ = GTK_TYPE_INT;
} | T_DOUBLE {
	$$ = GTK_TYPE_DOUBLE;
} | T_BOXED {
	$$ = GTK_TYPE_BOXED;
} | T_CLASS {
	$$ = GTK_TYPE_OBJECT;
} | T_ENUM {
	$$ = GTK_TYPE_ENUM;
} | T_FLAGS {
	$$ = GTK_TYPE_FLAGS;
}

simpledecl: fundtype primtype T_END {
	g_assert($2->kind==GTK_TYPE_INVALID);
	$2->kind = $1;
};

semitype: const_def primtype {
	$$.is_const = $1;
	$$.indirection = 0;
	$$.notnull = FALSE;
	$$.prim = $2;
} | semitype T_POINTER {
	$$ = $1;
	$$.indirection++;
};

type: semitype | semitype T_NOTNULLPTR {
	$$ = $1;
	$$.indirection++;
	$$.notnull = TRUE;
};


ident: T_IDENT;

typeorvoid: type | T_VOID {
	$$.prim = NULL;
	$$.indirection = 0;
	$$.is_const = FALSE;
	$$.notnull = FALSE;
};

	
primtype: ident T_SCOPE ident {
	$$ = get_type($1, $3);
} | ident {
	$$ = get_type(current_module->name, $1);
};

paramlist: /* empty */ {
        $$ = NULL;
} | param paramtail {
	$$ = g_slist_prepend($2, $1);
};

paramtail: /* empty */ {
	$$ = NULL;
} | T_COMMA param paramtail {
	$$ = g_slist_prepend($3, $2);
};

param: type ident docstring {
	$$=g_new(Param, 1);
	$$->method=current_method;
	$$->doc=$3;
	$$->name=$2;
	$$->type=$1;
};

methkind: T_ABSTRACT {
	$$ = METH_VIRTUAL;
} | T_DIRECT {
	$$ = METH_DIRECT;
} | /* empty */ {
	$$ = METH_DIRECT;
} | T_STATIC {
	$$ = METH_STATIC;
} | T_PRE_EMIT {
	$$ = METH_EMIT_PRE;
} | T_POST_EMIT {
	$$ = METH_EMIT_POST;
} | T_DUAL_EMIT {
	$$ = METH_EMIT_BOTH;
};


datakind: T_ABSTRACT {
	$$ = DATA_STATIC_VIRTUAL;
} | T_DIRECT {
	$$ = DATA_DIRECT;
} | /* empty */ {
	$$ = DATA_DIRECT;
} | T_STATIC {
	$$ = DATA_STATIC;
};


methprot: T_PROTECTED{
	$$ = METH_PROTECTED;
} | T_PUBLIC {
	$$ = METH_PUBLIC;
};

dataprot: /* empty */ {
	$$ = DATA_PROTECTED;
} | T_READ_ONLY {
	$$ = DATA_READONLY;
} | T_READ_WRITE {
	$$ = DATA_READWRITE;
};

emitdef: /* empty */ {
	$$ = EMIT_NONE;
} | T_PRE_EMIT {
	$$ = EMIT_PRE;
} | T_POST_EMIT {
	$$ = EMIT_POST;
};

docstring: T_STRING {
	$$ = $1;
} | /* empty */ {
	$$ = NULL;
};


idlist: ident {
	$$ = g_slist_prepend(NULL, (gpointer)($1));
} | idlist T_COMMA ident {
	$$ = g_slist_append($1, (gpointer)($3));
};

def: classdef | enumdef | flagsdef;

enumdef: T_ENUM primtype T_OPEN_B idlist T_CLOSE_B docstring T_END {
	EnumDef* d=g_new(EnumDef, 1);
	g_assert($2->kind==GTK_TYPE_ENUM);
	d->alternatives = $4;
	$$=DEF(d);
	$$->type=$2;
	$$->doc=$6;
};

flagsdef: T_FLAGS primtype T_OPEN_B idlist T_CLOSE_B docstring T_END {
	FlagsDef* d=g_new(FlagsDef, 1);
	g_assert($2->kind==GTK_TYPE_ENUM);
	d->flags = $4;
	$$=DEF(d);
	$$->type=$2;
	$$->doc=$6;
};

parent: /* empty */{
	$$=NULL;
} | T_INHERITANCE primtype{
	$$=$2;
}

classdef: T_CLASS primtype parent docstring T_OPEN_B {
	g_assert($2->kind==GTK_TYPE_OBJECT);
	g_assert(!$3 || $3->kind==GTK_TYPE_OBJECT);
	current_class=g_new(ObjectDef, 1);
} classbody T_CLOSE_B T_END {
	Type t={FALSE, 1, TRUE, $2};
	current_class->self_type[0]=t;
	t.is_const=TRUE;
	current_class->self_type[1]=t;
	current_class->parent = $3;
	current_class->members = $7;
	$$=DEF(current_class);
	current_class=NULL;
	$$->type = $2;
	$$->doc = $4;
};

member_def: data_member_def | method_def;



data_member_def: dataprot datakind type ident emitdef docstring T_END {
	DataMember* m = g_new(DataMember, 1);
	m->prot = $1;
	m->type = $3;
	m->kind = $2;
	$$ = MEMBER(m);
	$$->membertype = MEMBER_DATA;
	$$->name = $4;
/* 	$$->emit = $5; */
	$$->doc = $6;
};

method_def: methprot methkind typeorvoid ident T_OPEN_P {
	current_method = g_new(Method, 1);
} paramlist T_CLOSE_P const_def emitdef docstring T_END {
	current_method->prot = $1;
	current_method->ret_type = $3;
	current_method->self_const = $9;
	current_method->params = $7;
	current_method->kind = $2;
	$$=MEMBER(current_method);
	current_method=NULL;
	$$->membertype = MEMBER_METHOD;
	$$->name = $4;
	/* $$->emit = $10; */
	$$->doc = $11;
};

const_def: T_CONST {
	$$ = TRUE;
} | /* empty */ {
	$$ = FALSE;
};

classbody: /* empty */ {
	$$ = NULL;
} | classbody member_def{
	$2->my_class=current_class;
	$$ = g_slist_prepend($1, $2);
};

%%
#define YYDEBUG 1

GHashTable* type_hash;
GHashTable* class_hash;

 gboolean in_ident;

int yyerror (char* s){
	g_error ("Parser error: %s", s);
	return 0;
}
