#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "gcg.h"
#include "parse.h"
#include <unistd.h>
#include "output.h"

#ifndef CPP
#define CPP "cpp"
#endif

Id header_root = "..";
Id source_name = NULL;
Id impl_name = NULL;

gboolean collect_marshall = FALSE;

GString* cpp_cmd;

void get_options(int argc, char* argv[]){
	gint x=0;
	yydebug = yy_flex_debug = FALSE;
	do{
		x=getopt(argc, argv, "D:i:dI:o:m");
		switch(x){
		case 'D':
			header_root=optarg;
			break;
		case 'I':
			g_string_append(cpp_cmd, " -I ");
			g_string_append(cpp_cmd, optarg);
			break;
		case 'i':
			impl_name=optarg;
			break;
		case 'd':
			if(!yydebug)
				yydebug = TRUE;
			else
				yy_flex_debug = TRUE;
			break;
		case 'o':
			source_name = optarg;
			break;
		case 'm':
			collect_marshall = TRUE;
		case '?':
		case ':':
			g_error("Bad option %c!\n", x);
		}
	}while(x!=EOF);
}

void output_cb(Def* def, gpointer out){
	output_def(out, def);
}



void open_out(PNode*(*func)(Module*),
	       Id suffix, PNode* n, PRoot* out){
	GString* s = g_string_new(header_root);
	gchar* str;
	PNode* guard;
	FILE* f;
	g_string_append(s, "/");
	str = p_to_str(func(current_module), NULL);
	g_string_append(s, str);
	g_free(str);
	f = fopen(s->str, "w+");
	if(!f)
		g_error("Unable to open file %s: %s",
			s->str,
			strerror(errno));
	g_string_free(s, TRUE);
	guard=p_fmt("_g_~_~_~",
		    p_c_ident(current_module->package->name),
		    p_c_ident(current_module->name),
		    p_str(suffix));
	p_write(p_fmt("#ifndef ~\n"
		      "#define ~\n"
		      "~"
		      "#endif /* ~ */\n",
		      guard,
		      guard,
		      n,
		      guard),
		f, out);
	fclose(f);
}	
	
int main(int argc, char* argv[]){
	/*	target=stdout;*/
	PRoot* out=pr_new();
	FILE* f;
	
	init_db();
	cpp_cmd = g_string_new(CPP);
	yydebug=0;
	get_options(argc, argv);
	g_string_append(cpp_cmd, " ");
	g_string_append(cpp_cmd, argv[optind]);

	
	yyin=popen(cpp_cmd->str, "r");
	/*yyin=fopen(argv[optind], "r");*/
	g_assert(yyin);
	yyparse();
	
	if(!impl_name)
		impl_name = p_to_str(p_fmt("~.c",
					   p_c_ident(current_module->name)),
				     NULL);
	if(!source_name)
		source_name = p_to_str(p_fmt("~_s.c",
					     p_c_ident(current_module->name)),
				       NULL);
		
	foreach_def(output_cb, out);

	f=fopen(source_name, "w+");
	if(!f)
		g_error("Unable to open file %s: %s",
			source_name, strerror(errno));
	p_write(p_fmt("~~~"
		      "#include \"~\"\n",
		      p_col("source_prot_depends", p_prot_include),
		      p_col("source_head", NULL),
		      p_col("source", NULL),
		      p_str(impl_name)),
		f, out);
	fclose(f);
	

	open_out(p_type_header, "type",
		 p_fmt("#include <gtk/gtktypeutils.h>\n"
		       "~",
		       p_col("type", NULL)),
		 out);
	
	open_out(p_func_header, "funcs",
		   p_fmt("~~~",
			 p_col("func_parent_depends", p_func_include),
			 p_col("func_depends", p_type_include),
			 p_col("functions", NULL)),
		   out);
	
	open_out(p_prot_header, "prot",
		 p_fmt("~~~",
		       p_col("prot_parent_depends", p_prot_include),
		       p_col("prot_depends", p_type_include),
		       p_col("protected", NULL)),
		 out);

	open_out(p_import_header, "import",
		 p_fmt("~~~",
		       p_func_include(current_module),
		       p_col("import_depends", p_import_include),
		       p_col("import_alias", NULL)),
		 out);
return 0;
}



