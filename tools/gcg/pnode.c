#include "pnode.h"
#include <stdio.h>
#include <stdarg.h>

const gconstpointer p_no_data = &p_no_data;
static const gconstpointer p_node_magic_tag = &p_node_magic_tag;


#define BE_NODE(x) (g_assert((x) && ((PNode*)(x))->magic == &p_node_magic_tag))

static GMemChunk* p_node_chunk;

static GList* p_float_list;
static GHashTable* p_str_hash;

struct _PNode{
	gconstpointer magic;
	guint ref_count;
	GList* float_link;
	const gchar* str;
	GSList* children;
};

static PNode p_nil_node = {
	&p_node_magic_tag,
	-1,
	NULL,
	NULL,
	NULL
};

PNode* p_nil = &p_nil_node;

static PNode* p_make(void){
	PNode* node;
	if(!p_node_chunk)
		p_node_chunk = g_mem_chunk_create(PNode, 1024,
						  G_ALLOC_AND_FREE);
	
	node = g_chunk_new(PNode, p_node_chunk);
	node->ref_count = 0;
	node->magic = &p_node_magic_tag;
	node->float_link = p_float_list = g_list_prepend(p_float_list, node);
	node->str = NULL;
	node->children = NULL;
	return node;
}

void p_ref(PNode* node){
	BE_NODE(node);
	if(node==p_nil)
		return;
	if(!node->ref_count){
		p_float_list = g_list_remove_link(p_float_list,
						node->float_link);
		node->float_link = NULL;
	}
	node->ref_count++;
}

void p_unref(PNode* node){
	BE_NODE(node);
	if(node==p_nil)
		return;
	node->ref_count--;
	if(node->ref_count>0)
		return;
	else{
		GSList* l;
		if(node->children){
			for(l = node->children;l;l = l->next)
				p_unref(l->data);
			g_slist_free(node->children);
		} else 
			g_hash_table_remove(p_str_hash, node->str);
		g_free((gpointer)node->str);
		g_mem_chunk_free(p_node_chunk, node);
	}
}

void p_write(PNode* node, FILE* f){
	GSList* l;
	BE_NODE(node);
	if(node==p_nil)
		return;
	l = node->children;
	if(l){
		const gchar* c = node->str;
		if(c)
			while(*c){
				gint i = 0;
				while(c[i] && c[i] != '~')
					i++;
				fwrite(c, i, 1, f);
				c += i;
				if(*c == '~'){
					c++;
					g_assert(l);
					p_write(l->data, f);
					l = l->next;
				}
			}
		else
			do{
				p_write(l->data, f);
			}while((l=l->next));
	}else if(node->str)
		fputs(node->str, f);
}

void p_traverse(PNode* node, PNodeTraverseFunc func, gpointer user_data){
	GSList* l;
	BE_NODE(node);
	func(node, user_data);
	for(l=node->children;l;l=l->next)
		p_traverse(l->data, func, user_data);
}

PNode* p_str(const gchar* str){
	PNode* n;

	if(!p_str_hash)
		p_str_hash = g_hash_table_new(g_str_hash, g_str_equal);
	n = g_hash_table_lookup(p_str_hash, str);
	if(!n){
		n = p_make();
		n->str = g_strdup(str);
		g_hash_table_insert(p_str_hash, n->str, n);
	}
	return n;
}

PNode* p_prf(const gchar* format, ...){
	PNode* n = p_make();
	va_list args;
	va_start(args, format);
	n->str = g_strdup_vprintf (format, args);
	return n;
}

PNode* p_fmt(const gchar* f, ...){
	va_list args;
	gchar* b = f;
	PNode* n;
	GSList* l = NULL;
	va_start(args, f);
	g_assert(f);
	while(*f){
		while(*f && *f != '~')
			f++;
		if(*f == '~'){
			PNode* p;
			p = va_arg(args, PNode*);
			BE_NODE(p);
			if(p!=p_nil)
				p_ref(p);
			l = g_slist_prepend(l, p);
			
			f++;
		}
	}
	if(!l)
		return p_str(f);
	n = p_make();
	n->str = g_strdup(b);
	n->children = g_slist_reverse(l);
	return n;
}

PNode* p_lst(PNode* n, ...){
	va_list args;
	GSList* l = NULL;
	PNode* node;
	g_assert(n);
	va_start(args, n);
	while(n){
		BE_NODE(n);
		if(n!=p_nil){
			p_ref(n);
			l = g_slist_prepend(l, n);
		}
		n = va_arg(args, PNode*);
	}
	node = p_make();
	node->children = g_slist_reverse(l);
	return node;
}

PNode* p_for(GSList* l, PNodeCreateFunc func, gpointer user_data){
	PNode* n = p_make();
	GSList* lst = NULL;
	n->str = NULL;
	while(l){
		PNode* p;
		if(user_data==p_nil)
			p=func(l->data);
		else
			p=func(l->data, user_data);
		BE_NODE(p);
		if(p!=p_nil){
			p_ref(p);
			lst=g_slist_prepend(lst, p);
		}			
		l=l->next;
	}
	n->children=g_slist_reverse(lst);
	return n;
}

typedef struct _PRNode{
	GQuark tag;
	PNode* node;
} PRNode;


struct _PRoot{
	GList* nodes;
};


PRoot* pr_new(void){
	PRoot* pr = g_new(PRoot, 1);
	pr->nodes = NULL;
	return pr;
}

void pr_add(PRoot* pr, const gchar* tag, PNode* node){
	PRNode* n;
	g_assert(pr);
	BE_NODE(node);
	if(node == p_nil)
		return;
	n = g_new(PRNode, 1);
	n->tag = g_quark_from_string(tag);
	n->node = node;
	pr->nodes = g_list_prepend(pr->nodes, n);
	p_ref(node);
}

void pr_write(PRoot* pr, FILE* stream, const gchar** tags, gint n){
	GList* l;
	GQuark* quarks;
	gint i;

	quarks=g_new(GQuark, n);
	for(i=0;i<n;i++)
		quarks[i]=g_quark_from_string(tags[i]);
		
	g_assert(pr);
	for(l=g_list_last(pr->nodes);l;l=l->prev){
		PRNode* node = l->data;
		for(i=0;i<n;i++)
			if(node->tag==quarks[i])
				break;
		if(i==n)
			continue;
		p_write(node->node, stream);
	}
}

void pr_free(PRoot* pr){
	GList* l;
	for(l=pr->nodes;l;l = l->next){
		PRNode* n = l->data;
		p_unref(n->node);
		g_free(n);
	}
	g_list_free(pr->nodes);
	g_free(pr);
}
		
	
