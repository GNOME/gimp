#include "gcg.h"
#include "output.h"
#include "marshall.h"


typedef enum {
	MARSHALL_INT,
	MARSHALL_DOUBLE,
	MARSHALL_POINTER,
	MARSHALL_VOID,
	MARSHALL_LONG
} MarshallType;


struct _SignalType {
	Id package;
	MarshallType rettype;
	GSList* argtypes;
};

MarshallType marshalling_type(Type* t){
	if(!t)
		return MARSHALL_VOID;
	if(t->indirection)
		return MARSHALL_POINTER;
	if(!t->prim)
		return MARSHALL_VOID;
	switch(t->prim->kind){
	case TYPE_INT:
	case TYPE_FLAGS:
	case TYPE_ENUM:
		return MARSHALL_INT;
	case TYPE_LONG:
		return MARSHALL_LONG;
	case TYPE_DOUBLE:
		return MARSHALL_DOUBLE;
	case TYPE_NONE:
		return MARSHALL_VOID;
	default:
		g_assert_not_reached();
		return MARSHALL_VOID;
	}
}


SignalType* sig_type(Method* m){
	SignalType *t=g_new(SignalType, 1);
	GSList* p=m->params, *a=NULL;
	t->package = DEF(MEMBER(m)->my_class)->type->module->package->name;
	t->rettype = marshalling_type(&m->ret_type);
	while(p){
		Param* param=p->data;
		MarshallType* t=g_new(MarshallType, 1);
		*t=marshalling_type(&param->type);
		a=g_slist_prepend(a, t);
		p=p->next;
	}
	a=g_slist_reverse(a);
	t->argtypes=a;
	return t;
}

void sig_type_free(SignalType* t){
	GSList* l=t->argtypes;
	while(l){
		g_free(l->data);
		l=l->next;
	}
	g_slist_free(t->argtypes);
	g_free(t);
}

PNode* p_gtype_name(MarshallType t, gboolean abbr){
	static const struct GTypeName{
		MarshallType type;
		Id name;
		Id aname;
	}names[]={
		{MARSHALL_POINTER, "POINTER", "P"},
		{MARSHALL_INT, "INT", "I"},
		{MARSHALL_DOUBLE, "DOUBLE", "D"},
		{MARSHALL_LONG, "LONG", "L"},
		{MARSHALL_VOID, "NONE", "0"},
	};
	gint i;
	
	for(i=0;i<(gint)(sizeof(names)/sizeof(names[0]));i++)
		if(names[i].type==t)
			return p_str(abbr
				     ?names[i].aname
				     :names[i].name);
	g_assert_not_reached();
	return NULL;
}

PNode* p_gtabbr(gpointer p){
	MarshallType* t=p;
	return p_gtype_name(*t, TRUE);
}



PNode* p_gtktype(Type* t){
	if(t->indirection==0){
		if(!t->prim)
			return p_str("GTK_TYPE_NONE");
		switch(t->prim->kind){
		case TYPE_INT:
			return p_str("GTK_TYPE_INT");
		case TYPE_DOUBLE:
			return p_str("GTK_TYPE_DOUBLE");
		case TYPE_ENUM:
		case TYPE_FLAGS:
			return p_macro_name(t->prim, "TYPE", NULL);
		case TYPE_CHAR:
			return p_str("GTK_TYPE_CHAR");
		case TYPE_FOREIGN:
			g_error("Cannot marshall foreign type %s.%s!",
				t->prim->module->package->name,
				t->prim->name);
			return NULL;
		default:
			g_error("Cannot marshall type by value: %s.%s",
				t->prim->module->package->name,
				t->prim->name);
			return NULL;
		}
	}else if(t->indirection==1
		 && t->prim
		 && (t->prim->kind==TYPE_BOXED
		     || t->prim->kind==TYPE_OBJECT))
		return p_macro_name(t->prim, "TYPE", NULL);
	else
		return p_str("GTK_TYPE_POINTER");
}
			

PNode* p_signal_func_name(SignalType* t, PNode* basename){
	return p_fmt("_~_~_~_~",
		     p_c_ident(t->package),
		     basename,
		     p_gtype_name(t->rettype, TRUE),
		     p_for(t->argtypes, p_gtabbr, p_nil));
}

PNode* p_signal_marshaller_name(SignalType* t){
	return p_signal_func_name(t, p_str("marshall"));
}

PNode* p_signal_demarshaller_name(SignalType* t){
	return p_signal_func_name(t, p_str("demarshall"));
}

PNode* p_handler_type(SignalType* t){
	return p_fmt("_~Handler_~_~",
		     p_str(t->package),
		     p_gtype_name(t->rettype, TRUE),
		     p_for(t->argtypes, p_gtabbr, p_nil));
}

PNode* p_signal_id(Method* s){
	PrimType* t=DEF(MEMBER(s)->my_class)->type;
	return p_fmt("_~_~_signal_~",
		     p_c_ident(t->module->package->name),
		     p_c_ident(t->name),
		     p_c_ident(MEMBER(s)->name));
}

typedef struct{
	PNode* args;
	gint idx;
}ArgMarshData;

PNode* p_arg_marsh(gpointer p, gpointer d){
	Param* par=p;
	gint* idx=d;
	(*idx)++;
	return p_fmt(/* "\targs[~].type=~;\n" unnecessary... */
		     "\tGTK_VALUE_~(args[~]) = ~;\n",
		     /* p_prf("%d", *idx),
			p_gtktype(&par->type), */
		     p_gtype_name(marshalling_type(&par->type), FALSE),
		     p_prf("%d", *idx),
		     p_c_ident(par->name));
}
		     
PNode* p_sig_marshalling(Method* m){
	gint idx=-1;
	gboolean ret=marshalling_type(&m->ret_type)!=MARSHALL_VOID;
	return p_fmt("\t{\n"
		     "\tGtkArg args[~];\n"
		     "~"
		     "~"
		     "~"
		     "\tgtk_signal_emitv((GtkObject*)~, ~, args);\n"
		     "~"
		     "\t}\n",
		     p_prf("%d",
			   g_slist_length(m->params)+ret),
		     ret
		     ?p_fmt("\t~ retval;\n",
			    p_type(&m->ret_type))
		     :p_nil,
		     p_for(m->params, p_arg_marsh, &idx),
		     ret
		     /* cannot use retloc here, ansi forbids casted lvalues */
		     ?p_fmt("\tGTK_VALUE_POINTER(args[~]) = &retval;\n",
			    p_prf("%d", g_slist_length(m->params)))
		     :p_nil,
		     p_c_ident(DEF(MEMBER(m)->my_class)->type->name),
		     p_signal_id(m),
		     ret
		     ?p_str("\treturn retval;\n")
		     :p_nil);
}
		     
		
PNode* p_arg_demarsh(gpointer p, gpointer d){
	MarshallType* t=p;
	ArgMarshData* data=d;
	return p_fmt("\t\tGTK_VALUE_~(~[~]),\n",
		     p_gtype_name(*t, FALSE),
		     data->args,
		     p_fmt("%d", data->idx++));
}

PNode* p_demarshaller(SignalType* t){
	gint idx=0;

	
	return p_fmt("~~",
		     (t->rettype==TYPE_NONE)
		     ? p_fmt("\t*(GTK_RETLOC_~(args[~])) =\n",
			     p_gtype_name(t->rettype, FALSE),
			     p_prf("%d", g_slist_length(t->argtypes)))
		     : p_nil,
		     p_fmt("\t~(object,\n"
			   "~"
			   "\tuser_data);\n",
			   p_cast(p_handler_type(t), p_str("func")),
			   p_for(t->argtypes, p_arg_demarsh, &idx)));
}


