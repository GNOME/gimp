#include "gcg.h"
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

Id func_hdr_name;
Id type_hdr_name;
Id prot_hdr_name;
Id source_name;
Id impl_name;


void get_options(int argc, char* argv[]){
	gint x=0;
	do{
		x=getopt(argc, argv, "f:t:p:s:i:");
		switch(x){
		case 'f':
			func_hdr_name=optarg;
			break;
		case 't':
			type_hdr_name=optarg;
			break;
		case 'p':
			prot_hdr_name=optarg;
			break;
		case 's':
			source_name=optarg;
			break;
		case 'i':
			impl_name=optarg;
			break;
		case '?':
		case ':':
			g_error("Bad option %c!\n", x);
		}
	}while(x!=EOF);
}


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
	get_options(argc, argv);
	yyin=fopen(argv[optind], "r");
	g_assert(yyin);
	yyparse();
	type_gtk_type=g_new(Type, 1);
	type_gtk_type->is_const=FALSE;
	type_gtk_type->indirection=0;
	type_gtk_type->notnull=FALSE;
	type_gtk_type->prim=get_decl(GET_ID("Gtk"), GET_ID("Type"));
	g_assert(type_gtk_type->prim);
	func_hdr=file_new(func_hdr_name);
	type_hdr=file_new(type_hdr_name);
	prot_hdr=file_new(prot_hdr_name);
	source=file_new(source_name);
	source_head=file_sub(source);
	g_hash_table_foreach(def_hash, output_type, NULL);
	file_flush(func_hdr);
	file_flush(type_hdr);
	file_flush(source_head);
	/*	file_flush(source);*/
	file_flush(prot_hdr);
	return 0;
}



