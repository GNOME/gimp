#ifndef __MARSHALL_H__
#define __MARSHALL_H__

#include "gcg.h"
#include "pnode.h"

typedef struct _SignalType SignalType;
SignalType* sig_type(Method* m);
void sig_type_free(SignalType* t);
PNode* p_gtabbr(gpointer p);
PNode* p_signal_marshaller_name(SignalType* s);
PNode* p_signal_demarshaller_name(SignalType* s);
PNode* p_arg_marsh(gpointer p, gpointer d);
PNode* p_gtktype(Type* t);
PNode* p_signal_id(Method* s);
PNode* p_sig_marshalling(Method* m);


#endif
