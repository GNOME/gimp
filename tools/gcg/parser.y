%{
#include "gcg.h"
#define YYDEBUG 1
%}

%union {
	GSList* list;
	Id id;
	GString* str;
	TypeName typename;
	PrimType* primtype;
	MethodProtection methprot;
	DataProtection dataprot;
	MemberKind kind;
	Def* def;
	ObjectDef* class_def;
	Member* member;
	Param* param;
	EmitDef emit_def;
	gboolean bool;
	Type type;
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


%token<id> T_IDENT
%token<id> T_HEADERNAME
%token<str> T_STRING

%type<type> type
%type<type> typeorvoid
%type<type> semitype
%type<typename> typename
%type<primtype> primtype
%type<list> paramlist
%type<list> idlist
%type<kind> kinddef
%type<methprot> methprot
%type<dataprot> dataprot
%type<list> classbody
%type<def> flagsdef
%type<def> enumdef
%type<def> classdef
%type<def> def
%type<primtype> object_type
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

deffile: /* empty */ | deffile import | deffile modulescope;

import: T_IMPORT T_IDENT T_END {
	imports=g_slist_prepend(imports, (gpointer)($2));
}

modulescope: T_MODULE T_IDENT T_OPEN_B {
	current_module=$2;
} modulebody T_CLOSE_B {
	current_module=NULL;
} T_END;

modulebody: /* empty */ | modulebody headerdef | modulebody def {
	put_def ($2);
	};

headerdef: T_HEADER T_HEADERNAME T_OPEN_B {
	current_header=$2;
} decllist T_CLOSE_B {
	current_header=NULL;
} T_END;

decllist: /* empty */ | decllist decl;

decl: simpledecl | classdecl | opaquedecl | protclassdecl;

simpledecl: T_TYPE typename T_END {
	PrimType* t=g_new(PrimType, 1);
	t->name = $2;
	t->kind=TYPE_TRANSPARENT;
	t->decl_header = t->def_header = current_header;
	put_decl(t);
};

protclassdecl: T_PROTECTED T_CLASS typename T_END {
	PrimType* t;
	t=get_decl($3.module, $3.type);
	if(!t){
		t=g_new(PrimType, 1);
		t->name=$3;
		t->kind=TYPE_CLASS;
		t->decl_header=NULL;
	}
	g_assert(t->kind==TYPE_CLASS);
	t->def_header=current_header;
	put_decl(t);
}

classdecl: T_CLASS typename T_END {
	PrimType* t;
	t=get_decl($2.module, $2.type);
	if(!t){
		t=g_new(PrimType, 1);
		t->name=$2;
		t->kind=TYPE_CLASS;
		t->def_header=NULL;
	}
	g_assert(t->kind==TYPE_CLASS);
	t->decl_header=current_header;
	put_decl(t);
}

opaquedecl: T_OPAQUE typename T_END {
	PrimType* t=g_new(PrimType, 1);
	t->name = $2;
	t->kind=TYPE_OPAQUE;
	t->decl_header = NULL;
	t->def_header = current_header;
	g_assert(!get_decl($2.module, $2.type));
	put_decl(t);
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
}

typeorvoid: type | T_VOID {
	$$.prim = NULL;
	$$.indirection = 0;
	$$.is_const = FALSE;
	$$.notnull = FALSE;
}
	
primtype: typename {
	$$=get_decl($1.module, $1.type);
	g_assert($$);
};

typename: T_IDENT T_SCOPE T_IDENT {
	$$.module=$1;
	$$.type=$3;
} | T_IDENT {
	g_assert(current_module);
	$$.module=current_module;
	$$.type=$1;
};

object_type: primtype {
	$$=$1;
	g_assert($$->kind==TYPE_CLASS);
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

param: type T_IDENT docstring {
	$$=g_new(Param, 1);
	$$->method=current_method;
	$$->doc=$3;
	$$->name=$2;
	$$->type=$1;
};

kinddef: T_ABSTRACT {
	$$ = KIND_ABSTRACT;
} | T_DIRECT {
	$$ = KIND_DIRECT;
} | /* empty */ {
	$$ = KIND_DIRECT;
} | T_STATIC {
	$$ = KIND_STATIC;
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


idlist: T_IDENT {
	$$ = g_slist_prepend(NULL, (gpointer)($1));
} | idlist T_COMMA T_IDENT {
	$$ = g_slist_append($1, (gpointer)($3));
};

def: classdef | enumdef | flagsdef;

enumdef: T_ENUM primtype T_OPEN_B idlist T_CLOSE_B docstring T_END {
	EnumDef* d=g_new(EnumDef, 1);
	g_assert($2->kind==TYPE_TRANSPARENT);
	d->alternatives = $4;
	$$=DEF(d);
	$$->klass=&enum_class;
	$$->type=$2;
	$$->doc=$6;
};

flagsdef: T_FLAGS primtype T_OPEN_B idlist T_CLOSE_B docstring T_END {
	FlagsDef* d=g_new(FlagsDef, 1);
	g_assert($2->kind==TYPE_TRANSPARENT);
	d->flags = $4;
	$$=DEF(d);
	$$->klass=&flags_class;
	$$->type=$2;
	$$->doc=$6;
};

classdef: T_CLASS object_type T_INHERITANCE object_type docstring T_OPEN_B {
	current_class=g_new(ObjectDef, 1);
} classbody T_CLOSE_B T_END {
	current_class->parent = $4;
	current_class->members = $8;
	$$=DEF(current_class);
	current_class=NULL;
	$$->klass=&object_class;
	$$->type = $2;
	$$->doc = $5;
};

member_def: data_member_def | method_def;


data_member_def: dataprot kinddef type T_IDENT emitdef docstring T_END {
	DataMember* m = g_new(DataMember, 1);
	m->prot = $1;
	m->type = $3;
	$$ = MEMBER(m);
	$$->membertype = MEMBER_DATA;
	$$->kind = $2;
	$$->name = $4;
/* 	$$->emit = $5; */
	$$->doc = $6;
};

method_def: methprot kinddef typeorvoid T_IDENT T_OPEN_P {
	current_method = g_new(Method, 1);
} paramlist T_CLOSE_P const_def emitdef docstring T_END {
	current_method = NULL;
	current_method->prot = $1;
	current_method->ret_type = $3;
	current_method->self_const = $9;
	current_method->params = $7;
	$$=MEMBER(current_method);
	current_method=NULL;
	$$->membertype = MEMBER_METHOD;
	$$->kind = $2;
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
	$$ = g_slist_prepend($1, $2);
};

%%
#define YYDEBUG 1

GHashTable* type_hash;
GHashTable* class_hash;


int yyerror (char* s){
	g_error ("Parser error: %s", s);
}



