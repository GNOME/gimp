#ifndef __GCG_H__
#define __GCG_H__
#include <glib.h>

extern gboolean in_ident;

typedef const gchar* Id;

#define GET_ID(x) (g_quark_to_string(g_quark_from_string(x)))

typedef const gchar* Header;

typedef struct _Member Member;
typedef struct _PrimType PrimType;
typedef struct _Type Type;
typedef struct _ObjectDef ObjectDef;
typedef struct _Def Def;
typedef struct _EnumDef EnumDef;
typedef struct _FlagsDef FlagsDef;
typedef struct _DataMember DataMember;
typedef struct _Method Method;
typedef struct _MemberClass MemberClass;
typedef struct _DefClass DefClass;
typedef struct _Param Param;
typedef struct _Module Module;
typedef struct _Package Package;

typedef enum{
	TYPE_INVALID,
	TYPE_NONE,
	TYPE_INT,
	TYPE_LONG,
	TYPE_DOUBLE,
	TYPE_ENUM,
	TYPE_FLAGS,
	TYPE_FOREIGN,
	TYPE_OBJECT,
	TYPE_BOXED,
	TYPE_CHAR
} TypeKind;

struct _Package {
	Id name;
	Id headerbase;
	GHashTable* type_hash;
	GHashTable* mod_hash;
};

struct _Module {
	Package* package;
	Id name;
	Id header;
};


struct _PrimType {
	Module* module;
	Id name;
	TypeKind kind;
};

struct _Type {
	gboolean is_const; /* concerning the _ultimate dereferenced_ object */
	gint indirection; /* num of pointers to type */
	gboolean notnull; /* concerning the _immediate pointer_ */
	PrimType* prim;
};

struct _DefClass {
	void (*output)(Def*);
};

struct _Def {
	PrimType *type;
	GString* doc;
};

#define DEF(x) ((Def*)(x))

extern DefClass enum_class;

struct _EnumDef {
	Def def;
	PrimType* parent;
	GSList* alternatives; /* list of Id */
};

extern DefClass flags_class;

struct _FlagsDef {
	Def def;
	PrimType* parent; /* flags to extend */
	GSList* flags; /* list of Id */
};

extern DefClass object_class;

struct _ObjectDef {
	Def def;
	PrimType* parent;
	Type self_type[2]; /* both non-const and const */
	GSList* members; 
};

typedef enum {
	METH_PUBLIC,
	METH_PROTECTED,
	VIS_PRIVATE
} MethProtection;

typedef enum {
	DATA_READWRITE,
	DATA_READONLY,
	DATA_PROTECTED
} DataProtection;

typedef enum _EmitDef{
	EMIT_NONE,
	EMIT_PRE,
	EMIT_POST
} EmitDef;

typedef enum _MemberType{
	MEMBER_DATA,
	MEMBER_METHOD
} MemberType;

typedef enum _DataMemberKind{
	DATA_STATIC,
	DATA_DIRECT,
	DATA_STATIC_VIRTUAL
} DataMemberKind;

typedef enum _MethodKind{
	METH_STATIC,
	METH_DIRECT,
	METH_VIRTUAL,
	METH_EMIT_PRE,
	METH_EMIT_POST,
	METH_EMIT_BOTH
} MethodKind;

struct _Member{
	MemberType membertype;
	ObjectDef* my_class;
	Id name;
	GString* doc;
};

#define MEMBER(x) ((Member*)(x))

struct _DataMember {
	Member member;
	DataMemberKind kind;
	DataProtection prot;
	Type type;
};

struct _Method {
	Member member;
	MethodKind kind;
	MethProtection prot;
	GSList* params; /* list of Param* */
	gboolean self_const;
	Type ret_type;
};

struct _Param {
	Id name;
	Method* method;
	GString* doc;
	Type type;
};

typedef void (*DefFunc)(Def* def, gpointer user_data);


void init_db(void);

PrimType* get_type(Package* pkg, Id type);
void put_type(PrimType* t);
Package* get_pkg(Id pkgname);
void put_pkg(Package* pkg);
Module* get_mod(Package* pkg, Id module);
void put_mod(Module* m);
void put_def(Def* d);
void foreach_def(DefFunc f, gpointer user_data);

#endif
