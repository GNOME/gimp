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
#if 0
	if(m->kind!=KIND_ABSTRACT)
		return;
	pr(s, "\t%1 %1%5;\n",
	   pr_type, m->type,
	   (m->method?pr_nil:pr), m->name
	   (m->method?pr:pr_nil), "(*%s) (%2)",
	   m->name,
	   pr_params, m->params, ptrue);
#endif
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

void pr_class_cast(File* s, ObjectDef* t, const gchar* format, ...){
	va_list args;
	va_start (args, format);
	pr(s, "%3 (%2)",
	   pr_macro_name, DEF(t)->type, NULL, "CLASS_CAST",
	   prv, format, args);
}
		   

void pr_abstract_member(File* s, Id varname, ObjectDef* klass, Id membername){
	pr(s,
	   "(%3->%s)",
	   pr_class_cast, klass, "GTK_OBJECT(%s)->klass", varname,
	   membername);
}

void output_method_funcs(Method* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	Id name=MEMBER(m)->name;
	output_func
	(t,
	 name,
	 m->params,
	 &m->ret_type,
	 m->prot==METH_PUBLIC?func_hdr:prot_hdr,
	 MEMBER(m)->my_class,
	 m->self_const,
	 FALSE,
	 "\t%?s%?5%%%%",
	 !!m->ret_type.prim, "return ",
	 MEMBER(m)->kind==KIND_ABSTRACT,
	 pr, "%2%2", pr_abstract_member, "self", t, name
	 );
}
	 

/*
void pr_accessor_def(...){
	Param p;
	GSList l;
	l.data=&p;
	mk_self_param(
	pr_func(m->klass, m->name, ,
		"\treturn self->%s;\n");
*/


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
		    pr_class_name, o,
		    pr_varname, t, "class_init",
		    pr_varname, t, "init",
		    pr_internal_varname, t, "type",
		    pr_macro_name, o->parent, "TYPE", NULL,
		    pr_internal_varname, t, "type");
}

void pr_gtype(File* s, Type* t){
	if((t->indirection==0 && t->prim->kind==TYPE_TRANSPARENT)
	   || (t->indirection==1 && t->prim->kind==TYPE_CLASS))
		pr_macro_name(s, t->prim, "TYPE", NULL);
	else if(t->indirection)
		pr(s, "GTK_TYPE_POINTER");
	else
		g_error("Illegal non-pointer type %s%s\n",
			t->prim->name.module,
			t->prim->name.type);
}

void output_object(ObjectDef* o){
	output_def(DEF(o));
	output_object_type_init(o);
	pr_class_body(prot_hdr, o);
	pr_class_decl(prot_hdr, o);
	pr_object_decl(type_hdr, o);
	pr_object_body(prot_hdr, o);
}



DefClass object_class={
	output_object
	};

