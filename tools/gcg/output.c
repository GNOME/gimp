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

PNode* p_param(Param* p, ParamOptions* opt){
	return p_lst(opt->first ? p_nil : p_str(", "),
		     opt->types ? p_type(&p->type) : p_nil,
		     opt->types && opt->names ? p_str(" ") : p_nil,
		     opt->names ? p_c_ident(p->name) : p_nil,
		     NULL);
}

PNode* p_params(GSList* args, ParamOptions* opt){
	ParamOptions o=*opt;
	
	return (args
		? (o.first
		   ? p_lst(p_param(args->data, opt),
			   (o.first=FALSE,
			    p_for(args->next, p_param, &o)),
			   NULL)
		   : p_for(args, p_param, &o))
		: (o.first
		   ? p_str("void")
		   : p_nil));
}

PNode* p_primtype(PrimType* t){
	return p_lst(p_str(t->name.module),
		     p_str(t->name.type),
		     NULL);
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

PNode* p_varname(PrimType* t, Id name){
	return p_fmt("~_~_~",
		     p_c_ident(t->name.module),
		     p_c_ident(t->name.type),
		     p_c_ident(name));
}

PNode* p_internal_varname(PrimType* t, Id name){
	return p_fmt("_~",
	   p_varname(t, name));
}


PNode* p_prototype(PrimType* type, Id funcname,
		   GSList* params, Type* rettype, gboolean internal){
	ParamOptions o={TRUE, TRUE, TRUE};
	return p_fmt("~ ~ (~)",
		     p_type(rettype),
		     (internal?p_internal_varname:p_varname)(type, funcname),
		     p_params(params, &o));
}

PNode* p_type_guard(Param* p){
	Type* t=&p->type;
	
	return p_lst
	((t->indirection && t->notnull
	     ? p_fmt("\tg_assert (%s);\n", p->name)
	     : p_nil),
	 ((t->indirection==1 && t->prim->kind == GTK_TYPE_OBJECT)
	  ? (t->notnull
		? p_fmt("\tg_assert (%3(%1));\n",
			p_macro_name(t->prim, "IS", NULL),
			p_c_ident(p->name))
		: p_fmt("\tg_assert (!%1 || %3(%s));\n",
			p_c_ident(p->name),
			p_macro_name(t->prim, "IS", NULL),
			p_c_ident(p->name)))
	  : (t->indirection==0
		? ((t->prim->kind == GTK_TYPE_ENUM)
		   ? p_fmt("\tg_assert (%1 <= %3);\n",
			   p_c_ident(p->name),
			   p_macro_name(t->prim, NULL, "LAST"))
		   : ((t->prim->kind == GTK_TYPE_FLAGS)
		      ? p_fmt("\tg_assert ((%s << 1) < %3);\n",
			      p_c_ident(p->name),
			      p_macro_name(t->prim, NULL, "LAST"))
		      : p_nil))
		: p_nil)),
	 NULL);
}
	
void output_func(OutCtx* ctx,
		 PrimType* type, Id funcname, GSList* params, Type* rettype,
		 Visibility scope, ObjectDef* self, gboolean self_const,
		 gboolean internal, PNode* body){
	GSList l;
	Param p;
	PRoot* root;
	
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

	switch(scope){
	case VIS_PUBLIC:
		root = ctx->func_hdr;
		break;
	case VIS_PROTECTED:
		root = ctx->prot_hdr;
		break;
	case VIS_PRIVATE:
		root = ctx->pvt_hdr;
		break;
	}
		
	pr_add(root, p_fmt("~~;\n",
			   scope==VIS_PRIVATE
			   ? p_str("static ")
			   : p_nil,
			   p_prototype(type, funcname, params,
				       rettype, internal)));
	pr_add(ctx->src, p_fmt("~~{\n"
			       "~"
			       "~"
			       "}\n\n",
			       scope != VIS_PRIVATE
			       ? p_nil
					  : p_str("static "),
			       p_prototype(type, funcname, params,
					   rettype, internal),
			       p_for(params, p_type_guard, p_nil),
			       body));
}


PNode* p_macro_name(PrimType* t, Id mid, Id post){
	return p_fmt("~~_~~",
		     p_c_macro(t->name.module),
		     mid ? p_lst(p_str("_"), p_c_macro(mid), NULL) : p_nil,
		     p_c_macro(t->name.type),
		     post ? p_lst(p_str("_"), p_c_macro(post), NULL) : p_nil);
}

void output_var(OutCtx* ctx, Def* d, Type* t, Id varname, Visibility scope,
		gboolean internal){
	PRoot* root;
	switch(scope){
	case VIS_PUBLIC:
		root = ctx->func_hdr;
		break;
	case VIS_PROTECTED:
		root = ctx->prot_hdr;
		break;
	case VIS_PRIVATE:
		root = ctx->pvt_hdr;
		break;
	}
	
	pr_add(root, p_fmt("extern ~ ~;\n",
			    p_type(t),
			    (internal
			     ? p_internal_varname
			     : p_varname) (d->type, varname)));
	pr_add(ctx->src, p_fmt("~~ ~;\n",
			       scope == VIS_PRIVATE
			       ? p_nil
			       : p_str("static "),
			       p_type(t),
			       (internal
				? p_internal_varname
				: p_varname) (d->type, varname)));
}	

void output_def(OutCtx* ctx, Def* d){
	PrimType* t=d->type;
	/* GTK_TYPE_FOO macro */
	pr_add(ctx->type_hdr, p_str("\n\n"));
	pr_add(ctx->src, p_str("\n"));
	pr_add(ctx->prot_hdr, p_str("\n\n"));
	pr_add(ctx->pvt_hdr, p_str("\n"));
	pr_add(ctx->type_hdr, p_fmt("#define ~ \\\n"
				     " (~ ? ~ : ~())\n",
				     p_macro_name(t, "TYPE", NULL),
				     p_internal_varname(t, "type"),
				     p_internal_varname(t, "type"),
				     p_internal_varname(t, "init_type")));
	output_var(ctx, d, type_gtk_type, "type", VIS_PUBLIC, TRUE);
	switch(d->type->kind){
	case GTK_TYPE_OBJECT:
		output_object(ctx, d);
		break;
	case GTK_TYPE_ENUM:
		output_enum(ctx, d);
		break;
	case GTK_TYPE_FLAGS:
		output_flags(ctx, d);
		break;
	default:
		;
	}
	
}
	
