#ifndef __MARSHALL_H__
#define __MARSHALL_H__

#include "gcg.h"
#include "pnode.h"
PNode* p_gtype_name(GtkType t, gboolean abbr);
PNode* p_gtabbr(gpointer p);
GtkType marshalling_type(Type* t);
PNode* p_signal_marshaller_name(Method* s);
PNode* p_signal_demarshaller_name(Method* s);
PNode* p_arg_marsh(gpointer p, gpointer d);

#endif
