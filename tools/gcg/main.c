#include "classc.h"
#include "parser.h"
#include <stdio.h>
#include <glib.h>
#include <unistd.h>
#include "output.h"

GHashTable* decl_hash;
GHashTable* def_hash;
Id current_module;
Method* current_method;
ObjectDef* current_class;
Id current_header;
GSList* imports;


guint type_name_hash(gconstpointer c){
	const TypeName* t=c;
	return g_str_hash(t->module) ^ g_str_hash(t->type);
}

gboolean type_name_cmp(gconstpointer a, gconstpointer b){
	const TypeName *t1=a, *t2=b;
	return (t1->type == t2->type) && (t1->module == t2->module);
}

void output_type(TypeName* t, Def* def, gpointer foo){
	def->klass->output(def);
}

int main(int argc, char* argv[]){
	extern int yydebug;
	extern FILE* yyin;
	/*	target=stdout;*/
	
	decl_hash=g_hash_table_new(type_name_hash, type_name_cmp);
	def_hash=g_hash_table_new(type_name_hash, type_name_cmp);
	yydebug=0;
	yyin=fopen(argv[1], "r");
	yyparse();
	type_gtk_type=g_new(Type, 1);
	type_gtk_type->is_const=FALSE;
	type_gtk_type->indirection=0;
	type_gtk_type->notnull=FALSE;
	type_gtk_type->prim=get_decl(GET_ID("Gtk"), GET_ID("Type"));
	g_assert(type_gtk_type->prim);
	func_hdr=file_new("gen_funcs.h");
	type_hdr=file_new("gen_types.h");
	prot_hdr=file_new("gen_prots.h");
	source=file_new("gen_source.c");
	source_head=file_sub(source);
	g_hash_table_foreach(def_hash, output_type, NULL);
	file_flush(func_hdr);
	file_flush(type_hdr);
	file_flush(source_head);
	/*	file_flush(source);*/
	file_flush(prot_hdr);
	return 0;
}



