
#ifndef __gimp_marshal_MARSHAL_H__
#define __gimp_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* BOOLEAN:OBJECT (./gimpmarshal.list:25) */
extern void gimp_marshal_BOOLEAN__OBJECT (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* BOOLEAN:POINTER (./gimpmarshal.list:26) */
extern void gimp_marshal_BOOLEAN__POINTER (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* BOOLEAN:VOID (./gimpmarshal.list:27) */
extern void gimp_marshal_BOOLEAN__VOID (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* INT:OBJECT (./gimpmarshal.list:29) */
extern void gimp_marshal_INT__OBJECT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

/* INT:POINTER (./gimpmarshal.list:30) */
extern void gimp_marshal_INT__POINTER (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);

/* OBJECT:INT (./gimpmarshal.list:32) */
extern void gimp_marshal_OBJECT__INT (GClosure     *closure,
                                      GValue       *return_value,
                                      guint         n_param_values,
                                      const GValue *param_values,
                                      gpointer      invocation_hint,
                                      gpointer      marshal_data);

/* OBJECT:POINTER (./gimpmarshal.list:33) */
extern void gimp_marshal_OBJECT__POINTER (GClosure     *closure,
                                          GValue       *return_value,
                                          guint         n_param_values,
                                          const GValue *param_values,
                                          gpointer      invocation_hint,
                                          gpointer      marshal_data);

/* POINTER:INT (./gimpmarshal.list:35) */
extern void gimp_marshal_POINTER__INT (GClosure     *closure,
                                       GValue       *return_value,
                                       guint         n_param_values,
                                       const GValue *param_values,
                                       gpointer      invocation_hint,
                                       gpointer      marshal_data);

/* POINTER:INT,INT (./gimpmarshal.list:36) */
extern void gimp_marshal_POINTER__INT_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* POINTER:OBJECT,INT (./gimpmarshal.list:37) */
extern void gimp_marshal_POINTER__OBJECT_INT (GClosure     *closure,
                                              GValue       *return_value,
                                              guint         n_param_values,
                                              const GValue *param_values,
                                              gpointer      invocation_hint,
                                              gpointer      marshal_data);

/* POINTER:POINTER (./gimpmarshal.list:38) */
extern void gimp_marshal_POINTER__POINTER (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* POINTER:POINTER,INT (./gimpmarshal.list:39) */
extern void gimp_marshal_POINTER__POINTER_INT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* POINTER:POINTER,INT,INT (./gimpmarshal.list:40) */
extern void gimp_marshal_POINTER__POINTER_INT_INT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* POINTER:VOID (./gimpmarshal.list:41) */
extern void gimp_marshal_POINTER__VOID (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* VOID:DOUBLE (./gimpmarshal.list:43) */
#define gimp_marshal_VOID__DOUBLE	g_cclosure_marshal_VOID__DOUBLE

/* VOID:ENUM (./gimpmarshal.list:44) */
#define gimp_marshal_VOID__ENUM	g_cclosure_marshal_VOID__ENUM

/* VOID:INT (./gimpmarshal.list:45) */
#define gimp_marshal_VOID__INT	g_cclosure_marshal_VOID__INT

/* VOID:INT,BOOLEAN,INT,OBJECT (./gimpmarshal.list:46) */
extern void gimp_marshal_VOID__INT_BOOLEAN_INT_OBJECT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

/* VOID:INT,INT (./gimpmarshal.list:47) */
extern void gimp_marshal_VOID__INT_INT (GClosure     *closure,
                                        GValue       *return_value,
                                        guint         n_param_values,
                                        const GValue *param_values,
                                        gpointer      invocation_hint,
                                        gpointer      marshal_data);

/* VOID:INT,INT,INT (./gimpmarshal.list:48) */
extern void gimp_marshal_VOID__INT_INT_INT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* VOID:INT,INT,INT,INT (./gimpmarshal.list:49) */
extern void gimp_marshal_VOID__INT_INT_INT_INT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:INT,OBJECT (./gimpmarshal.list:50) */
extern void gimp_marshal_VOID__INT_OBJECT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:INT,POINTER (./gimpmarshal.list:51) */
extern void gimp_marshal_VOID__INT_POINTER (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* VOID:INT,POINTER,POINTER (./gimpmarshal.list:52) */
extern void gimp_marshal_VOID__INT_POINTER_POINTER (GClosure     *closure,
                                                    GValue       *return_value,
                                                    guint         n_param_values,
                                                    const GValue *param_values,
                                                    gpointer      invocation_hint,
                                                    gpointer      marshal_data);

/* VOID:OBJECT (./gimpmarshal.list:53) */
#define gimp_marshal_VOID__OBJECT	g_cclosure_marshal_VOID__OBJECT

/* VOID:OBJECT,INT (./gimpmarshal.list:54) */
extern void gimp_marshal_VOID__OBJECT_INT (GClosure     *closure,
                                           GValue       *return_value,
                                           guint         n_param_values,
                                           const GValue *param_values,
                                           gpointer      invocation_hint,
                                           gpointer      marshal_data);

/* VOID:OBJECT,INT,POINTER (./gimpmarshal.list:55) */
extern void gimp_marshal_VOID__OBJECT_INT_POINTER (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:OBJECT,POINTER (./gimpmarshal.list:56) */
extern void gimp_marshal_VOID__OBJECT_POINTER (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:POINTER (./gimpmarshal.list:57) */
#define gimp_marshal_VOID__POINTER	g_cclosure_marshal_VOID__POINTER

/* VOID:POINTER,INT (./gimpmarshal.list:58) */
extern void gimp_marshal_VOID__POINTER_INT (GClosure     *closure,
                                            GValue       *return_value,
                                            guint         n_param_values,
                                            const GValue *param_values,
                                            gpointer      invocation_hint,
                                            gpointer      marshal_data);

/* VOID:POINTER,INT,OBJECT (./gimpmarshal.list:59) */
extern void gimp_marshal_VOID__POINTER_INT_OBJECT (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:POINTER,OBJECT (./gimpmarshal.list:60) */
extern void gimp_marshal_VOID__POINTER_OBJECT (GClosure     *closure,
                                               GValue       *return_value,
                                               guint         n_param_values,
                                               const GValue *param_values,
                                               gpointer      invocation_hint,
                                               gpointer      marshal_data);

/* VOID:POINTER,POINTER (./gimpmarshal.list:61) */
extern void gimp_marshal_VOID__POINTER_POINTER (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:POINTER,UINT,INT,OBJECT (./gimpmarshal.list:62) */
extern void gimp_marshal_VOID__POINTER_UINT_INT_OBJECT (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:UINT (./gimpmarshal.list:63) */
#define gimp_marshal_VOID__UINT	g_cclosure_marshal_VOID__UINT

/* VOID:VOID (./gimpmarshal.list:64) */
#define gimp_marshal_VOID__VOID	g_cclosure_marshal_VOID__VOID

G_END_DECLS

#endif /* __gimp_marshal_MARSHAL_H__ */

