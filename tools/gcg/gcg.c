#include "gcg.h"
#include "parser.h"
#include <stdio.h>
#include <glib.h>

#define __USE_POSIX2

#include <unistd.h>
#include "output.h"

#ifndef CPP
#define CPP "cpp"
#endif


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
	
	FILE* f;
	init_db();
	yydebug=0;
	get_options(argc, argv);
	yyin=fopen(argv[optind], "r");
	g_assert(yyin);
	yyparse();
	
	foreach_def(output_cb, out);
	f=fopen(type_hdr_name, "w+");
	pr_write(out, f, "type");
	f=fopen(source_name, "w+");
	pr_write(out, f, "source_deps");
	pr_write(out, f, "source_head");
	pr_write(out, f, "source");
	f=fopen(func_hdr_name, "w+");
	pr_write(out, f, "func_deps");
	pr_write(out, f, "functions");
	f=fopen(prot_hdr_name, "w+");
	pr_write(out, f, "prot_deps");
	pr_write(out, f, "protected");
	return 0;
}



