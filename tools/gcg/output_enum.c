#include "output.h"

void pr_enum_member(File* s, Id id, PrimType* t){
	pr(s,
	   "\t%3,\n",
	   pr_macro_name, t, NULL, id);
}

void pr_enum_decl(File* s, EnumDef* e){
	pr(s,
	   "typedef enum {\n"
	   "%3"
	   "} %1;\n",
	   pr_list_foreach, e->alternatives, pr_enum_member, DEF(e)->type,
	   pr_primtype, DEF(e)->type);
}

void pr_enum_value(File* s, Id i, PrimType* t){
	pr(s,
	   "\t\t{%3,\n"
	   "\t\t\"%3\",\n"
	   "\t\t\"%s\"},\n",
	   pr_macro_name, t, NULL, i,
	   pr_macro_name, t, NULL, i,
	   i);
}

void output_enum_type_init(EnumDef* e){
	PrimType* t=DEF(e)->type;
	output_func(t, "init_type", NULL, type_gtk_type, type_hdr, NULL,
		    FALSE, TRUE,
		    "\tstatic GtkEnumValue values[%d] = {\n"
		    "%3"
		    "\t\t{0, NULL, NULL}\n"
		    "\t};\n"
		    "\t%2 = gtk_type_register_enum (\"%1\", values);\n"
		    "\treturn %2;\n",
		    g_slist_length(e->alternatives)+1,
		    pr_list_foreach, e->alternatives, pr_enum_value, t,
		    pr_internal_varname, t, "type",
		    pr_primtype, t,
		    pr_internal_varname, t, "type");
}
	   
void output_enum(EnumDef* e){
	output_def(DEF(e));
	output_enum_type_init(e);
	pr_enum_decl(type_hdr, e);
}

DefClass enum_class={
	output_enum
};

	
