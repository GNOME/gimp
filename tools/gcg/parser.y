%{
#include "gcg.h"
#include "parse.h"
	
#define YYDEBUG 1

static Package* current_package;
Module* current_module;
static ObjectDef* current_class;
static Method* current_method;
static PrimType* root_class;
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
	TypeKind fund_type;
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
%token T_FOREIGN
%token T_PACKAGE
%token T_ROOT
%token T_CHAR

%token<id> T_IDENT
%token<id> T_HEADERNAME
%token<str> T_STRING

%type<id> maybeident
%type<id> ident
%type<id> headerdef
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

declarations: /* empty */ | declarations package | declarations rootdef;


rootdef: T_ROOT primtype T_END {
	if($2->kind != TYPE_OBJECT)
		g_error("Bad root type: %s.%s",
			$2->module->package->name,
			$2->name);
	root_class = $2;
};


definitions: current_module_def deflist;

deflist: /* empty */ | deflist def {
	put_def($2);
} | deflist import;

import: T_IMPORT importlist T_END;

importlist: ident {
	Package* p = get_pkg($1);
	if(!p)
		g_error("Attempt to import unknown package %s", $1);
	imports_list = g_slist_prepend(imports_list, p);
} | importlist T_COMMA ident {
	Package* p = get_pkg($3);
	if(!p)
		g_error("Attempt to import unknown package %s", $3);
	imports_list = g_slist_prepend(imports_list, p);
}

package: T_PACKAGE maybeident headerdef T_OPEN_B {
	Package* p;
	Id i = $2;
	if(!i)
		i = GET_ID("");
	p = get_pkg(i);
	if(!p){
		p = g_new(Package, 1);
		p->name = i;
		p->type_hash = g_hash_table_new(NULL, NULL);
		p->mod_hash = g_hash_table_new(NULL, NULL);
		p->headerbase = $3;
		put_pkg(p);
	}
	current_package = p;
} modulelist T_CLOSE_B {
	current_package = NULL;
};

current_module_def: T_MODULE ident T_SCOPE ident T_END {
	Package* p = get_pkg($2);
	Module* m;
	if(!p)
		g_error("Unknown package %s", $2);
	m = get_mod(p, $4);
	if(!m)
		g_error("Unknown module %s.%s", $2, $4);
	current_module = m;
}


modulelist: /* empty */ | modulelist module;

headerdef: /* empty */ {
	$$ = NULL;
}| T_HEADERNAME;


maybeident: ident | /* empty */ {
	$$ = NULL;
};

	

module: T_MODULE maybeident headerdef T_OPEN_B {
	Module* m = get_mod(current_package, $2);
	if(!m){
		m = g_new(Module, 1);
		m->package = current_package;
		m->name = $2;
		m->header = $3;
		put_mod(m);
	}
	current_module = m;
} decllist T_CLOSE_B;

decllist: /* empty */ | decllist decl;

decl: simpledecl ;/* | classdecl | opaquedecl | protclassdecl;*/

fundtype: T_INT {
	$$ = TYPE_INT;
} | T_DOUBLE {
	$$ = TYPE_DOUBLE;
} | T_CLASS {
	$$ = TYPE_OBJECT;
} | T_ENUM {
	$$ = TYPE_ENUM;
} | T_FLAGS {
	$$ = TYPE_FLAGS;
} | T_BOXED {
	$$ = TYPE_BOXED;
} | T_FOREIGN {
	$$ = TYPE_FOREIGN;
} | T_CHAR {
	$$ = TYPE_CHAR;
};



simpledecl: fundtype ident T_END {
	PrimType* t = g_new(PrimType, 1);
	t->module = current_module;
	t->name = $2;
	t->kind = $1;
	put_type(t);
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

type: semitype {
	if(!$$.indirection){
		g_assert($$.prim->kind != TYPE_OBJECT);
		g_assert($$.prim->kind != TYPE_BOXED);
	}
} | semitype T_NOTNULLPTR {
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

	
primtype: maybeident T_SCOPE ident {
	Id i = $1;
	Package* p;
	PrimType* t;
	if(!i)
		i = GET_ID("");
	p = get_pkg(i);
	if(!p)
		g_error("Unknown package %s!", i);
	t = get_type(p, $3);
	if(!t)
		g_error("Unknown type %s:%s", i, $3);
	$$ = t;
} | ident {
	Package* p = current_module->package;
	PrimType* t;
	GSList* l = imports_list;
	t = get_type(p, $1);
	while(l && !t){
		t = get_type(l->data, $1);
		l = l->next;
	}
	if(!t)
		g_error("Couldn't find type %s in import list", $1);
	$$ = t;
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

enumdef: T_ENUM primtype docstring T_OPEN_B idlist T_CLOSE_B {
	EnumDef* d=g_new(EnumDef, 1);
	g_assert($2->kind==TYPE_ENUM);
	d->alternatives = $5;
	$$=DEF(d);
	$$->type=$2;
	$$->doc=$3;
};

flagsdef: T_FLAGS primtype docstring T_OPEN_B idlist T_CLOSE_B T_END {
	FlagsDef* d=g_new(FlagsDef, 1);
	g_assert($2->kind==TYPE_ENUM);
	d->flags = $5;
	$$=DEF(d);
	$$->type=$2;
	$$->doc=$3;
};

parent: /* empty */{
	g_assert(root_class);
	$$ = root_class;
} | T_INHERITANCE primtype{
	$$=$2;
}

classdef: T_CLASS primtype parent docstring T_OPEN_B {
	g_assert($2->kind==TYPE_OBJECT);
	g_assert(!$3 || $3->kind==TYPE_OBJECT);
	g_assert($2->module == current_module);
	current_class=g_new(ObjectDef, 1);
} classbody T_CLOSE_B {
	Type t;
	t.is_const = FALSE;
	t.indirection = 1;
	t.notnull = TRUE;
	t.prim = $2;
	current_class->self_type[0]=t;
	t.is_const=TRUE;
	current_class->self_type[1]=t;
	current_class->parent = $3;
	current_class->members = g_slist_reverse($7);
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
