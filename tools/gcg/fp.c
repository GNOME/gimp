#include "output.h"


FunParams* fparams(const gchar* fmt, ...){
	FunParams* first;
	FunParams** last=&first;
	gint i;
	va_list args;

	va_start(args, fmt);
	for(i=0;fmt[i];i++) switch(fmt[i]){
		FunParams* f;
		GSList* l;
	case 'p':
		l=va_arg(args, GSList*);
		while(l){
			Param* p=l->data;
			f=g_new(FunParams, 1);
			f->type=p->type;
			f->name=p_c_ident(p->name);
			f->doc=p_nil;
			*last=f;
			last=&f->next;
			l=l->next;
		}
		break;
	case 't':
		f=g_new(FunParams, 1);
		f->type=*va_arg(args, Type*);
		f->name=va_arg(args, PNode*);
		p_ref(f->name);
			f->doc=va_arg(args, PNode*);
			p_ref(f->doc);
			*last=f;
			last=&f->next;
			break;
	case 's':
		f=g_new(FunParams, 1);
		f->type.prim=va_arg(args, PrimType*);
		f->type.indirection=va_arg(args, gint);
		f->type.notnull=va_arg(args, gboolean);
		f->name=va_arg(args, PNode*);
		p_ref(f->name);
		f->doc=va_arg(args, PNode*);
		p_ref(f->doc);
		*last=f;
		last=&f->next;
		break;
	}
	*last=NULL;
	return first;
}

void fparams_free(FunParams* f){
	while(f){
		FunParams* n=f->next;
		p_unref(f->name);
		p_unref(f->doc);
		g_free(f);
		f=n;
	}
}
	
