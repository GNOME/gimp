#include <glib.h>
#include <glib-object.h>
#include "gimpmarshal.h"


/* BOOLEAN:OBJECT (./gimpmarshal.list:25) */
void
gimp_cclosure_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__OBJECT) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     data2);
  register GMarshalFunc_BOOLEAN__OBJECT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_object (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOLEAN:POINTER (./gimpmarshal.list:26) */
void
gimp_cclosure_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__POINTER) (gpointer     data1,
                                                     gpointer     arg_1,
                                                     gpointer     data2);
  register GMarshalFunc_BOOLEAN__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* BOOLEAN:VOID (./gimpmarshal.list:27) */
void
gimp_cclosure_marshal_BOOLEAN__VOID (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  typedef gboolean (*GMarshalFunc_BOOLEAN__VOID) (gpointer     data1,
                                                  gpointer     data2);
  register GMarshalFunc_BOOLEAN__VOID callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gboolean v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_BOOLEAN__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  g_value_set_boolean (return_value, v_return);
}

/* INT:OBJECT (./gimpmarshal.list:29) */
void
gimp_cclosure_marshal_INT__OBJECT (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_INT__OBJECT) (gpointer     data1,
                                            gpointer     arg_1,
                                            gpointer     data2);
  register GMarshalFunc_INT__OBJECT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__OBJECT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_object (param_values + 1),
                       data2);

  g_value_set_int (return_value, v_return);
}

/* INT:POINTER (./gimpmarshal.list:30) */
void
gimp_cclosure_marshal_INT__POINTER (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data)
{
  typedef gint (*GMarshalFunc_INT__POINTER) (gpointer     data1,
                                             gpointer     arg_1,
                                             gpointer     data2);
  register GMarshalFunc_INT__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gint v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_INT__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       data2);

  g_value_set_int (return_value, v_return);
}

/* OBJECT:INT (./gimpmarshal.list:32) */
void
gimp_cclosure_marshal_OBJECT__INT (GClosure     *closure,
                                   GValue       *return_value,
                                   guint         n_param_values,
                                   const GValue *param_values,
                                   gpointer      invocation_hint,
                                   gpointer      marshal_data)
{
  typedef GObject* (*GMarshalFunc_OBJECT__INT) (gpointer     data1,
                                                gint         arg_1,
                                                gpointer     data2);
  register GMarshalFunc_OBJECT__INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  GObject* v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_OBJECT__INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_int (param_values + 1),
                       data2);

  g_value_set_object (return_value, v_return);
  if (NULL != (v_return))
    g_object_unref (v_return);
}

/* OBJECT:POINTER (./gimpmarshal.list:33) */
void
gimp_cclosure_marshal_OBJECT__POINTER (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data)
{
  typedef GObject* (*GMarshalFunc_OBJECT__POINTER) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     data2);
  register GMarshalFunc_OBJECT__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  GObject* v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_OBJECT__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       data2);

  g_value_set_object (return_value, v_return);
  if (NULL != (v_return))
    g_object_unref (v_return);
}

/* POINTER:INT (./gimpmarshal.list:35) */
void
gimp_cclosure_marshal_POINTER__INT (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__INT) (gpointer     data1,
                                                 gint         arg_1,
                                                 gpointer     data2);
  register GMarshalFunc_POINTER__INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_int (param_values + 1),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:INT,INT (./gimpmarshal.list:36) */
void
gimp_cclosure_marshal_POINTER__INT_INT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__INT_INT) (gpointer     data1,
                                                     gint         arg_1,
                                                     gint         arg_2,
                                                     gpointer     data2);
  register GMarshalFunc_POINTER__INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__INT_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_int (param_values + 1),
                       g_value_get_int (param_values + 2),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:OBJECT,INT (./gimpmarshal.list:37) */
void
gimp_cclosure_marshal_POINTER__OBJECT_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__OBJECT_INT) (gpointer     data1,
                                                        gpointer     arg_1,
                                                        gint         arg_2,
                                                        gpointer     data2);
  register GMarshalFunc_POINTER__OBJECT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__OBJECT_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_object (param_values + 1),
                       g_value_get_int (param_values + 2),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:POINTER (./gimpmarshal.list:38) */
void
gimp_cclosure_marshal_POINTER__POINTER (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__POINTER) (gpointer     data1,
                                                     gpointer     arg_1,
                                                     gpointer     data2);
  register GMarshalFunc_POINTER__POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__POINTER) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:POINTER,INT (./gimpmarshal.list:39) */
void
gimp_cclosure_marshal_POINTER__POINTER_INT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__POINTER_INT) (gpointer     data1,
                                                         gpointer     arg_1,
                                                         gint         arg_2,
                                                         gpointer     data2);
  register GMarshalFunc_POINTER__POINTER_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       g_value_get_int (param_values + 2),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:POINTER,INT,INT (./gimpmarshal.list:40) */
void
gimp_cclosure_marshal_POINTER__POINTER_INT_INT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__POINTER_INT_INT) (gpointer     data1,
                                                             gpointer     arg_1,
                                                             gint         arg_2,
                                                             gint         arg_3,
                                                             gpointer     data2);
  register GMarshalFunc_POINTER__POINTER_INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__POINTER_INT_INT) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       g_value_get_pointer (param_values + 1),
                       g_value_get_int (param_values + 2),
                       g_value_get_int (param_values + 3),
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* POINTER:VOID (./gimpmarshal.list:41) */
void
gimp_cclosure_marshal_POINTER__VOID (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  typedef gpointer (*GMarshalFunc_POINTER__VOID) (gpointer     data1,
                                                  gpointer     data2);
  register GMarshalFunc_POINTER__VOID callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;
  gpointer v_return;

  g_return_if_fail (return_value != NULL);
  g_return_if_fail (n_param_values == 1);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_POINTER__VOID) (marshal_data ? marshal_data : cc->callback);

  v_return = callback (data1,
                       data2);

  g_value_set_pointer (return_value, v_return);
}

/* VOID:DOUBLE (./gimpmarshal.list:43) */
void
gimp_cclosure_marshal_VOID__DOUBLE (GClosure     *closure,
                                    GValue       *return_value,
                                    guint         n_param_values,
                                    const GValue *param_values,
                                    gpointer      invocation_hint,
                                    gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__DOUBLE) (gpointer     data1,
                                             gdouble      arg_1,
                                             gpointer     data2);
  register GMarshalFunc_VOID__DOUBLE callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__DOUBLE) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_double (param_values + 1),
            data2);
}

/* VOID:INT,INT (./gimpmarshal.list:44) */
void
gimp_cclosure_marshal_VOID__INT_INT (GClosure     *closure,
                                     GValue       *return_value,
                                     guint         n_param_values,
                                     const GValue *param_values,
                                     gpointer      invocation_hint,
                                     gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT) (gpointer     data1,
                                              gint         arg_1,
                                              gint         arg_2,
                                              gpointer     data2);
  register GMarshalFunc_VOID__INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_int (param_values + 2),
            data2);
}

/* VOID:INT,INT,INT (./gimpmarshal.list:45) */
void
gimp_cclosure_marshal_VOID__INT_INT_INT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT_INT) (gpointer     data1,
                                                  gint         arg_1,
                                                  gint         arg_2,
                                                  gint         arg_3,
                                                  gpointer     data2);
  register GMarshalFunc_VOID__INT_INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_int (param_values + 2),
            g_value_get_int (param_values + 3),
            data2);
}

/* VOID:INT,INT,INT,INT (./gimpmarshal.list:46) */
void
gimp_cclosure_marshal_VOID__INT_INT_INT_INT (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_INT_INT_INT) (gpointer     data1,
                                                      gint         arg_1,
                                                      gint         arg_2,
                                                      gint         arg_3,
                                                      gint         arg_4,
                                                      gpointer     data2);
  register GMarshalFunc_VOID__INT_INT_INT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 5);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_INT_INT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_int (param_values + 2),
            g_value_get_int (param_values + 3),
            g_value_get_int (param_values + 4),
            data2);
}

/* VOID:INT,POINTER (./gimpmarshal.list:47) */
void
gimp_cclosure_marshal_VOID__INT_POINTER (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_POINTER) (gpointer     data1,
                                                  gint         arg_1,
                                                  gpointer     arg_2,
                                                  gpointer     data2);
  register GMarshalFunc_VOID__INT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_pointer (param_values + 2),
            data2);
}

/* VOID:INT,POINTER,POINTER (./gimpmarshal.list:48) */
void
gimp_cclosure_marshal_VOID__INT_POINTER_POINTER (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__INT_POINTER_POINTER) (gpointer     data1,
                                                          gint         arg_1,
                                                          gpointer     arg_2,
                                                          gpointer     arg_3,
                                                          gpointer     data2);
  register GMarshalFunc_VOID__INT_POINTER_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__INT_POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_int (param_values + 1),
            g_value_get_pointer (param_values + 2),
            g_value_get_pointer (param_values + 3),
            data2);
}

/* VOID:OBJECT,INT (./gimpmarshal.list:49) */
void
gimp_cclosure_marshal_VOID__OBJECT_INT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_INT) (gpointer     data1,
                                                 gpointer     arg_1,
                                                 gint         arg_2,
                                                 gpointer     data2);
  register GMarshalFunc_VOID__OBJECT_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_object (param_values + 1),
            g_value_get_int (param_values + 2),
            data2);
}

/* VOID:OBJECT,INT,POINTER (./gimpmarshal.list:50) */
void
gimp_cclosure_marshal_VOID__OBJECT_INT_POINTER (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_INT_POINTER) (gpointer     data1,
                                                         gpointer     arg_1,
                                                         gint         arg_2,
                                                         gpointer     arg_3,
                                                         gpointer     data2);
  register GMarshalFunc_VOID__OBJECT_INT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_INT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_object (param_values + 1),
            g_value_get_int (param_values + 2),
            g_value_get_pointer (param_values + 3),
            data2);
}

/* VOID:OBJECT,POINTER (./gimpmarshal.list:51) */
void
gimp_cclosure_marshal_VOID__OBJECT_POINTER (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_POINTER) (gpointer     data1,
                                                     gpointer     arg_1,
                                                     gpointer     arg_2,
                                                     gpointer     data2);
  register GMarshalFunc_VOID__OBJECT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_object (param_values + 1),
            g_value_get_pointer (param_values + 2),
            data2);
}

/* VOID:POINTER,INT (./gimpmarshal.list:52) */
void
gimp_cclosure_marshal_VOID__POINTER_INT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_INT) (gpointer     data1,
                                                  gpointer     arg_1,
                                                  gint         arg_2,
                                                  gpointer     data2);
  register GMarshalFunc_VOID__POINTER_INT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_INT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_pointer (param_values + 1),
            g_value_get_int (param_values + 2),
            data2);
}

/* VOID:POINTER,POINTER (./gimpmarshal.list:53) */
void
gimp_cclosure_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                             GValue       *return_value,
                                             guint         n_param_values,
                                             const GValue *param_values,
                                             gpointer      invocation_hint,
                                             gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__POINTER_POINTER) (gpointer     data1,
                                                      gpointer     arg_1,
                                                      gpointer     arg_2,
                                                      gpointer     data2);
  register GMarshalFunc_VOID__POINTER_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 3);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__POINTER_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_value_get_pointer (param_values + 1),
            g_value_get_pointer (param_values + 2),
            data2);
}

