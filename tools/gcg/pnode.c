#include "pnode.h"
#include <stdio.h>
#include <stdarg.h>

static const gconstpointer p_node_magic_tag = &p_node_magic_tag;



#define BE_NODE(x) g_assert((x) && ((PNode*)(x))->magic == &p_node_magic_tag)

static GMemChunk* p_node_chunk;

static GList* p_float_list;
static GHashTable* p_str_hash;

typedef enum{
	NODE_NIL,
	NODE_DATA,
	NODE_COLLECT
} PNodeType;

typedef struct{
	gchar* str;
	PNode* node;
} PBlock;

typedef struct _PRNode{
	GQuark tag;
	PNode* node;
} PRNode;


struct _PRoot{
	GData* data;
	GList* nodes;
};

struct _PNode{
	gconstpointer magic;
	guint ref_count;
	GList* float_link;
	PNodeType type;
	union {
		struct {
			gint nblocks;
			PBlock* blocks;
		} b;
		struct {
			PNodeCreateFunc func;
			GQuark tag;
		} c;
	} u;
};

static PNode p_nil_node = {
	&p_node_magic_tag,
	-1,
	NULL,
	NODE_NIL,
	{{0,NULL}}
};

PNode* p_nil = &p_nil_node;

static PNode* p_make(PNodeType type){
	PNode* node;
	if(!p_node_chunk)
		p_node_chunk = g_mem_chunk_create(PNode, 1024,
						  G_ALLOC_AND_FREE);
	
	node = g_chunk_new(PNode, p_node_chunk);
	node->ref_count = 0;
	node->magic = &p_node_magic_tag;
	node->float_link = p_float_list = g_list_prepend(p_float_list, node);
	node->type = type;
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
	switch(node->type){
	case NODE_DATA:{
		PBlock* bl = node->u.b.blocks;
		gint i=node->u.b.nblocks;
		while(i--){
			g_free(bl[i].str);
			p_unref(bl[i].node);
		}
		g_free(bl);
		break;
	}
	case NODE_NIL:
		break;
	case NODE_COLLECT:
		break;
	}
	g_mem_chunk_free(p_node_chunk, node);
}

typedef struct{
	FILE* f;
	PRoot* p;
} PWrite;

typedef struct{
	PWrite* w;
	PNode* n;
} PCollect;

static void p_write(PNode* node, PWrite* w);


void cb_pwrite(gpointer key, gpointer value, gpointer data){
	PCollect* c = data;
	PNodeCreateFunc func = c->n->u.c.func;
	(void)key;
	if(func)
		p_write(func(value), c->w);
	else
		p_write(value, c->w);
}
	

static void p_write(PNode* node, PWrite* w){
	BE_NODE(node);
	switch(node->type){
	case NODE_DATA:{
		gint i = 0, n = node->u.b.nblocks;
		PBlock* bl = node->u.b.blocks;
		
		for(i = 0; i < n; i++){
			if(bl[i].str)
				fputs(bl[i].str, w->f);
			p_write(bl[i].node, w);
		}
		break;
	}
	case NODE_NIL:
		break;
	case NODE_COLLECT:{
		GHashTable* h = g_datalist_id_get_data(&w->p->data,
						       node->u.c.tag);
		PCollect c;
		c.w = w;
		c.n = node;
		if(!h)
			break;
		g_hash_table_foreach(h, cb_pwrite, &c);
		break;
	}
	}
}

static PNode* p_simple_string(gchar* str){
	PNode* n = p_make(NODE_DATA);
	n->u.b.nblocks = 1;
	n->u.b.blocks = g_new(PBlock, 1);
	n->u.b.blocks[0].str = str;
	n->u.b.blocks[0].node = p_nil;
	return n;
}
	
PNode* p_str(const gchar* str){
	PNode* n;

	if(!p_str_hash)
		p_str_hash = g_hash_table_new(g_str_hash, g_str_equal);
	n = g_hash_table_lookup(p_str_hash, str);
	if(n)
		return n;
	else
		return p_simple_string(g_strdup(str));
}

PNode* p_prf(const gchar* format, ...){
	PNode* n;
	va_list args;
	va_start(args, format);
	n = p_simple_string(g_strdup_vprintf (format, args));
	va_end(args);
	return n;
}

PNode* p_fmt(const gchar* f, ...){
	va_list args;
	const gchar* b;
	PNode* n = p_make(NODE_DATA);
	gint i;
	PBlock* bl;
	
	va_start(args, f);
	g_assert(f);
	for(b = f, i = 0; *b; b++)
		if(*b == '~')
			i++;
	if(b!=f && b[-1] != '~')
		i++;
	bl = g_new(PBlock, i);
	n->u.b.blocks = bl;
	n->u.b.nblocks = i;
	
	for(b = f, i = 0; *b; i++){
		gint idx=0;
		PNode* p = p_nil;
		while(b[idx] && b[idx] != '~')
			idx++;
		bl[i].str = g_strndup(b, idx);
		if(b[idx] == '~'){
			p = va_arg(args, PNode*);
			BE_NODE(p);
			p_ref(p);
			idx++;
		}
		bl[i].node = p;
		b = &b[idx];
	}
	return n;
}

PNode* p_for(GSList* lst, PNodeCreateFunc func, gpointer user_data){
	PNode* n = p_make(NODE_DATA);
	GSList* l = lst;
	gint i = g_slist_length(l);
	n->u.b.nblocks = i;
	n->u.b.blocks = g_new(PBlock, i);
	i = 0;
	while(l){
		PNode* p;
		if(user_data == p_nil)
			p = func(l->data);
		else
			p = func(l->data, user_data);
		BE_NODE(p);
		p_ref(p);
		n->u.b.blocks[i].str = NULL;
		n->u.b.blocks[i++].node = p;
		l = l->next;
	}
	return n;
}

PNode* p_col(const gchar* tag, PNodeCreateFunc func){
	PNode* n = p_make(NODE_COLLECT);
	n->u.c.func = func;
	n->u.c.tag = g_quark_from_string(tag);
	return n;
}

PRoot* pr_new(void){
	PRoot* pr = g_new(PRoot, 1);
	pr->nodes = NULL;
	g_datalist_init(&pr->data);
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

void pr_put(PRoot* pr, const gchar* tag, gpointer datum){
	GHashTable* h = g_datalist_get_data(&pr->data, tag);
	if(!h){
		h = g_hash_table_new(NULL, NULL);
		g_datalist_set_data(&pr->data, tag, h);
	}

	g_hash_table_insert(h, datum, datum);
}

void pr_write(PRoot* pr, FILE* stream, const gchar* tag){
	GList* l;
	GQuark q;
	PWrite w;
	
	g_assert(pr);

	w.p = pr;
	w.f = stream;
	q = g_quark_from_string(tag);
	for(l=g_list_last(pr->nodes);l;l=l->prev){
		PRNode* node = l->data;
		if(node->tag == q)
			p_write(node->node, &w);
	}
}

gchar* pr_to_str(PRoot* pr, const gchar* tag){
	FILE* f = tmpfile();
	glong len;
	gchar* buf;
	pr_write(pr, f, tag);
	len = ftell(f);
	rewind(f);
	buf = g_new(gchar, len+1);
	fread(buf, len, 1, f);
	buf[len]='\0';
	fclose(f);
	return buf;
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
		
	
