#include "gcg.h"
#include "parser.h"
#include <stdio.h>
#include <glib.h>
#include <unistd.h>
#include "output.h"

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

void output_cb(Def* def, gpointer out){
	output_def(out, def);
}

int main(int argc, char* argv[]){
	/*	target=stdout;*/
	PRoot* out=pr_new();
	const gchar* tag_type="type";
	const gchar* tag_source="source";
	const gchar* tag_source_head="source_head";
	const gchar* tag_functions="functions";
	const gchar* tag_protected="protected";
	
	FILE* f;
	init_db();
	yydebug=0;
	get_options(argc, argv);
	yyin=fopen(argv[optind], "r");
	g_assert(yyin);
	yyparse();
	type_gtk_type=g_new(Type, 1);
	type_gtk_type->is_const=FALSE;
	type_gtk_type->indirection=0;
	type_gtk_type->notnull=FALSE;
	type_gtk_type->prim=get_type(GET_ID("Gtk"), GET_ID("Type"));
	g_assert(type_gtk_type->prim);
	
	foreach_def(output_cb, out);
	f=fopen(type_hdr_name, "w+");
	pr_write(out, f, &tag_type, 1);
	f=fopen(source_name, "w+");
	pr_write(out, f, &tag_source_head, 1);
	pr_write(out, f, &tag_source, 1);
	f=fopen(func_hdr_name, "w+");
	pr_write(out, f, &tag_functions, 1);
	f=fopen(prot_hdr_name, "w+");
	pr_write(out, f, &tag_protected, 1);
	return 0;
}



