#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include "gcg.h"
#include <stdio.h>
#include "pnode.h"

typedef gconstpointer PBool;
extern const PBool ptrue;
extern const PBool pfalse;


typedef struct _FunParams FunParams;
struct _FunParams{
	FunParams* next;
	Type type;
	PNode* name;
	PNode* doc;
};

FunParams* fparams(const gchar* fmt, ...);
void fparams_free(FunParams* f);


typedef struct{
	gboolean first : 1;
	gboolean names : 1;
	gboolean types : 1;
	/* gboolean docs : 1; */
} ParamOptions;

PNode* p_prim_varname(PrimType* t);
PNode* p_cast(PNode* type, PNode* expr);
PNode* p_params(FunParams* args, ParamOptions* opt);
PNode* p_primtype(PrimType* t);
PNode* p_type(Type* t);
PNode* p_varname(PrimType* t, PNode* name);
PNode* p_internal_varname(PrimType* t, PNode* name);
PNode* p_object_member(Member* m);
PNode* p_object_body(ObjectDef* c);
PNode* p_object_decl(ObjectDef* c);
PNode* p_class_member(Member* m);
PNode* p_class_body(ObjectDef* c);
PNode* p_class_decl(ObjectDef* c);
PNode* p_prototype(Type* rettype, PNode* name,
		   PNode* args1,
		   FunParams* args2);



PNode* p_macro_name(PrimType* t, Id mid, Id post);
PNode* p_class_macros(ObjectDef* c );
PNode* p_get_type(ObjectDef* c);
PNode* p_gtype(Type* t);
PNode* p_guard_name(const gchar* c);
PNode* p_guard_start(const gchar *c);
PNode* p_guard_end(const gchar *c);

PNode* p_c_ident(Id id);
PNode* p_c_macro(Id id);
PNode* p_prot_header(Module* m);
PNode* p_type_header(Module* m);


void output_func(PRoot* out,
		 Id tag,
		 Type* rettype,
		 PNode* name,
		 PNode* args1,
		 FunParams* args2,
		 PNode* body);


void output_var(PRoot* out, Id tag, PNode* type, PNode* name);
void output_def(PRoot* out, Def* d);
void output_object(PRoot* out, Def* d);
void output_enum(PRoot* out, Def* d);
void output_flags(PRoot* out, Def* d);
PNode* p_type_include(Module* m);



#endif
