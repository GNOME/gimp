#include "output.h"

void pr_object_member(File* s, Member* m){
	DataMember* d;
	if(m->membertype != MEMBER_DATA
	   || m->kind != KIND_DIRECT)
		return;
	d=(DataMember*)m;
	pr(s, "\t%1 %1;\n",
	   pr_type, &d->type,
	   pr_low, m->name);
}

void pr_object_body(File* s, ObjectDef* o){
	GSList* l;
	pr(s, "struct _%1 {\n"
	   "\t%1 parent;\n"
	   "%3"
	   "};\n",
	   pr_primtype, DEF(o)->type,
	   pr_primtype, o->parent,
	   pr_list_foreach, o->members, pr_object_member, no_data);
}

void pr_object_decl(File* s, ObjectDef* o){
	PrimType* n=DEF(o)->type;
	pr(s,
	   "#ifdef STRICT_OBJECT_TYPES\n"
	   "typedef struct _%1 %1;\n"
	   "#else\n"
	   "typedef struct _GenericObject %1;\n"
	   "#endif\n",
	   pr_primtype, n,
	   pr_primtype, n,
	   pr_primtype, n);
}

void pr_class_member(File* s, Member* m){
	ParamOptions o=PARAMS_TYPES;
	if(m->kind!=KIND_ABSTRACT)
		return;
	if(m->membertype==MEMBER_DATA){
		DataMember* d=(DataMember*)m;
		pr(s, "\t%1 %s;\n",
		   pr_type, &d->type,
		   m->name);
	}else if (m->membertype==MEMBER_METHOD){
		Method* d=(Method*)m;
		pr(s, "\t%1 (*%s) (%1%2);\n",
		   pr_type, &d->ret_type,
		   m->name,
		   pr_type, &m->my_class->self_type[d->self_const],
		   pr_params, d->params, &o);
	}
}




void pr_class_name(File* s, PrimType* o){
	pr(s, "%1Class",
	   pr_primtype, o);
}

void pr_impl_name(File* s, ObjectDef* o){
	pr(s, "%1Impl",
	   pr_type, DEF(o)->type);
}


void pr_class_body(File* s, ObjectDef* o){
	pr(s,
	   "struct _%1 {\n"
	   "\t%1 parent_class;\n"
	   "%3"
	   "};\n",
	   pr_class_name, DEF(o)->type,
	   pr_class_name, o->parent,
	   pr_list_foreach, o->members, pr_class_member, no_data);
}

void pr_class_decl(File* s, ObjectDef* o){
	pr(s,
	   "typedef struct _%1 %1;\n",
	   pr_class_name, DEF(o)->type,
	   pr_class_name, DEF(o)->type);
}

void output_abstract_method(Method* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	Id name=MEMBER(m)->name;
	ParamOptions o=PARAMS_NAMES;
	output_func
	(t,
	 name,
	 m->params,
	 &m->ret_type,
	 m->prot==METH_PUBLIC?func_hdr:prot_hdr,
	 MEMBER(m)->my_class,
	 m->self_const,
	 FALSE,
	 "\t%?s%3(GTK_OBJECT(self)->klass)->%s(self%2);\n",
	 !!m->ret_type.prim, "return ",
	 pr_macro_name, t, NULL, "CLASS",
	 name,
	 pr_params, m->params, &o);
}

void output_data_member(DataMember* m){
	Id name=MEMBER(m)->name;
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	switch(m->prot){
	case DATA_READWRITE: {
		Id set_name=g_strconcat("set_", name, NULL);
		Param p;
		GSList l;
		p.type=m->type;
		p.name="value";
		l.data=&p;
		l.next=NULL;
		output_func(t,
			    set_name,
			    &l,
			    NULL,
			    func_hdr,
			    MEMBER(m)->my_class,
			    FALSE,
			    FALSE,
			    "\tself->%s=value;\n",
			    name);
		g_free(set_name);
	}
	/* fall through */
	case DATA_READONLY:
		output_func(t,
			    name,
			    NULL,
			    &m->type,
			    func_hdr,
			    MEMBER(m)->my_class,
			    TRUE,
			    FALSE,
			    "\treturn self->%s;\n",
			    name);
	case DATA_PROTECTED:
	}
}

void output_member(Member* m, gpointer dummy){
	switch(m->membertype){
	case MEMBER_METHOD:
		if(m->kind==KIND_ABSTRACT)
			output_abstract_method((Method*)m);
		break;
	case MEMBER_DATA:
		if(m->kind==KIND_DIRECT)
			output_data_member((DataMember*)m);
		break;
	}
}
			    
void pr_class_macros(File* s, ObjectDef* o ){
	PrimType* t=DEF(o)->type;
	pr(s, "#define %3(o) GTK_CHECK_CAST(o, %3, %1)\n",
	   pr_macro_name, t, NULL, NULL,
	   pr_macro_name, t, "TYPE", NULL,
	   pr_primtype, t);
	pr(s, "#define %3(o) GTK_CHECK_TYPE(o, %3)\n",
	   pr_macro_name, t, "IS", NULL,
	   pr_macro_name, t, "TYPE", NULL);
}

void output_object_type_init(ObjectDef* o){
	PrimType* t=DEF(o)->type;
	output_func(t, "init_type", NULL, type_gtk_type, type_hdr, NULL,
		    FALSE, TRUE,
		    "\tstatic GtkTypeInfo info = {\n"
		    "\t\t\"%1\",\n"
		    "\t\tsizeof (%1),\n"
		    "\t\tsizeof (%1),\n"
		    "\t\t(GtkClassInitFunc) %2,\n"
		    "\t\t(GtkObjectInitFunc) %2,\n"
		    "\t\tNULL,\n"
		    "\t\tNULL,\n"
		    "\t\tNULL,\n"
		    "\t};\n"
		    "\t%2 = gtk_type_unique (%3, &info);\n"
		    "\treturn %2;\n",
		    pr_primtype, t,
		    pr_primtype, t,
		    pr_class_name, DEF(o)->type,
		    pr_varname, t, "class_init",
		    pr_varname, t, "init",
		    pr_internal_varname, t, "type",
		    pr_macro_name, o->parent, "TYPE", NULL,
		    pr_internal_varname, t, "type");
}


void output_object(ObjectDef* o){
	output_def(DEF(o));
	output_object_type_init(o);
	pr_class_body(prot_hdr, o);
	pr_class_decl(prot_hdr, o);
	pr_object_decl(type_hdr, o);
	pr_object_body(prot_hdr, o);
	g_slist_foreach(o->members, output_member, NULL);
}



DefClass object_class={
	output_object
	};

