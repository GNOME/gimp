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
	FlagData d={
		-1,
		DEF(e)->type
	};
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

void output_flags_type_init(OutCtx* ctx, FlagsDef* e){
	PrimType* t=DEF(e)->type;
	output_func(ctx, t, "init_type", NULL, type_gtk_type, VIS_PUBLIC,
		    NULL, FALSE, TRUE,
		    p_fmt("\tstatic GtkFlagValue values[~] = {\n"
			  "~"
			  "\t\t{0, NULL, NULL}\n"
			  "\t};\n"
			  "\t~ = gtk_type_register_flags (\"~\", values);\n"
			  "\treturn ~;\n",
			  p_prf("%d", g_slist_length(e->flags)+1),
			  p_for(e->flags, p_flags_value, t),
			  p_internal_varname(t, "type"),
			  p_primtype(t),
			  p_internal_varname(t, "type")));
}
	   
void output_flags(OutCtx* ctx, FlagsDef* e){
	pr_add(ctx->type_hdr, p_flags_decl(e));
	output_flags_type_init(ctx, e);
}

