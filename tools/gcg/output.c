#include <stdio.h>
#include "output.h"
#include <stdarg.h>
#include <ctype.h>

PNode* p_c_ident(Id id){
	PNode* n;
	gchar* c;
	gchar* s;

	c=s=g_strdup(id);

	while(*c){
		*c=tolower(*c);
		if(!isalnum(*c))
			*c='_';
		c++;
	}
	n=p_str(s);
	g_free(s);
	return n;
}

PNode* p_c_macro(Id id){
	PNode* n;
	gchar* c;
	gchar* s;

	c=s=g_strdup(id);
	while(*c){
		*c=toupper(*c);
		if(!isalnum(*c))
			*c='_';
		c++;
	}
	n=p_str(s);
	g_free(s);
	return n;
}

PNode* p_param(FunParams* p, ParamOptions* opt){
	return p_lst(opt->first ? p_nil : p_str(", "),
		     opt->types ? p_type(&p->type) : p_nil,
		     opt->types && opt->names ? p_str(" ") : p_nil,
		     opt->names ? p->name : p_nil,
		     NULL);
}

PNode* p_params(FunParams* args, ParamOptions* opt){
	ParamOptions o=*opt;
	PNode* n=p_nil;
	while(args){
		n=p_lst(n, p_param(args, &o), NULL);
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
	return p_lst(p_str(t->module->name),
		     p_str(t->name),
		     NULL);
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
			node=p_lst(node, p_str("*"), NULL);
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
		     p_c_ident(t->module->name),
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
	return p_lst
	((t->indirection && t->notnull
	     ? p_fmt("\tg_assert (~);\n", var)
	     : p_nil),
	 ((t->indirection==1 && p->kind == TYPE_OBJECT)
	  ? (t->notnull
		? p_fmt("\tg_assert (~(~));\n",
			p_macro_name(p, "IS", NULL),
			var)
	     : p_fmt("\tg_assert (!~ || ~(~));\n",
		     var,
		     p_macro_name(p, "IS", NULL),
		     var))
	  : (t->indirection==0
	     ? ((p->kind == TYPE_ENUM)
		? p_fmt("\tg_assert (~ <= ~);\n",
			var,
			p_macro_name(p, NULL, "LAST"))
		: ((p->kind == TYPE_FLAGS)
		   ? p_fmt("\tg_assert ((~ << 1) < ~);\n",
			   var,
			   p_macro_name(p, NULL, "LAST"))
		   : p_nil))
	     : p_nil)),
	 NULL);
}

PNode* p_type_guards(FunParams* args){
	PNode* p=p_nil;
	while(args){
		p=p_lst(p, p_type_guard(&args->type, args->name), NULL);
		args=args->next;
	}
	return p;
}

PNode* p_prototype(Type* rettype, PNode* name,
		   PNode* args1,
		   FunParams* args2){
	ParamOptions o={(!args1 || args1==p_nil), TRUE, TRUE};
	return p_fmt("~ ~ (~~)",
		     p_type(rettype),
		     name,
		     args1,
		     p_params(args2, &o));
}

void output_var_alias(PRoot* out, PrimType* t, PNode* basename){
	pr_add(out, "import_alias",
	       p_fmt("#define ~ ~_~\n",
		     basename,
		     p_c_ident(t->module->name),
		     basename));
}

void output_type_alias(PRoot* out, PrimType* t, PNode* basename){
	pr_add(out, "import_alias",
	       p_fmt("typedef ~~ ~;\n",
		     p_str(t->module->name),
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
	pr_add(out, tag ? tag : "source_head",
	       p_fmt("~~;\n",
		     tag
		     ? p_nil
		     : p_str("static "),
		     p_prototype(rettype, name, args1, args2)));
	pr_add(out, "source",
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
		     p_c_macro(t->module->name),
		     mid ? p_lst(p_str("_"), p_c_macro(mid), NULL) : p_nil,
		     p_c_macro(t->name),
		     post ? p_lst(p_str("_"), p_c_macro(post), NULL) : p_nil);
}

void output_var(PRoot* out, Id tag, PNode* type, PNode* name){
	if(tag)
		pr_add(out, tag,
		       p_fmt("extern ~ ~;\n",
			     type,
			     name));
	pr_add(out, "source_head",
	       p_fmt("~~ ~;\n",
		     tag?p_nil:p_str("static "),
		     type,
		     name));
}	

void output_def(PRoot* out, Def* d){
	PrimType* t=d->type;
	PNode* type_var=p_internal_varname(t, p_str("type"));
	
	/* GTK_TYPE_FOO macro */
	pr_add(out, "type", p_str("\n\n"));
	pr_add(out, "source", p_str("\n"));
	pr_add(out, "protected", p_str("\n\n"));
	pr_add(out, "source_head", p_str("\n"));
	pr_add(out, "type", p_fmt("#define ~ \\\n"
				  " (~ ? ~ : ~())\n",
				  p_macro_name(t, "TYPE", NULL),
				  type_var,
				  type_var,
				  p_internal_varname(t, p_str("init_type"))));
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
	
