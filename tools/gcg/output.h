#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "gcg.h"
#include <stdio.h>
#include "pnode.h"

typedef gconstpointer PBool;
extern const PBool ptrue;
extern const PBool pfalse;

typedef struct{
	PRoot* type_hdr;
	PRoot* func_hdr;
	PRoot* prot_hdr;
	PRoot* src;
	PRoot* pvt_hdr;
}OutCtx;

typedef struct{
	gboolean first : 1;
	gboolean names : 1;
	gboolean types : 1;
	/* gboolean docs : 1; */
} ParamOptions;

PNode* p_params(GSList* args, ParamOptions* opt);
PNode* p_primtype(PrimType* t);
PNode* p_type(Type* t);
PNode* p_self_type(ObjectDef* c, PBool const_self);
PNode* p_varname(PrimType* t, Id name);
PNode* p_internal_varname(PrimType* t, Id name);
PNode* p_object_member(Member* m);
PNode* p_object_body(ObjectDef* c);
PNode* p_object_decl(ObjectDef* c);
PNode* p_class_member(Member* m);
PNode* p_class_body(ObjectDef* c);
PNode* p_class_decl(ObjectDef* c);
PNode* p_prototype(PrimType* type, Id funcname,
		  GSList* params, Type* rettype, gboolean internal);


PNode* p_macro_name(PrimType* t, Id mid, Id post);
PNode* p_class_macros(ObjectDef* c );
PNode* p_get_type(ObjectDef* c);
PNode* p_gtype(Type* t);
PNode* p_guard_name(const gchar* c);
PNode* p_guard_start(const gchar *c);
PNode* p_guard_end(const gchar *c);

PNode* p_c_ident(Id id);
PNode* p_c_macro(Id id);

  
void output_func(OutCtx* out, PrimType* type, Id funcname, GSList* params,
		 Type* rettype, Visibility scope, ObjectDef* self,
		 gboolean self_const, gboolean internal, PNode* body);

void output_var(OutCtx* out, Def* d, Type* t, Id varname, Visibility scope,
		gboolean internal);

void output_def(OutCtx* out, Def* d);
void output_object(OutCtx* out, ObjectDef* d);
void output_enum(OutCtx* out, EnumDef* d);
void output_flags(OutCtx* out, FlagsDef* d);

#endif
