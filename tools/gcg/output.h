#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "gcg.h"
#include <stdio.h>
typedef gconstpointer PBool;
extern const PBool ptrue;
extern const PBool pfalse;

typedef enum{
	VAR_STATIC,
	VAR_PUBLIC,
	VAR_PROTECTED
} VarProt;

typedef struct _File File;
File* file_new(const gchar* filename);
void file_flush(File* f);
void file_add_dep(File* f, Id header);

extern File* type_hdr;
extern File* func_hdr;
extern File* prot_hdr;
extern File* pub_import_hdr;
extern File* prot_import_hdr;
extern File* source;
extern File* source_head; /* "private header", so to say */

void pr(File* s, const gchar* f, ...);
void prv(File* s, const gchar* f, va_list args);
void pr_nil(File* s, ...);
void pr_c(File* s, gchar c);
void pr_low(File* s, const gchar* str);
void pr_up(File* s, const gchar* str);

extern const gconstpointer no_data;

void pr_list_foreach(File* s, GSList* l, void (*f)(), gpointer arg);
void pr_params(File* s, GSList* args);
void pr_primtype(File* s, PrimType* t);
void pr_type(File* s, Type* t);
void pr_self_type(File* s, ObjectDef* c, PBool const_self);
void pr_varname(File* s, PrimType* t, Id name);
void pr_internal_varname(File* s, PrimType* t, Id name);
void pr_object_member(File* s, Member* m);
void pr_object_body(File* s, ObjectDef* c);
void pr_object_decl(File* s, ObjectDef* c);
void pr_class_member(File* s, Member* m);
void pr_class_body(File* s, ObjectDef* c);
void pr_class_decl(File* s, ObjectDef* c);
void pr_prototype(File* s, PrimType* type, Id funcname,
		  GSList* params, Type* rettype, gboolean internal);

void pr_func(ObjectDef* self, Id funcname, GSList* params, Type* rettype,
	     VarProt prot, gboolean with_self, const gchar* body, ...);

void pr_macro_name(File* s, PrimType* t, Id mid, Id post);
void pr_class_macros(File* s, ObjectDef* c );
void pr_get_type(File* s, ObjectDef* c);
void pr_gtype(File* s, Type* t);
void pr_guard_name(File* s, const gchar* c);
void pr_guard_start(File* s, const gchar *c);
void pr_guard_end(File* s, const gchar *c);
#endif
