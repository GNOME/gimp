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


#define GIMP_PLUG_IN_ERROR           (gimp_plug_in_error_quark ())

#define GIMP_TYPE_PLUG_IN (gimp_plug_in_get_type ())
G_DECLARE_DERIVABLE_TYPE (GimpPlugIn, gimp_plug_in, GIMP, PLUG_IN, GObject)


/**
 * GimpPlugInClass:
 *
 * A class which every plug-in should subclass, while overriding
 * [vfunc@PlugIn.query_procedures] and/or [vfunc@PlugIn.init_procedures], as
 * well as [vfunc@PlugIn.create_procedure].
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
   * This method can be overridden by all plug-ins to return a newly allocated
   * list of allocated strings naming the procedures registered by this
   * plug-in. See documentation of [vfunc@PlugIn.init_procedures] for
   * differences.
   *
   * Returns: (element-type gchar*) (transfer full): the names of the procedures registered by @plug_in.
   */
  GList          * (* query_procedures) (GimpPlugIn  *plug_in);

  /**
   * GimpPlugInClass::init_procedures:
   * @plug_in: a #GimpPlugIn.
   *
   * This method can be overridden by all plug-ins to return a newly allocated
   * list of allocated strings naming procedures registered by this plug-in.
   * It is different from [vfunc@PlugIn.query_procedures] in that init happens
   * at every startup, whereas query happens only once in the life of a plug-in
   * (right after installation or update). Hence [vfunc@PlugIn.init_procedures]
   * typically returns procedures dependent to runtime conditions (such as the
   * presence of a third-party tool), whereas [vfunc@PlugIn.query_procedures]
   * would usually return procedures that are always available unconditionally.
   *
   * Most of the time, you only want to override
   * [vfunc@PlugIn.query_procedures] and leave [vfunc@PlugIn.init_procedures]
   * untouched.
   *
   * Returns: (element-type gchar*) (transfer full): the names of the procedures registered by @plug_in.
   **/
  GList          * (* init_procedures)  (GimpPlugIn  *plug_in);

  /**
   * GimpPlugInClass::create_procedure:
   * @plug_in:        a #GimpPlugIn.
   * @procedure_name: procedure name.
   *
   * This method must be overridden by all plug-ins and return a newly
   * allocated #GimpProcedure named @name.
   *
   * This method will be called for every @name as returned by
   * [vfunc@PlugIn.query_procedures] and [vfunc@PlugIn.init_procedures] so care
   * must be taken to handle them all.  Upon procedure registration,
   * [vfunc@PlugIn.create_procedure] will be called in the order of the lists
   * returned by [vfunc@PlugIn.query_procedures] and
   * [vfunc@PlugIn.init_procedures]
   *
   * Returns: (transfer full): the procedure to be registered or executed by @plug_in.
   */
  GimpProcedure  * (* create_procedure) (GimpPlugIn  *plug_in,
                                         const gchar *procedure_name);

  /**
   * GimpPlugInClass::quit:
   * @plug_in: a #GimpPlugIn.
   *
   * This method can be overridden by a plug-in which needs to perform some
   * actions upon quitting.
   */
  void             (* quit)             (GimpPlugIn  *plug_in);

  /**
   * GimpPlugInClass::set_i18n:
   * @plug_in:        a #GimpPlugIn.
   * @procedure_name: procedure name.
   * @gettext_domain: (out) (nullable): Gettext domain. If %NULL, it
   *                  defaults to the plug-in name as determined by the
   *                  directory the binary is called from.
   * @catalog_dir:    (out) (nullable) (type utf8): relative path to a
   *                  subdirectory of the plug-in folder containing the compiled
   *                  Gettext message catalogs. If %NULL, it defaults to
   *                  "locale/".
   *
   * This method can be overridden by all plug-ins to customize
   * internationalization of the plug-in.
   *
   * This method will be called before initializing, querying or running
   * @procedure_name (respectively with [vfunc@PlugIn.init_procedures],
   * [vfunc@PlugIn.query_procedures] or with the `run()` function set in
   * `gimp_image_procedure_new()`).
   *
   * By default, GIMP plug-ins look up gettext compiled message catalogs
   * in the subdirectory `locale/` under the plug-in folder (same folder
   * as `gimp_get_progname()`) with a text domain equal to the plug-in
   * name (regardless @procedure_name). It is unneeded to override this
   * method if you follow this localization scheme.
   *
   * If you wish to disable localization or localize with another system,
   * simply set the method to %NULL, or possibly implement this method
   * to do something useful for your usage while returning %FALSE.
   *
   * If you wish to tweak the @gettext_domain or the @catalog_dir, return
   * %TRUE and allocate appropriate @gettext_domain and/or @catalog_dir
   * (these use the default if set %NULL).
   *
   * Note that @catalog_dir must be a relative path, encoded as UTF-8,
   * subdirectory of the directory of `gimp_get_progname()`.
   * The domain names "gimp30-std-plug-ins", "gimp30-script-fu" and
   * "gimp30-python" are reserved and can only be used with a %NULL
   * @catalog_dir. These will use the translation catalogs installed for
   * core plug-ins, so you are not expected to use these for your
   * plug-ins, except if you are making a core plug-in. More domain
   * names may become reserved so we discourage using a gettext domain
   * starting with "gimp30-".
   *
   * When localizing your plug-in this way, GIMP also binds
   * @gettext_domain to the UTF-8 encoding.
   *
   * Returns: %TRUE if this plug-in will use Gettext localization. You
   *          may return %FALSE if you wish to disable localization or
   *          set it up differently.
   *
   * Since: 3.0
   */
  gboolean         (* set_i18n)         (GimpPlugIn   *plug_in,
                                         const gchar  *procedure_name,
                                         gchar       **gettext_domain,
                                         gchar       **catalog_dir);

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


GQuark          gimp_plug_in_error_quark            (void);

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
