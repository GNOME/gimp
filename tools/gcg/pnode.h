#ifndef __PNODE_H__
#define __PNODE_H__
#include <glib.h>
#include <stdio.h>

typedef struct _PNode PNode;
typedef struct _PRoot PRoot;
typedef const gconstpointer Tag;
extern PNode* p_nil;

typedef void (*PNodeTraverseFunc) (PNode* n, gpointer user_data);
typedef PNode* (*PNodeCreateFunc) ();

void p_ref(PNode* node);
void p_unref(PNode* node);
PNode* p_str(const gchar* str);
PNode* p_prf(const gchar* format, ...) G_GNUC_PRINTF(1, 2);
PNode* p_fmt(const gchar* f, ...);
PNode* p_lst(PNode* n, ...);
PNode* p_for(GSList* l, PNodeCreateFunc func, gpointer user_data);
PNode* p_col(const gchar* tag, PNodeCreateFunc func);
void p_write(PNode* node, FILE* f, PRoot* r);
gchar* p_to_str(PNode* n, PRoot* pr);



PRoot* pr_new(void);
void pr_put(PRoot* pr, const gchar* tag, gpointer datum);
void pr_write(PRoot* pr, FILE* stream, const gchar* tag);
void pr_free(PRoot* root);



#endif
