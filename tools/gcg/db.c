#include "gcg.h"


static GHashTable* module_hash;

void init_db(void){
	module_hash=g_hash_table_new(NULL, NULL);
}

PrimType* get_type(Id modname, Id name){
	Module* m = get_mod(modname);
	PrimType* p = g_hash_table_lookup(m->decl_hash, name);
	if(!p){
		p=g_new(PrimType, 1);
		p->module=m;
		p->name=name;
		p->kind=GTK_TYPE_INVALID;
		p->decl_header=NULL;
		p->def_header=NULL;
		p->definition=NULL;
		g_hash_table_insert(m->decl_hash, name, p);
	}
	return p;
}

void put_def(Def* d){
	PrimType* t=d->type;
	g_assert(t);
	t->definition=d;
}

Def* get_def(Id modname, Id type){
	PrimType* t=get_type(modname, type);
	g_assert(t);
	return t->definition;
}

Module* get_mod(Id modname){
	Module* m=g_hash_table_lookup(module_hash, modname);
	if(!m){
		m=g_new(Module, 1);
		m->name=modname;
		m->decl_hash=g_hash_table_new(NULL, NULL);
		m->common_header=NULL;
		g_hash_table_insert(module_hash, modname, m);
	}
	return m;
}

typedef struct{
	DefFunc f;
	gpointer user_data;
}DFEdata;

void dfe_bar(gpointer key, gpointer p, gpointer foo){
	PrimType* t=p;
	DFEdata* d=foo;
	if(t->definition)
		(d->f)(t->definition, d->user_data);
}


void dfe_foo(gpointer key, gpointer p, gpointer dfed){
	Module* m=p;
	g_hash_table_foreach(m->decl_hash, dfe_bar, dfed);
}

void foreach_def(DefFunc f, gpointer user_data){
	DFEdata d={f,user_data};
	g_hash_table_foreach(module_hash, dfe_foo, &d);
};

