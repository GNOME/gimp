#include <stdarg.h>
#include <stdio.h>
#include <glib.h>
#include <ctype.h>
#include "output.h"

const PBool ptrue = &ptrue;
const PBool pfalse = NULL;
File* type_hdr;
File* func_hdr;
File* prot_hdr;
File* pub_import_hdr;
File* prot_import_hdr;
File* source;
File* source_head; /* inlined to the source */

struct _File{
	FILE* stream;
	const gchar* filename;
	File* parent;
	GHashTable* deps;
};

void write_guard_name(FILE* s, const gchar* c){
	gboolean enc=FALSE;
	fputs("_g_", s);
	while(*c){
		if(isalnum(*c))
			fputc(*c, s);
		else
			fputc('_', s);
		c++;
	}
	fputc('_', s);
}

void write_guard_start(FILE* s, const gchar *c){
	fputs("#ifndef ", s);
	write_guard_name(s, c);
	fputs("\n#define ", s);
	write_guard_name(s, c);
	fputs("\n", s);
}

void write_guard_end(FILE* s, const gchar *c){
	fputs("#endif /* ", s);
	write_guard_name(s, c);
	fputs(" */\n", s);
}


File* file_new(const gchar* filename){
	File* f=g_new(File, 1);
	f->stream=tmpfile();
	f->filename=filename;
	f->deps=g_hash_table_new(g_str_hash, g_str_equal);
	f->parent=NULL;
	return f;
}

File* file_sub(File* parent){
	File* f=g_new(File, 1);
	f->stream=tmpfile();
	f->filename=NULL;
	f->deps=NULL;
	f->parent=parent;
	return f;
}


void file_add_dep(File* f, Id header){
	while(f->parent)
		f=f->parent;
	g_hash_table_insert(f->deps, header, NULL);
}

void write_dep(gpointer key, gpointer value, gpointer user_data){
	Id i=key;
	FILE* stream=user_data;
	fprintf(stream, "#include %s\n", i);
}	

void file_flush(File* f){
	File* root;
	File* old;
	static const gint bufsize=1024;
	FILE* real;
	guint8 buf[bufsize];
	size_t i;
	for(root=f;root->parent;root=root->parent);
	real=fopen(root->filename, "w+");
	g_assert(real);
	write_guard_start(real, root->filename); /* more scoping needed */
	g_hash_table_foreach(root->deps, write_dep, real);
	fflush(root->stream);
	old=NULL;
	while(f){
		g_free(old);
		rewind(f->stream);
		i=0;
		do{
			i=fread(buf, 1, bufsize, f->stream);
			fwrite(buf, 1, i, real);
		}while(i==bufsize);
		fclose(f->stream);
		old=f;
		f=f->parent;
	}
	write_guard_end(real, root->filename);
	fclose(real);
	g_hash_table_destroy(root->deps);
	g_free(root);
}

typedef void (*Func)();

const Func nilf = 0;

static void call_func(Func fun, File* s, gpointer* x, gint nargs){
	switch(nargs){
	case 0:
		fun(s);
		break;
	case 1:
		fun(s, x[0]);
		break;
	case 2:
		fun(s, x[0], x[1]);
		break;
	case 3:
		fun(s, x[0], x[1], x[2]);
		break;
	case 4:
		fun(s, x[0], x[1], x[2], x[3]);
		break;
	case 5:
		fun(s, x[0], x[1], x[2], x[3], x[4]);
		break;
	case 6:
		fun(s, x[0], x[1], x[2], x[3], x[4], x[5]);
		break;
	case 7:
		fun(s, x[0], x[1], x[2], x[3], x[4], x[5], x[6]);
		break;
	case 8:
		fun(s, x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7]);
		break;
	case 9:
		fun(s, x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8]);
		break;
	}
}

void prv(File* s, const gchar* f, va_list args){
	gint i=0;
	if(!s)
		return;
	while(*f){
		while(f[i] && f[i]!='%')
			i++;
		fwrite(f, i, 1, s->stream);
		f+=i;
		i=0;
		if(f[0]=='%'){
			char c=f[1];
			gboolean doit=TRUE;
			f+=2;
			if(c=='%')
				fputc('%', s->stream);
			else if(c=='?'){
				doit=va_arg(args, gboolean);
				c=*f++;
			}
			if(c=='s'){
				const gchar* str=va_arg(args, const gchar*);
				if(doit)
					fputs(str, s->stream);
			} else if(c=='d'){
				gint n;
				n=va_arg(args, gint);
				fprintf(s->stream, "%d", n);
			} else if(c=='v'){
				const gchar* nextformat;
				va_list nextargs;
				nextformat=va_arg(args, const gchar*);
				nextargs=va_arg(args, va_list);
				if(doit)
					prv(s, nextformat, nextargs);
			}else if(c>='0' && c<='9'){
				Func fun;
				gint n;
				gpointer x[8];
				fun=va_arg(args, Func);
				for(n=0;n<c-'0';n++)
					x[n]=va_arg(args, gpointer);
				if(doit)
					call_func(fun, s, x, c-'0');
			} else g_error("Bad format\n");
		}
	}
}


void pr(File* s, const gchar* format, ...){
	va_list args;
	va_start(args, format);
	prv(s, format, args);
	va_end(args);
}

void pr_f(File* s, const gchar* format, ...){
	va_list args;
	va_start(args, format);
	vfprintf(s->stream, format, args);
	va_end(args);
}

void pr_nil(File* s, ...){
}

void pr_c(File* s, gchar c){
	if(s)
		fputc(c, s->stream);
}

void pr_low(File* s, const gchar* str){
	gchar x;
	while(*str){
		x=*str++;
		pr_c(s, (x=='-')?'_':tolower(x));
	}
}

void pr_up(File* s, const gchar* str){
	gchar x;
	while(*str){
		x=*str++;
		pr_c(s, (x=='-')?'_':toupper(x));
	}
}
		

const gconstpointer no_data = &no_data;

/* This depends on a func pointer being about the same as a gpointer */

void pr_list_foreach(File* s, GSList* l, void (*f)(), gpointer arg){
	while(l){
		if(arg==no_data)
			f(s, l->data);
		else
			f(s, l->data, arg);
		l=l->next;
	}
}

