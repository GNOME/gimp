#include "output.h"

typedef struct {
	gint i;
	PrimType* t;
}FlagData;

void pr_flags_member(File* s, Id id, FlagData* d){
	pr(s,
	   "\t%3 = 1 << %d,\n",
	   pr_macro_name, d->t, NULL, id,
	   d->i);
	d->i++;
}

void pr_flags_decl(File* s, FlagsDef* e){
	FlagData d={
		0,
		DEF(e)->type
	};
	pr(s,
	   "typedef enum {\n"
	   "%3"
	   "} %1;\n"
	   "const %1 %3 = %3;\n",
	   pr_list_foreach, e->flags, pr_flags_member, &d,
	   pr_primtype, d.t,
	   pr_primtype, d.t,
	   pr_macro_name, d.t, NULL, "LAST",
	   pr_macro_name, d.t, NULL, g_slist_last(e->flags)->data);
}

void pr_flags_value(File* s, Id i, PrimType* t){
	pr(s,
	   "\t\t{%3,\n"
	   "\t\t\"%3\",\n"
	   "\t\t\"%s\"},\n",
	   pr_macro_name, t, NULL, i,
	   pr_macro_name, t, NULL, i,
	   i);
}

void output_flags_type_init(FlagsDef* e){
	PrimType* t=DEF(e)->type;
	output_func(t, "init_type", NULL, type_gtk_type, type_hdr, NULL,
		    FALSE, TRUE,
		    "\tstatic GtkFlagValue values[%d] = {\n"
		    "%3"
		    "\t\t{0, NULL, NULL}\n"
		    "\t};\n"
		    "\t%2 = gtk_type_register_flags (\"%1\", values);\n"
		    "\treturn %2;\n",
		    g_slist_length(e->flags)+1,
		    pr_list_foreach, e->flags, pr_flags_value, t,
		    pr_internal_varname, t, "type",
		    pr_primtype, t,
		    pr_internal_varname, t, "type");
}
	   
void output_flags(FlagsDef* e){
	output_def(DEF(e));
	pr_flags_decl(type_hdr, e);
	output_flags_type_init(e);
}

DefClass flags_class={
	output_flags
};

Type* type_gtk_type;
