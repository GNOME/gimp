#include "gcg.h"


void put_decl(PrimType* t){
	g_hash_table_insert(decl_hash, &t->name, t);
}

void put_def(Def* d){
	g_hash_table_insert(def_hash, &d->type->name, d);
}

Def* get_def(Id module, Id type){
	TypeName n;
	n.module=module;
	n.type=type;
	return g_hash_table_lookup(def_hash, &n);
}

PrimType* get_decl(Id module, Id type){
	TypeName n;
	n.module=module;
	n.type=type;
	return g_hash_table_lookup(decl_hash, &n);
}

