#include "gcg.h"
#include "output.h"

typedef struct {
	Id package;
	GtkType rettype;
	GSList* argtypes;
} SignalType;

PNode* p_gtype_name(GtkType t, gboolean abbr){
	static const struct GTypeName{
		GtkType type;
		Id name;
		Id aname;
	}names[]={
		{GTK_TYPE_POINTER, "POINTER", "P"},
		{GTK_TYPE_INT, "INT", "I"},
		{GTK_TYPE_DOUBLE, "DOUBLE", "D"},
		{GTK_TYPE_LONG, "LONG", "L"},
		{GTK_TYPE_CHAR, "CHAR", "C"},
		{GTK_TYPE_NONE, "NONE", "0"},
	};
	gint i;
	
	for(i=0;i<sizeof(names)/sizeof(names[0]);i++)
		if(names[i].type==t)
			if(abbr)
				return p_str(names[i].aname);
			else
				return p_str(names[i].name);
	g_assert_not_reached();
	return NULL;
}

PNode* p_gtabbr(gpointer p){
	GtkType* t=p;
	return p_gtype_name(*t, TRUE);
}

GtkType marshalling_type(Type* t){
	g_assert(t);

	if(!t->prim)
		return GTK_TYPE_NONE;
	
	g_assert(t->prim->kind!=GTK_TYPE_INVALID);

	if(t->indirection)
		return GTK_TYPE_POINTER;
	switch(t->prim->kind){
	case GTK_TYPE_UINT:
	case GTK_TYPE_BOOL:
	case GTK_TYPE_FLAGS:
	case GTK_TYPE_ENUM:
		return GTK_TYPE_INT;
	case GTK_TYPE_STRING:
		return GTK_TYPE_POINTER;
	case GTK_TYPE_UCHAR:
		return GTK_TYPE_CHAR;
	case GTK_TYPE_ULONG:
		return GTK_TYPE_LONG;
	default:
		return t->prim->kind;
	}
}


PNode* p_signal_func_name(Method* s, PNode* basename){
	SignalType t;
	GSList* p=s->params, *a=NULL;
	PNode* ret;
	t.package = DEF(MEMBER(s)->my_class)->type->module->name;
	t.rettype = marshalling_type(&s->ret_type);
	while(p){
		Param* param=p->data;
		GtkType* t=g_new(GtkType, 1);
		*t=marshalling_type(&param->type);
		a=g_slist_prepend(a, t);
		p=p->next;
	}
	a=g_slist_reverse(a);
	t.argtypes=a;
	ret=p_fmt("_~_~_~_~",
		  p_c_ident(t.package),
		  basename,
		  p_gtype_name(t.rettype, TRUE),
		  p_for(t.argtypes, p_gtabbr, p_nil));
	while(a){
		g_free(a->data);
		a=a->next;
	}
	g_slist_free(a);
	return ret;
}

PNode* p_signal_marshaller_name(Method* m){
	return p_signal_func_name(m, p_str("marshall"));
}

PNode* p_signal_demarshaller_name(Method* m){
	return p_signal_func_name(m, p_str("demarshall"));
}

PNode* p_handler_type(SignalType* t){
	return p_fmt("_~Handler_~_~",
		     p_str(t->package),
		     p_gtype_name(t->rettype, TRUE),
		     p_for(t->argtypes, p_gtabbr, p_nil));
}


typedef struct{
	PNode* args;
	gint idx;
}ArgMarshData;

		
PNode* p_arg_marsh(gpointer p, gpointer d){
	GtkType* t=p;
	ArgMarshData* data=d;
	return p_fmt("\t\tGTK_VALUE_~(~[~]),\n",
		     p_gtype_name(*t, FALSE),
		     data->args,
		     p_fmt("%d", data->idx++));
}

PNode* p_demarshaller(SignalType* t){
	gint idx=0;

	
	return p_lst((t->rettype==GTK_TYPE_NONE)
		     ? p_fmt("\t*(GTK_RETLOC_~(args[~])) =\n",
			     p_gtype_name(t->rettype, FALSE),
			     p_prf("%d", g_slist_length(t->argtypes)))
		     : p_nil,
		     p_fmt("\t~(object,\n"
			   "~"
			   "\tuser_data);\n",
			   p_cast(p_handler_type(t), p_str("func")),
			   p_for(t->argtypes, p_arg_marsh, &idx)),
		     p_nil);
}


