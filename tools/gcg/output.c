#include <stdio.h>
#include "output.h"
#include <stdarg.h>
#include <ctype.h>

PNode* p_c_ident(Id id){
	PNode* n;
	gint i = 0;
	GString* s = g_string_new(NULL);
	
	for(i = 0; id[i]; i++){
		if(!isalnum(id[i]) || (i && isupper(id[i])))
			g_string_append_c(s, '_');
		if(isalnum(id[i]))
			g_string_append_c(s, tolower(id[i]));
	}
	n = p_str(s->str);
	g_string_free(s, TRUE);
	return n;
}

PNode* p_c_macro(Id id){
	PNode* n;
	gint i = 0;
	GString* s = g_string_new(NULL);
	
	for(i = 0; id[i]; i++){
		if(!isalnum(id[i]) || (i && isupper(id[i])))
			g_string_append_c(s, '_');
		if(isalnum(id[i]))
			g_string_append_c(s, toupper(id[i]));
	}
	n = p_str(s->str);
	g_string_free(s, TRUE);
	return n;
}

PNode* p_param(FunParams* p, ParamOptions* o){
	return p_fmt("~~~~~~",
		     o->first ? p_nil : p_str(","),
		     !o->first && !(o->types && o->names) ? p_str(" ") : p_nil,
		     (o->types && o->names) ? p_str("\n\t") : p_nil,
		     o->types ? p_type(&p->type) : p_nil,
		     o->types && o->names ? p_str(" ") : p_nil,
		     o->names ? p->name : p_nil);
}

PNode* p_header(Module* m, Id suffix){
	Id base = m->package->headerbase;
	Id hdr = m->header;
	Id name = m->package->name;
	
	return p_fmt("~~",
		     base
		     ? (base[0]
			? p_fmt("~/", p_str(base))
			: p_nil)
		     : ((name && name[0])
			? p_fmt("~/", p_c_ident(name))
			: p_nil),
		     hdr
		     ? p_str(hdr)
		     : p_fmt("~~",
			     p_c_ident(m->name),
			     p_str(suffix)));
}

PNode* p_prot_header(Module* m){
	return p_header(m, ".p.h");
}

PNode* p_type_header(Module* m){
	return p_header(m, ".t.h");
}

PNode* p_func_header(Module* m){
	return p_header(m, ".h");
}

PNode* p_import_header(Module* m){
	if(m->header)
		return p_nil;
	else
		return p_header(m, ".i.h");
}

PNode* p_type_include(Module* m){
	return p_fmt("#include <~>\n",
		     p_type_header(m));
}

PNode* p_prot_include(Module* m){
	return p_fmt("#include <~>\n",
		     p_prot_header(m));
}

PNode* p_func_include(Module* m){
	return p_fmt("#include <~>\n",
		     p_func_header(m));
}

PNode* p_import_include(Module* m){
	PNode* hdr = p_import_header(m);
	if(hdr == p_nil)
		return p_nil;
	else
		return p_fmt("#include <~>\n", hdr);
}


PNode* p_params(FunParams* args, ParamOptions* opt){
	ParamOptions o=*opt;
	PNode* n=p_nil;
	while(args){
		n=p_fmt("~~", n, p_param(args, &o));
		args=args->next;
		o.first=FALSE;
	}
	if(n==p_nil)
		if(opt->first)
			return p_str("void");
		else
			return p_nil;
	else
		return n;
}

PNode* p_prim_varname(PrimType* t){
	return p_c_ident(t->name);
}

PNode* p_primtype(PrimType* t){
	return p_fmt("~~",
		     p_str(t->module->package->name),
		     p_str(t->name));
}

PNode* p_cast(PNode* force_type, PNode* expression){
	return p_fmt("((~)~)",
		     force_type,
		     expression);
}

PNode* p_type(Type* t){
	if(t && t->prim){
		PNode* node;
		gint i=t->indirection;
		node=p_fmt("~~",
			   t->is_const ? p_str("const ") : p_nil,
			   p_primtype(t->prim));
		while(i--)
			node=p_fmt("~~", node, p_str("*"));
		/* file_add_dep(s, t->prim->decl_header);*/
		return node;
	}else
		return p_str("void");
}	

PNode* p_self_type(ObjectDef* o, PBool const_self){
	return p_fmt("~~*",
	      (const_self ? p_str("const ") : p_nil),
	      p_primtype(DEF(o)->type));
}

PNode* p_varname(PrimType* t, PNode* name){
	return p_fmt("~_~_~",
		     p_c_ident(t->module->package->name),
		     p_c_ident(t->name),
		     name);
}

PNode* p_internal_varname(PrimType* t, PNode* name){
	return p_fmt("_~",
	   p_varname(t, name));
}




PNode* p_type_guard(Type* t, PNode* var){
	PrimType* p=t->prim;
	/*
	if(t->notnull && (p->indirection>1 || p->kind!=GTK_TYPE_OBJECT))
		return p_fmt("\tg_assert (~);\n", var);
	else if(p->kind==GTK_TYPE_OBJECT)
		return p_fmt("\tg_assert (~
	
		kind !=
	*/
	return p_fmt("~~",
		     (t->indirection && t->notnull
		      ? p_fmt("\tg_assert (~);\n", var)
		      : p_nil),
		     ((t->indirection==1 && p->kind == TYPE_OBJECT)
		      ? (t->notnull
			 ? p_fmt("\tg_assert (~(~));\n",
				 p_macro_name(p, "is", NULL),
				 var)
			 : p_fmt("\tg_assert (!~ || ~(~));\n",
				 var,
				 p_macro_name(p, "is", NULL),
				 var))
		      : (t->indirection==0
			 ? ((p->kind == TYPE_ENUM)
			    ? p_fmt("\tg_assert (~ <= ~);\n",
				    var,
				    p_macro_name(p, NULL, "last"))
			    : ((p->kind == TYPE_FLAGS)
			       ? p_fmt("\tg_assert ((~ << 1) < ~);\n",
				       var,
				       p_macro_name(p, NULL, "last"))
			       : p_nil))
			 : p_nil)));
}

PNode* p_type_guards(FunParams* args){
	PNode* p=p_nil;
	while(args){
		p=p_fmt("~~", p, p_type_guard(&args->type, args->name));
		args=args->next;
	}
	return p;
}

PNode* p_prototype(Type* rettype, PNode* name,
		   PNode* args1,
		   FunParams* args2){
	ParamOptions o;
	o.first = !args1 || args1 == p_nil;
	o.names = TRUE;
	o.types = TRUE;
	
	return p_fmt("~ ~ (~~)",
		     p_type(rettype),
		     name,
		     args1,
		     p_params(args2, &o));
}

void output_var_alias(PRoot* out, PrimType* t, PNode* basename){
	pr_put(out, "import_alias",
	       p_fmt("#define ~ ~_~\n",
		     basename,
		     p_c_ident(t->module->package->name),
		     basename));
}

void output_type_alias(PRoot* out, PrimType* t, PNode* basename){
	pr_put(out, "import_alias",
	       p_fmt("typedef ~~ ~;\n",
		     p_str(t->module->package->name),
		     basename,
		     basename));
}
	       

void output_func(PRoot* out,
		 Id tag,
		 Type* rettype,
		 PNode* name,
		 PNode* args1,
		 FunParams* args2,
		 PNode* body){
	pr_put(out, tag ? tag : "source_head",
	       p_fmt("~~;\n",
		     tag
		     ? p_nil
		     : p_str("static "),
		     p_prototype(rettype, name, args1, args2)));
	pr_put(out, "source",
	       p_fmt("~~{\n"
		     "~"
		     "~"
		     "}\n\n",
		     tag
		     ? p_nil
		     : p_str("static "),
		     p_prototype(rettype, name, args1, args2),
		     p_type_guards(args2),
		     body));
}


PNode* p_macro_name(PrimType* t, Id mid, Id post){
	return p_fmt("~~_~~",
		     p_c_macro(t->module->package->name),
		     mid ? p_fmt("_~", p_c_macro(mid)) : p_nil,
		     p_c_macro(t->name),
		     post ? p_fmt("_~", p_c_macro(post)) : p_nil);
}

void output_var(PRoot* out, Id tag, PNode* type, PNode* name){
	if(tag)
		pr_put(out, tag,
		       p_fmt("extern ~ ~;\n",
			     type,
			     name));
	pr_put(out, "source_head",
	       p_fmt("~~ ~;\n",
		     tag?p_nil:p_str("static "),
		     type,
		     name));
}	

void output_def(PRoot* out, Def* d){
	PrimType* t=d->type;
	PNode* type_var=p_internal_varname(t, p_str("type"));
	
	/* GTK_TYPE_FOO macro */
	pr_put(out, "type", p_str("\n\n"));
	pr_put(out, "source", p_str("\n"));
	pr_put(out, "protected", p_str("\n\n"));
	pr_put(out, "source_head", p_str("\n"));
	pr_put(out, "type", p_fmt("#define ~ \\\n"
				  " (~ ? (void)0 : ~ (), ~)\n",
				  p_macro_name(t, "type", NULL),
				  type_var,
				  p_internal_varname(t, p_str("init_type")),
				  type_var));
	output_macro_import(out, t, "type", NULL);
	output_type_import(out, t->module->package, p_str(t->name));
	output_var(out, "type",
		   p_str("GtkType"),
		   type_var);
	switch(d->type->kind){
	case TYPE_OBJECT:
		output_object(out, d);
		break;
	case TYPE_ENUM:
		output_enum(out, d);
		break;
	case TYPE_FLAGS:
		output_flags(out, d);
		break;
	default:
		;
	}
	
}

void output_type_import(PRoot* out, Package* pkg, PNode* body){
#if 1
	pr_put(out, "import_alias",
	       p_fmt("#define ~ ~~\n",
		     body,
		     p_str(pkg->name),
		     body));
#else
	pr_put(out, "import_alias",
	       p_fmt("typedef ~~ ~;\n",
		     p_str(pkg->name),
		     body,
		     body));
#endif
}

void output_macro_import(PRoot* out, PrimType* t, Id mid, Id post){
	pr_put(out, "import_alias",
	       p_fmt("#define ~~~ ~\n",
		     mid ? p_fmt("~_", p_c_macro(mid)) : p_nil,
		     p_c_macro(t->name),
		     post ? p_fmt("_~", p_c_macro(post)) : p_nil,
		     p_macro_name(t, mid, post)));
}

void output_var_import(PRoot* out, PrimType* t, PNode* body){
	pr_put(out, "import_alias",
	       p_fmt("#define ~_~ ~\n",
		     p_c_ident(t->name),
		     body,
		     p_varname(t, body)));
}
	       
	

/*
void add_dep(PRoot* out, Id tag, PrimType* type){
	pr_put(out, tag, type->module);
}

PNode* p_deps(Id tag){
	return p_col(tag, p_dep, NULL);
}
*/
