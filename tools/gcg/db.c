#include "gcg.h"


static GHashTable* package_hash = NULL;
static GSList* def_list = NULL;


void init_db(void){
	package_hash = g_hash_table_new(NULL, NULL);
}

PrimType* get_type(Package* pkg, Id name){
	return g_hash_table_lookup(pkg->type_hash, name);
}

void put_type(PrimType* t){
	g_hash_table_insert(t->module->package->type_hash,
			    (gpointer)t->name, t);
}

void put_def(Def* d){
	def_list = g_slist_append (def_list, d);
}

Package* get_pkg(Id pkgname){
	return g_hash_table_lookup(package_hash, pkgname);
}

void put_pkg(Package* pkg){
	g_hash_table_insert(package_hash, (gpointer)pkg->name, pkg);
}

Module* get_mod(Package* pkg, Id modname){
	return g_hash_table_lookup(pkg->mod_hash, modname);
}

void put_mod(Module* m){
	g_hash_table_insert(m->package->mod_hash, (gpointer)m->name, m);
}

void foreach_def(DefFunc f, gpointer user_data){
	GSList* l = def_list;
	while(l){
		f(l->data, user_data);
		l = l->next;
	}
}
