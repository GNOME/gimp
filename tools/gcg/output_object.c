#include "output.h"

PNode* p_object_member(Member* m){
	DataMember* d;
	if(m->membertype != MEMBER_DATA
	   || m->kind != KIND_DIRECT)
		return p_nil;
	d=(DataMember*)m;
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
	if(m->kind != KIND_ABSTRACT)
		return p_nil;
	if(m->membertype == MEMBER_DATA){
		DataMember* d=(DataMember*)m;
		return p_fmt("\t~ ~;\n",
			     p_type(&d->type),
			     p_c_ident(m->name));
	}else if (m->membertype == MEMBER_METHOD){
		Method* d=(Method*)m;
		return p_fmt("\t~ (*~) (~~);\n",
			     p_type(&d->ret_type),
			     p_c_ident(m->name),
			     p_type(&m->my_class->self_type[d->self_const]),
			     p_params(d->params, &o));
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

PNode* p_abstract_member(PrimType* t, Id name){
	return p_fmt("~(GTK_OBJECT(self)->klass)->~",
		     p_macro_name(t, NULL, "CLASS"),
		     p_c_ident(name));
}

PNode* p_real_varname(PrimType* t, Id name){
	return p_fmt("~_real",
		     p_varname(t, name));
}	

void output_method(OutCtx* ctx, Method* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	Id name=MEMBER(m)->name;
	MemberKind k=MEMBER(m)->kind;
	ParamOptions o={k==KIND_STATIC, TRUE, FALSE};
	output_func
	(ctx,t, name, m->params, &m->ret_type, m->prot,
	 (k == KIND_STATIC) ? NULL : MEMBER(m)->my_class,
	 m->self_const, FALSE,
	 p_fmt("\t~~(~~);\n",
	       m->ret_type.prim ? p_str("return ") : p_nil,
	       (k == KIND_ABSTRACT
		? p_abstract_member
		: p_real_varname)(t, name),
	       MEMBER(m)->kind != KIND_STATIC
	       ? p_nil
	       : p_str("self"),
	       p_params(m->params, &o)));
}

void output_data_member(OutCtx* ctx, DataMember* m){
	PrimType* t=DEF(MEMBER(m)->my_class)->type;
	PNode* name = p_c_ident(MEMBER(m)->name);
	
	switch(m->prot){
	case DATA_READWRITE: {
		gchar* set_name=g_strconcat("set_", name, NULL);
		Param p;
		GSList l;
		p.type=m->type;
		p.name=MEMBER(m)->name;
		l.data=&p;
		l.next=NULL;
		output_func(ctx, t, set_name, &l, NULL, VIS_PUBLIC,
			    MEMBER(m)->my_class, FALSE, FALSE,
			    p_fmt("\tself->~ = 1~;\n",
				  name,
				  name));
		g_free(set_name);
	}
	/* fall through */
	case DATA_READONLY:
		output_func(ctx, t, MEMBER(m)->name, NULL, &m->type, VIS_PUBLIC,
			    MEMBER(m)->my_class, TRUE, FALSE,
			    p_fmt("\treturn self->~;\n",
				  name));
	case DATA_PROTECTED:
	}
}

	
void output_member(OutCtx* ctx, Member* m){
	switch(m->membertype){
	case MEMBER_METHOD:
		output_method(ctx, (Method*)m);
		break;
	case MEMBER_DATA:
		if(m->kind==KIND_DIRECT)
			output_data_member(ctx, (DataMember*)m);
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

void output_object_type_init(OutCtx* out, ObjectDef* o){
	PrimType* t=DEF(o)->type;
	output_func(out, t, "init_type", NULL, type_gtk_type,
		    VIS_PUBLIC, NULL, FALSE, TRUE,
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
			  "\t~ = gtk_type_unique (~, &info);\n"
			  "\treturn ~;\n",
			  p_primtype(t),
			  p_primtype(t),
			  p_class_name(DEF(o)->type),
			  p_varname(t, "class_init"),
			  p_varname(t, "init"),
			  p_internal_varname(t, "type"),
			  p_macro_name(o->parent, "TYPE", NULL),
			  p_internal_varname(t, "type")));
}


void output_object(OutCtx* out, ObjectDef* o){
	output_object_type_init(out, o);
	pr_add(out->prot_hdr, p_class_body(o));
	pr_add(out->prot_hdr, p_class_decl(o));
	pr_add(out->type_hdr, p_object_decl(o));
	pr_add(out->prot_hdr, p_object_body(o));
	g_slist_foreach(o->members, output_member_cb, out);
}
/*
PNode* p_gtype(Type* t){
	if((t->indirection==0 && t->prim->kind==TYPE_TRANSPARENT)
	   || (t->indirection==1 && t->prim->kind==TYPE_CLASS))
		p_macro_name(s, t->prim, "TYPE", NULL);
	else if(t->indirection)
		return p_fmt( "GTK_TYPE_POINTER");
	else
		g_error("Illegal non-pointer type ~~\n",
			t->prim->name.module,
			t->prim->name.type);
}
*/
