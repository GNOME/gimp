#include <stdio.h>
#include "output.h"
#include <stdarg.h>

void pr_param(File* s, Param* p, ParamOptions* opt){
	pr(s, "%?s%?1%?s%?1",
	   !(*opt & PARAMS_FIRST), ", ",
	   !!(*opt & PARAMS_TYPES), pr_type, &p->type,
	   (*opt & PARAMS_TYPES) && (*opt & PARAMS_NAMES), " ",
	   !!(*opt & PARAMS_NAMES), pr_low, p->name);
}

void pr_params(File* s, GSList* args, ParamOptions* opt){
	ParamOptions o=*opt;
	if(args)
		if(o & PARAMS_FIRST){
			pr_param(s, args->data, &o);
			o &= ~PARAMS_FIRST;
			pr_list_foreach(s, args->next, pr_param, &o);
		}else
			pr_list_foreach(s, args, pr_param, &o);
	else
		if(o & PARAMS_FIRST)
			pr(s, "void");
}

void pr_primtype(File* s, PrimType* t){
	pr(s, "%s%s",
	   t->name.module,
	   t->name.type);
}

void pr_type(File* s, Type* t){
	if(t && t->prim){
		gint i=t->indirection;
		pr(s, "%?s%1",
		   t->is_const, "const ",
		   pr_primtype, t->prim);
		while(i--)
			pr_c(s, '*');
		if(t->indirection)
			file_add_dep(s, t->prim->decl_header);
		else
			file_add_dep(s, t->prim->def_header);
	}else
		pr(s, "void");
}	

void pr_self_type(File* s, ObjectDef* o, PBool const_self){
	pr(s, "%?s%2*",
	   !!const_self, "const ",
	   pr_primtype, DEF(o)->type);
}

void pr_varname(File* s, PrimType* t, Id name){
	pr(s, "%1_%1_%1",
	   pr_low, t->name.module,
	   pr_low, t->name.type,
	   pr_low, name);
}

void pr_internal_varname(File* s, PrimType* t, Id name){
	pr(s, "_%2",
	   pr_varname, t, name);
}


void pr_prototype(File* s, PrimType* type, Id funcname,
		  GSList* params, Type* rettype, gboolean internal){
	ParamOptions o=PARAMS_NAMES | PARAMS_TYPES | PARAMS_FIRST;
	pr(s, "%1 %2 (%2)",
	   pr_type, rettype,
	   (internal?pr_internal_varname:pr_varname), type, funcname,
	   pr_params, params, &o);
}

void pr_type_guard(File* s, Param* p){
	Type* t=&p->type;
	if(t->indirection && t->notnull)
		pr(s, "\tg_assert (%s);\n", p->name);
	if(t->indirection==1 && t->prim->kind == TYPE_CLASS)
		/* A direct object pointer is checked for its type */
		if(t->notnull)
			pr(s, "\tg_assert (%3(%s));\n",
			   pr_macro_name, t->prim, "IS", NULL,
			   p->name);
		else
			pr(s, "\tg_assert (!%s || %3(%s));\n",
			   p->name,
			   pr_macro_name, t->prim, "IS", NULL,
			   p->name);
	/* todo (low priority): range checks for enums */
}
	
void output_func(PrimType* type, Id funcname, GSList* params, Type* rettype,
		 File* hdr, ObjectDef* self, gboolean self_const,
		 gboolean internal, const gchar* body, ...){
	GSList l;
	Param p;
	va_list args;
	if(self){
		p.type.prim=DEF(self)->type;
		p.type.indirection=1;
		p.type.is_const=self_const;
		p.type.notnull=TRUE;
		p.name="self";
		l.data=&p;
		l.next=params;
		params=&l;
	}
	va_start(args, body);
	pr((hdr?hdr:source_head), "%?s%5;\n",
	   !hdr, "static ",
	   pr_prototype, type, funcname, params, rettype, internal);
	pr(source,
	   "%?s%5{\n"
	   "%3"
	   "%v"
	   "}\n\n",
	   !hdr, "static ",
	   pr_prototype, type, funcname, params, rettype, internal,
	   pr_list_foreach, params, pr_type_guard, no_data,
	   body, args);
} 

 
 
void pr_macro_name(File* s, PrimType* t, Id mid, Id post){
	pr(s,
	   "%1%?s%?1_%1%?s%?1",
	   pr_up, t->name.module,
	   !!mid, "_",
	   !!mid, pr_up, mid,
	   pr_up, t->name.type,
	   !!post, "_",
	   !!post, pr_up, post);
}

void output_var(Def* d, Type* t, Id varname, File* hdr, gboolean internal){
	if(hdr)
		pr(hdr,
		   "extern %1 %2;\n",
		   pr_type, t,
		   (internal?pr_internal_varname:pr_varname), &d->type->name,
		   varname);
	pr(source_head,
	   "%?s%1 %2;\n",
	   !hdr, "static ",
	   pr_type, t,
	   (internal?pr_internal_varname:pr_varname), &d->type->name,
	   varname);
}	

