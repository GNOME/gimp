#include "output.h"

PNode* p_enum_member(Id id, PrimType* t){
	return p_fmt("\t~,\n",
		     p_macro_name(t, NULL, id));
}

PNode* p_enum_decl(EnumDef* e){
	PrimType* t=DEF(e)->type;
	return p_fmt("typedef enum {\n"
		     "~"
		     "\t~ = ~\n"
		     "} ~;\n",
		     p_for(e->alternatives, p_enum_member, t),
		     p_macro_name(t, NULL, "LAST"),
		     p_macro_name(t, NULL,
				   g_slist_last(e->alternatives)->data),
		     p_primtype(t));
}

PNode* p_enum_value(Id i, PrimType* t){
	return p_fmt("\t\t{~,\n"
		     "\t\t\"~\",\n"
		     "\t\t\"~\"},\n",
		     p_macro_name(t, NULL, i),
		     p_macro_name(t, NULL, i),
		     p_str(i));
}

void output_enum_type_init(PRoot* out, EnumDef* e){
	PrimType* t=DEF(e)->type;
	output_func(out,
		    "type",
		    NULL,
		    p_internal_varname(t, p_str("init_type")),
		    p_nil,
		    NULL,
		    p_fmt("\tstatic GtkEnumValue values[~] = {\n"
			  "~"
			  "\t\t{0, NULL, NULL}\n"
			  "\t};\n"
			  "\t~ = gtk_type_register_enum (\"~\", values);\n",
			  p_prf("%d", g_slist_length(e->alternatives)+1),
			  p_for(e->alternatives, p_enum_value, t),
			  p_internal_varname(t, p_str("type")),
			  p_primtype(t),
			  p_internal_varname(t, p_str("type"))));
}
	   
void output_enum(PRoot* out, Def* d){
	EnumDef* e = (EnumDef*)d;
	output_enum_type_init(out, e);
	pr_add(out, "type", p_enum_decl(e));
}

