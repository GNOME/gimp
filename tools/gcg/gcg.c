#include "gcg.h"
#include "parser.h"
#include <stdio.h>
#include <glib.h>
#include <unistd.h>
#include "p_output.h"

GHashTable* decl_hash;
GHashTable* def_hash;
Id current_module;
Method* current_method;
ObjectDef* current_class;
Id current_header;
GSList* imports;
Type* type_gtk_type;

Id func_hdr_name;
Id type_hdr_name;
Id prot_hdr_name;
Id source_name;
Id impl_name;
extern int yydebug;
extern FILE* yyin;

extern int yyparse(void);

void get_options(int argc, char* argv[]){
	gint x=0;
	do{
		x=getopt(argc, argv, "f:t:p:s:i:d");
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
		case 'd':
			yydebug=1;
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

void output_cb(gpointer typename, gpointer def, gpointer ctx){
	(void)typename; /* Shut off warnings */
	output_def(ctx, def);
}

int main(int argc, char* argv[]){
	/*	target=stdout;*/
	OutCtx ctx;
	
	FILE* f;
	
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
	ctx.type_hdr=pr_new();
	ctx.func_hdr=pr_new();
	ctx.prot_hdr=pr_new();
	ctx.pvt_hdr=pr_new();
	ctx.src=pr_new();
	
	g_hash_table_foreach(def_hash, output_cb, &ctx);
	f=fopen(type_hdr_name, "w+");
	pr_write(ctx.type_hdr, f);
	f=fopen(source_name, "w+");
	pr_write(ctx.pvt_hdr, f);
	pr_write(ctx.src, f);
	f=fopen(func_hdr_name, "w+");
	pr_write(ctx.func_hdr, f);
	f=fopen(prot_hdr_name, "w+");
	pr_write(ctx.prot_hdr, f);
	return 0;
}



