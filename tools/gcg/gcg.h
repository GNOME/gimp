#ifndef __GCG_H__
#define __GCG_H__
#include <glib.h>
#include <gtk/gtktypeutils.h>

extern gboolean in_ident;

typedef const gchar* Id;

#define GET_ID(x) (g_quark_to_string(g_quark_from_string(x)))

typedef const gchar* Header;

typedef struct _Member Member;
typedef struct _TypeName TypeName;
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

struct _TypeName {
        Id module;
	Id type;
};

struct _PrimType {
	TypeName name;
	GtkFundamentalType kind;
	Id decl_header;
	Id def_header;
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
	GSList* alternatives; /* list of Id */
};

extern DefClass flags_class;

struct _FlagsDef {
	Def def;
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
	KIND_DIRECT,
	KIND_ABSTRACT,
	KIND_STATIC
} MemberKind;

typedef enum {
	VIS_PUBLIC,
	VIS_PROTECTED,
	VIS_PRIVATE
} Visibility;

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
	MEMBER_METHOD,
	MEMBER_SIGNAL
} MemberType;


struct _Member{
	MemberType membertype;
	MemberKind kind;
	ObjectDef* my_class;
	Id name;
	GString* doc;
};

#define MEMBER(x) ((Member*)(x))

struct _DataMember {
	Member member;
	DataProtection prot;
	Type type;
};

struct _Method {
	Member member;
	Visibility prot;
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



void put_decl(PrimType* t);
void put_def(Def* d);
PrimType* get_decl(Id module, Id type);
Def* get_def(Id module, Id type);

extern Type* type_gtk_type;
extern Id current_header;
extern Id current_module;
extern ObjectDef* current_class;
extern GSList* imports;
extern Method* current_method;

extern GHashTable* def_hash;
extern GHashTable* decl_hash;



#endif
