#include "output.h"
#include "marshall.h"

PNode* p_object_member(Member* m){
	DataMember* d;
	if(m->membertype != MEMBER_DATA)
		return p_nil;
	d=(DataMember*)m;
	if(d->kind!=DATA_DIRECT)
		return p_nil;
	return p_fmt("\t~ ~;\n",
		     p_type(&d->type),
		     p_c_ident(m->name));
}

PNode* p_object_body(ObjectDef* o){
	return p_fmt("struct _~ {\n"
		     "\t~ parent;\n"
		     "~"
		     "};\n",
		     p_primtype(DEF(o)->type),
		     p_primtype(o->parent),
		     p_for(o->members, p_object_member, p_nil));
}

PNode* p_object_decl(ObjectDef* o){
	PrimType* n=DEF(o)->type;
	return p_fmt("#ifdef STRICT_OBJECT_TYPES\n"
		     "typedef struct _~ ~;\n"
		     "#else\n"
		     "typedef struct _GenericObject ~;\n"
		     "#endif\n",
		     p_primtype(n),
		     p_primtype(n),
		     p_primtype(n));
}

PNode* p_class_member(Member* m){
	ParamOptions o = {FALSE,FALSE,TRUE};
	if(m->membertype == MEMBER_DATA){
		DataMember* d=(DataMember*)m;
		if(d->kind != DATA_STATIC_VIRTUAL)
			return p_nil;
		else
			return p_fmt("\t~ ~;\n",
				     p_type(&d->type),
				     p_c_ident(m->name));
	}else if (m->membertype == MEMBER_METHOD){
		Method* d=(Method*)m;
		FunParams* p;
		PNode* parnode;
		
		if(d->kind==METH_STATIC ||
		   d->kind==METH_DIRECT)
			return p_nil;
		p=fparams("tp",
			  &m->my_class->self_type[d->self_const],
			  p_nil, p_nil,
			  d->params);
		parnode=p_params(p, &o);
		fparams_free(p);
		return p_fmt("\t~ (*~) (~);\n",
			     p_type(&d->ret_type),
			     p_c_ident(m->name),
			     parnode);
	}else
		return p_nil;
}

PNode* p_class_name(PrimType* o){
	return p_fmt("~Class",
		     p_primtype(o));
}

PNode* p_impl_name(ObjectDef* o){
	return p_fmt("~Impl",
		     p_primtype(DEF(o)->type));
}

PNode* p_class_body(ObjectDef* o){
	return p_fmt("struct _~ {\n"
		     "\t~ parent_class;\n"
		     "~"
		     "};\n",
		     p_class_name(DEF(o)->type),
		     p_class_name(o->parent),
		     p_for(o->members, p_class_member, p_nil));
}

PNode* p_class_decl(ObjectDef* o){
	return p_fmt("typedef struct _~ ~;\n",
		     p_class_name(DEF(o)->type),
		     p_class_name(DEF(o)->type));
}

PNode* p_abstract_member(PrimType* t, PNode* name){
	return p_fmt("~(GTK_OBJECT(self)->klass)->~",
		     p_macro_name(t, NULL, "CLASS"),
		     name);
}

PNode* p_real_varname(PrimType* t, PNode* name){
	return p_fmt("~_real",
		     p_varname(t, name));
}	

PNode* p_signal_id(Method* s){
	PrimType* t=DEF(MEMBER(s)->my_class)->type;
	return p_fmt("_~_~_signal_~",
		     p_c_ident(t->module->name),
		     p_c_ident(t->name),
		     p_c_ident(MEMBER(s)->name));
}

void output_method(PRoot* out, Method* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	PNode* name=p_c_ident(MEMBER(m)->name);
	MethodKind k=m->kind;
	ParamOptions o={TRUE, TRUE, FALSE};
	FunParams* par;
	PNode* dispatch;
	
	if(k == METH_STATIC)
		par = fparams("p", m->params);
	else
		par = fparams("tp",
			       &MEMBER(m)->my_class->self_type[m->self_const],
			       p_str("self"),
			       p_nil,
			       m->params);
	switch(k){
	case METH_EMIT_PRE:
	case METH_EMIT_POST:
	case METH_EMIT_BOTH:
		output_var(out, NULL,
			   p_str("GtkSignal"),
			   p_signal_id(m));
		dispatch=p_fmt("~(~, ~)",
			       p_signal_marshaller_name(m),
			       p_signal_id(m),
			       p_params(par, &o));
		break;
	case METH_STATIC:
	case METH_DIRECT:
		dispatch=p_fmt("~(~)",
			       p_real_varname(t, name),
			       p_params(par, &o));
		break;
	case METH_VIRTUAL:
		dispatch=p_fmt("~(~)",
			       p_abstract_member(t, name),
			       p_params(par, &o));
		break;
	}
	output_func
	(out,
	 m->prot==METH_PUBLIC?"functions":"protected",
	 &m->ret_type,
	 p_varname(t, name),
	 p_nil,
	 par, 
	 p_fmt("\t~~;\n",
	       m->ret_type.prim ? p_str("return ") : p_nil,
	       dispatch));
	 fparams_free(par);
}

void output_data_member(PRoot* out, DataMember* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	PNode* name = p_c_ident(MEMBER(m)->name);
	
	switch(m->prot){
		FunParams* par;
	case DATA_READWRITE: {
		par=fparams("tt", &MEMBER(m)->my_class->self_type[FALSE],
			    p_str("self"),
			    p_nil,
			    &m->type,
			    name,
			    p_nil);
		output_func(out,
			    "functions",
			    NULL,
			    p_varname(t, p_fmt("set_~", name)),
			    p_nil,
			    par,
			    p_fmt("\tself->~ = ~;\n",
				  name,
				  name));
		fparams_free(par);
	}
	/* fall through */
	case DATA_READONLY:
		par=fparams("t", &MEMBER(m)->my_class->self_type[TRUE],
			    p_str("self"),
			    p_nil);
		output_func(out,
			    "functions",
			    NULL,
			    p_varname(t, name),
			    p_nil,
			    par,
			    p_fmt("\treturn self->~;\n",
				  name));
		fparams_free(par);
	case DATA_PROTECTED:
	}
}

	
void output_member(PRoot* out, Member* m){
	switch(m->membertype){
	case MEMBER_METHOD:
		output_method(out, (Method*)m);
		break;
	case MEMBER_DATA:
		output_data_member(out, (DataMember*)m);
		break;
	}
}

void output_member_cb(gpointer a, gpointer b){
	output_member(b, a);
}

PNode* p_class_macros(ObjectDef* o ){
	PrimType* t=DEF(o)->type;
	return p_fmt("#define ~(o) GTK_CHECK_CAST(o, ~, ~)\n"
		     "#define ~(o) GTK_CHECK_TYPE(o, ~)\n",
		     p_macro_name(t, NULL, NULL),
		     p_macro_name(t, "TYPE", NULL),
		     p_primtype(t),
		     p_macro_name(t, "IS", NULL),
		     p_macro_name(t, "TYPE", NULL));
}

void output_object_type_init(PRoot* out, ObjectDef* o){
	PrimType* t=DEF(o)->type;
	PNode* type_var=p_internal_varname(t, p_str("type"));
	output_func(out,
		    "type",
		    type_gtk_type,
		    p_internal_varname(t, p_str("init_type")),
		    p_nil,
		    NULL,
		    p_fmt("\tstatic const GtkTypeInfo info = {\n"
			  "\t\t\"~\",\n"
			  "\t\tsizeof (~),\n"
			  "\t\tsizeof (~),\n"
			  "\t\t(GtkClassInitFunc) ~,\n"
			  "\t\t(GtkObjectInitFunc) ~,\n"
			  "\t\tNULL,\n"
			  "\t\tNULL,\n"
			  "\t\tNULL,\n"
			  "\t};\n"
			  "\tif (!~)\n"
			  "\t\t~ = gtk_type_unique (~, &info);\n"
			  "\treturn ~;\n",
			  p_primtype(t),
			  p_primtype(t),
			  p_class_name(t),
			  p_varname(t, p_str("class_init")),
			  p_varname(t, p_str("init")),
			  type_var,
			  type_var,
			  p_macro_name(o->parent, "TYPE", NULL),
			  type_var));
}

PNode* p_param_marshtype(gpointer p){
	Param* param=p;
	return p_fmt(",\n\t\tGTK_TYPE_~",
		     p_gtype_name(marshalling_type(&param->type), FALSE));
}

PNode* p_member_class_init(gpointer m, gpointer o){
	ObjectDef* ob=o;
	Member* mem=m;
	Method* meth;
	if(mem->membertype!=MEMBER_METHOD)
		return p_nil;
	meth=m;
	switch(meth->kind){
	case METH_EMIT_PRE:
	case METH_EMIT_POST:
	case METH_EMIT_BOTH:
		return p_fmt("\t~ =\n"
			     "\tgtk_signal_new(\"~\",\n"
			     "\t\tGTK_RUN_~,\n"
			     "\t\tobklass->type,\n"
			     "\t\tGTK_SIGNAL_OFFSET (~, ~),\n"
			     "\t\t~,\n"
			     "\t\tGTK_TYPE_~,\n"
			     "\t\t~"
			     "~);\n"
			     "\tgtk_object_class_add_signals(obklass,\n"
			     "\t\t&~,\n"
			     "\t\t1);\n",
			     p_signal_id(meth),
			     p_str(mem->name),
			     p_str(meth->kind==METH_EMIT_PRE
				   ?"FIRST"
				   :meth->kind==METH_EMIT_POST
				   ?"LAST"
				   :"BOTH"),
			     p_class_name(DEF(ob)->type),
			     p_c_ident(mem->name),
			     p_signal_demarshaller_name(m),
			     p_gtype_name(marshalling_type(&meth->ret_type),
					  FALSE),
			     p_prf("%d", g_slist_length(meth->params)),
			     p_for(meth->params, p_param_marshtype, p_nil),
			     p_signal_id(meth));
		break;
	default:
		return p_nil;
		break;
	}
}
			     
			     
			     

void output_class_init(PRoot* out, ObjectDef* o){
	output_func(out, NULL,
		    NULL,
		    p_varname(DEF(o)->type, p_str("class_init")),
		    p_fmt("~* klass",
			  p_class_name(DEF(o)->type)),
		    NULL,
		    p_fmt("\tGtkObjectClass* obklass = \n"
			  "\t\t(GtkObjectClass*) klass;\n"
			  "~",
			  p_for(o->members, p_member_class_init, o)));
}

void output_object(PRoot* out, ObjectDef* o){
	output_object_type_init(out, o);
	output_class_init(out, o);
	pr_add(out, "protected", p_class_decl(o));
	pr_add(out, "protected", p_class_body(o));
	pr_add(out, "type", p_object_decl(o));
	pr_add(out, "protected", p_object_body(o));
	g_slist_foreach(o->members, output_member_cb, out);
}
