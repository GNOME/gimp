#include "output.h"
#include "marshall.h"

PNode* p_self_name(Member* o){
	return p_c_ident(DEF(o->my_class)->type->name);
}

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
	return p_fmt("typedef struct _~ ~;\n",
		     p_primtype(n),
		     p_primtype(n));
}

PNode* p_class_member(Member* m){
	ParamOptions o = {TRUE,FALSE,TRUE};
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


PNode* p_real_varname(PrimType* t, PNode* name){
	return p_fmt("~_real",
		     p_varname(t, name));
}	


PNode* p_signal_handler_type(Method* s){
	Member* m=MEMBER(s);
	return p_fmt("~Handler_~",
		     p_primtype(DEF(m->my_class)->type),
		     p_c_ident(m->name));
}


void output_connector(PRoot* out, Method* m){
	FunParams* par=fparams("t",
			       &MEMBER(m)->my_class->self_type[m->self_const],
			       p_self_name(MEMBER(m)),
			       p_nil);

	output_func(out, "functions",
		    &m->ret_type,
		    p_varname(DEF(MEMBER(m)->my_class)->type,
			      p_fmt("connect_~",
				    p_c_ident(MEMBER(m)->name))),
		    p_fmt("~ handler, gpointer user_data",
			  p_signal_handler_type(m)),
		    par,
		    p_fmt("\t~gtk_signal_connect((GtkObject*)~,\n"
			  "\t\t\"~\",\n"
			  "\t\t(GtkSignalFunc)handler,\n"
			  "\t\tuser_data);\n",
			  m->ret_type.prim?p_str("return "):p_nil,
			  p_self_name(MEMBER(m)),
			  p_str(MEMBER(m)->name)));
}
		    
PNode* p_param_marshtype(gpointer p){
	Param* param=p;
	return p_fmt(",\n\t\t~",
		     p_gtktype(&param->type));
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
			      p_self_name(MEMBER(m)),
			      p_nil,
			      m->params);
	
	switch(k){
		SignalType* sig;
	case METH_EMIT_PRE:
	case METH_EMIT_POST:
	case METH_EMIT_BOTH:
		sig=sig_type(m);
		output_var(out, NULL,
			   p_str("guint"),
			   p_signal_id(m));
		o.names=FALSE;
		o.types=TRUE;
		pr_put(out, "class_init_head",
		       p_fmt("\textern void ~ (GtkObject*, GtkSignalFunc, "
			     "gpointer, GtkArg*);\n",
			     p_signal_demarshaller_name(sig)));
		pr_put(out, "member_class_init",
		       p_fmt("\t~ =\n"
			     "\tgtk_signal_new(\"~\",\n"
			     "\t\tGTK_RUN_~,\n"
			     "\t\tobklass->type,\n"
			     "\t\tGTK_SIGNAL_OFFSET (~, ~),\n"
			     "\t\t~,\n"
			     "\t\t~,\n"
			     "\t\t~"
			     "~);\n"
			     "\tgtk_object_class_add_signals(obklass,\n"
			     "\t\t&~,\n"
			     "\t\t1);\n"
			     "\t}\n",
			     p_signal_id(m),
			     p_str(MEMBER(m)->name),
			     p_str(m->kind==METH_EMIT_PRE
				   ?"FIRST"
				   :m->kind==METH_EMIT_POST
				   ?"LAST"
				   :"BOTH"),
			     p_class_name(t),
			     name,
			     p_signal_demarshaller_name(sig),
			     p_gtktype(&m->ret_type),
			     p_prf("%d", g_slist_length(m->params)),
			     p_for(m->params, p_param_marshtype, p_nil),
			     p_signal_id(m)));
		
		pr_add(out, "functions",
		       p_fmt("typedef ~ (*~)(~, gpointer);\n",
			     p_type(&m->ret_type),
			     p_signal_handler_type(m),
			     p_params(par, &o)));
		output_connector(out, m);
		o.names=TRUE;
		o.types=FALSE;
		dispatch=p_sig_marshalling(m);
		break;
	case METH_STATIC:
	case METH_DIRECT:
		o.names=TRUE;
		o.types=TRUE;
		pr_add(out, "source_head",
		       p_fmt("static ~ ~_~_real (~);\n",
			     p_type(&m->ret_type),
			     p_c_ident(t->name),
			     name,
			     p_params(par, &o)));
		o.types=FALSE;
		dispatch=p_fmt("\t~~_~_real (~);\n",
			       m->ret_type.prim?
			       p_str("return "):
			       p_nil,
			       p_c_ident(t->name),
			       name,
			       p_params(par, &o));
		break;
	case METH_VIRTUAL:
		dispatch=p_fmt("\t~((~*)((GtkObject*) ~)->klass)->~ (~);\n",
			       m->ret_type.prim?
			       p_str("return "):
			       p_nil,
			       p_class_name(DEF(MEMBER(m)->my_class)->type),
			       p_c_ident(t->name),
			       name,
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
	 dispatch);
	
	fparams_free(par);
}

void output_data_member(PRoot* out, DataMember* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	PNode* name = p_c_ident(MEMBER(m)->name);
	PNode* self = p_self_name(MEMBER(m));

	
	switch(m->prot){
		FunParams* par;
	case DATA_READWRITE: {
		par=fparams("tt", &MEMBER(m)->my_class->self_type[FALSE],
			    self,
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
			    p_fmt("~"
				  "\t~->~ = ~;\n",
				  (m->type.prim->kind==TYPE_OBJECT
				   && m->type.indirection==1)
				  ?m->type.notnull
				  ?p_fmt("\tgtk_object_ref "
					 "((GtkObject*) ~);\n"
					 "\tgtk_object_unref "
					 "((GtkObject*) ~->~);\n",
					 name,
					 self,
					 name)
				  :p_fmt("\tif(~)\n"
					 "\t\tgtk_object_ref "
					 "((GtkObject*) ~);\n"
					 "\tif(~->~)\n"
					 "\t\tgtk_object_unref "
					 "((GtkObject*) ~->~);\n",
					 name,
					 name,
					 self,
					 name,
					 self,
					 name)
				  :p_nil,
				  self,
				  name,
				  name));
		fparams_free(par);
	}
	/* fall through */
	case DATA_READONLY:
		pr_put(out, "func_depends", m->type.prim->module);
		par=fparams("t", &MEMBER(m)->my_class->self_type[TRUE],
			    self,
			    p_nil);
		output_func(out,
			    "functions",
			    &m->type,
			    p_varname(t, name),
			    p_nil,
			    par,
			    p_fmt("\treturn ~->~;\n",
				  self,
				  name));
		fparams_free(par);
		/* fall through */
	case DATA_PROTECTED:
		pr_put(out, "prot_depends", m->type.prim->module);
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
	return p_fmt("#define ~(o) GTK_CHECK_TYPE(o, ~)\n",
		     p_macro_name(t, "IS", NULL),
		     p_macro_name(t, "TYPE", NULL));
}

void output_object_type_init(PRoot* out, ObjectDef* o){
	PrimType* t=DEF(o)->type;
	PNode* type_var=p_internal_varname(t, p_str("type"));
	output_func(out,
		    "type",
		    NULL,
		    p_internal_varname(t, p_str("init_type")),
		    p_nil,
		    NULL,
		    p_fmt("\tstatic GtkTypeInfo info = {\n"
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
			  "\t\t~ = gtk_type_unique (~, &info);\n",
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


void output_object_init(PRoot* out, ObjectDef* o){
	pr_add(out, "prot_deps",
	       p_col("prot_depends", p_type_include));
	pr_add(out, "func_deps",
	       p_col("func_depends", p_type_include));
	pr_add(out, "source_head",
	       p_fmt("static inline void ~ (~ ~);\n",
		     p_varname(DEF(o)->type, p_str("init_real")),
		     p_type(&o->self_type[FALSE]),
		     p_c_ident(DEF(o)->type->name)));
	output_func(out, NULL,
		    NULL,
		    p_varname(DEF(o)->type, p_str("init")),
		    p_fmt("~ ~",
			  p_type(&o->self_type[FALSE]),
			  p_c_ident(DEF(o)->type->name)),
		    NULL,
		    p_fmt("\t~ (~);\n",
			  p_varname(DEF(o)->type, p_str("init_real")),
			  p_c_ident(DEF(o)->type->name)));
}

void output_class_init(PRoot* out, ObjectDef* o){
	pr_add(out, "source_head",
	       p_fmt("static inline void ~ (~* klass);\n",
		     p_varname(DEF(o)->type, p_str("class_init_real")),
		     p_class_name(DEF(o)->type)));
	output_func(out, NULL,
		    NULL,
		    p_varname(DEF(o)->type, p_str("class_init")),
		    p_fmt("~* klass",
			  p_class_name(DEF(o)->type)),
		    NULL,
		    p_fmt("\tGtkObjectClass* obklass = "
			  "(GtkObjectClass*) klass;\n"
			  "~"
			  "~"
			  "\t~ (klass);\n",
			  p_col("class_init_head", NULL),
			  p_col("member_class_init", NULL),
			  p_varname(DEF(o)->type, p_str("class_init_real"))));
}

void output_object(PRoot* out, Def* d){
	ObjectDef* o = (ObjectDef*)d;
	output_object_type_init(out, o);
	output_class_init(out, o);
	output_object_init(out, o);
	pr_add(out, "protected", p_class_decl(o));
	pr_add(out, "protected", p_class_body(o));
	pr_add(out, "type", p_object_decl(o));
	pr_add(out, "type", p_class_macros(o));
	pr_add(out, "protected", p_object_body(o));
	g_slist_foreach(o->members, output_member_cb, out);
}
