#include "output.h"

typedef struct {
	gint i;
	PrimType* t;
}FlagData;

PNode* p_flags_member(Id id, FlagData* d){
	d->i++;
	return p_fmt("\t~ = 1 << ~,\n",
		     p_macro_name(d->t, NULL, id),
		     p_prf("%d", d->i));
}

PNode* p_flags_decl(FlagsDef* e){
	FlagData d;
	d.i = -1;
	d.t = DEF(e)->type;
	return p_fmt("typedef enum {\n"
		     "~"
		     "} ~;\n"
		     "const ~ ~ = ~;\n",
		     p_for(e->flags, p_flags_member, &d),
		     p_primtype(d.t),
		     p_primtype(d.t),
		     p_macro_name(d.t, NULL, "LAST"),
		     p_macro_name(d.t, NULL, g_slist_last(e->flags)->data));
}

PNode* p_flags_value(Id i, PrimType* t){
	return p_fmt("\t\t{~,\n"
		     "\t\t\"~\",\n"
		     "\t\t\"~\"},\n",
		     p_macro_name(t, NULL, i),
		     p_macro_name(t, NULL, i),
		     p_str(i));
}

void output_flags_type_init(PRoot* out, FlagsDef* e){
	PrimType* t=DEF(e)->type;
	output_func(out,
		    "type",
		    NULL,
		    p_internal_varname(t, p_str("init_type")),
		    p_nil,
		    NULL,
		    p_fmt("\tstatic GtkFlagValue values[~] = {\n"
			  "~"
			  "\t\t{0, NULL, NULL}\n"
			  "\t};\n"
			  "\t~ = gtk_type_register_flags (\"~\", values);\n",
			  p_prf("%d", g_slist_length(e->flags)+1),
			  p_for(e->flags, p_flags_value, t),
			  p_internal_varname(t, p_str("type")),
			  p_primtype(t),
			  p_internal_varname(t, p_str("type"))));
}
	   
void output_flags(PRoot* out, Def* d){
	FlagsDef* f = (FlagsDef*) d;
	pr_add(out, "type", p_flags_decl(f));
	output_flags_type_init(out, f);
}

