/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpplugin.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__GIMP_H_INSIDE__) && !defined (GIMP_COMPILATION)
#error "Only <libgimp/gimp.h> can be included directly."
#endif

#ifndef __GIMP_PLUG_IN_H__
#define __GIMP_PLUG_IN_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define GIMP_TYPE_PLUG_IN            (gimp_plug_in_get_type ())
#define GIMP_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_PLUG_IN, GimpPlugIn))
#define GIMP_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_PLUG_IN, GimpPlugInClass))
#define GIMP_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_PLUG_IN))
#define GIMP_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_PLUG_IN))
#define GIMP_PLUG_IN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_PLUG_IN, GimpPlugInClass))


typedef struct _GimpPlugInClass   GimpPlugInClass;
typedef struct _GimpPlugInPrivate GimpPlugInPrivate;

struct _GimpPlugIn
{
  GObject            parent_instance;

  GimpPlugInPrivate *priv;
};

/**
 * GimpPlugInClass:
 * @query_procedures: This method can be overridden by all plug-ins to
 *   return a newly allocated #GList of allocated strings naming the
 *   procedures registered by this plug-in. See documentation of
 *   #GimpPlugInClass.init_procedures() for differences.
 * @init_procedures: This method can be overridden by all plug-ins to
 *   return a newly allocated #GList of allocated strings naming
 *   procedures registered by this plug-in.
 *   It is different from #GimpPlugInClass.query_procedures() in that
 *   init happens at every startup, whereas query happens only once in
 *   the life of a plug-in (right after installation or update). Hence
 *   #GimpPlugInClass.init_procedures() typically returns procedures
 *   dependent to runtime conditions (such as the presence of a
 *   third-party tool), whereas #GimpPlugInClass.query_procedures()
 *   would usually return procedures that are always available
 *   unconditionally.
 *   Most of the time, you only want to override
 *   #GimpPlugInClass.query_procedures() and leave
 *   #GimpPlugInClass.init_procedures() untouched.
 * @create_procedure: This method must be overridden by all plug-ins
 *   and return a newly allocated #GimpProcedure named @name. It will
 *   be called for every @name as returned by
 *   #GimpPlugInClass.query_procedures() and
 *   #GimpPlugInClass.init_procedures() so care must be taken to
 *   handle them all.
 *   Upon procedure registration, #GimpPlugInClass.create_procedure()
 *   will be called in the order of the lists returned by
 *   #GimpPlugInClass.query_procedures() and
 *   #GimpPlugInClass.init_procedures()
 * @quit: This method can be overridden by a plug-in which needs to
 *   perform some actions upon quitting.
 *
 * A class which every plug-in should subclass, while overriding
 * #GimpPlugInClass.query_procedures() and/or
 * #GimpPlugInClass.init_procedures(), as well as
 * #GimpPlugInClass.create_procedure().
 *
 * Since: 3.0
 **/
struct _GimpPlugInClass
{
  GObjectClass  parent_class;

  /**
   * GimpPlugInClass::query_procedures:
   * @plug_in: a #GimpPlugIn.
   *
   * Returns: (element-type gchar*) (transfer full):
   *          the names of the procedures registered by @plug_in.
   **/
  GList          * (* query_procedures) (GimpPlugIn  *plug_in);

  /**
   * GimpPlugInClass::init_procedures:
   * @plug_in: a #GimpPlugIn.
   *
   * Returns: (element-type gchar*) (transfer full):
   *          the names of the procedures registered by @plug_in.
   **/
  GList          * (* init_procedures)  (GimpPlugIn  *plug_in);

  /**
   * GimpPlugInClass::create_procedure:
   * @plug_in:        a #GimpPlugIn.
   * @procedure_name: procedure name.
   *
   * Returns: (transfer full):
   *          the procedure to be registered or executed by @plug_in.
   **/
  GimpProcedure  * (* create_procedure) (GimpPlugIn  *plug_in,
                                         const gchar *procedure_name);

  /**
   * GimpPlugInClass::quit:
   * @plug_in: a #GimpPlugIn.
   **/
  void             (* quit)             (GimpPlugIn  *plug_in);

  /* Padding for future expansion */
  /*< private >*/
  void (* _gimp_reserved1) (void);
  void (* _gimp_reserved2) (void);
  void (* _gimp_reserved3) (void);
  void (* _gimp_reserved4) (void);
  void (* _gimp_reserved5) (void);
  void (* _gimp_reserved6) (void);
  void (* _gimp_reserved7) (void);
  void (* _gimp_reserved8) (void);
};


GType           gimp_plug_in_get_type               (void) G_GNUC_CONST;

void            gimp_plug_in_set_translation_domain (GimpPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_path);
void            gimp_plug_in_set_help_domain        (GimpPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_uri);

void            gimp_plug_in_add_menu_branch        (GimpPlugIn    *plug_in,
                                                     const gchar   *menu_path,
                                                     const gchar   *menu_label);

void            gimp_plug_in_add_temp_procedure     (GimpPlugIn    *plug_in,
                                                     GimpProcedure *procedure);
void            gimp_plug_in_remove_temp_procedure  (GimpPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

GList         * gimp_plug_in_get_temp_procedures    (GimpPlugIn    *plug_in);
GimpProcedure * gimp_plug_in_get_temp_procedure     (GimpPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

void            gimp_plug_in_extension_enable       (GimpPlugIn    *plug_in);
void            gimp_plug_in_extension_process      (GimpPlugIn    *plug_in,
                                                     guint          timeout);

void            gimp_plug_in_set_pdb_error_handler  (GimpPlugIn    *plug_in,
                                                     GimpPDBErrorHandler  handler);
GimpPDBErrorHandler
                gimp_plug_in_get_pdb_error_handler  (GimpPlugIn    *plug_in);


G_END_DECLS

#endif /* __GIMP_PLUG_IN_H__ */
