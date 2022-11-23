/* LIBLIGMA - The LIGMA Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * ligmaplugin.h
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#if !defined (__LIGMA_H_INSIDE__) && !defined (LIGMA_COMPILATION)
#error "Only <libligma/ligma.h> can be included directly."
#endif

#ifndef __LIGMA_PLUG_IN_H__
#define __LIGMA_PLUG_IN_H__

G_BEGIN_DECLS

/* For information look into the C source or the html documentation */


#define LIGMA_PLUG_IN_ERROR           (ligma_plug_in_error_quark ())

#define LIGMA_TYPE_PLUG_IN            (ligma_plug_in_get_type ())
#define LIGMA_PLUG_IN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), LIGMA_TYPE_PLUG_IN, LigmaPlugIn))
#define LIGMA_PLUG_IN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), LIGMA_TYPE_PLUG_IN, LigmaPlugInClass))
#define LIGMA_IS_PLUG_IN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), LIGMA_TYPE_PLUG_IN))
#define LIGMA_IS_PLUG_IN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), LIGMA_TYPE_PLUG_IN))
#define LIGMA_PLUG_IN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), LIGMA_TYPE_PLUG_IN, LigmaPlugInClass))


typedef struct _LigmaPlugInClass   LigmaPlugInClass;
typedef struct _LigmaPlugInPrivate LigmaPlugInPrivate;

struct _LigmaPlugIn
{
  GObject            parent_instance;

  LigmaPlugInPrivate *priv;
};

/**
 * LigmaPlugInClass:
 *
 * A class which every plug-in should subclass, while overriding
 * [vfunc@PlugIn.query_procedures] and/or [vfunc@PlugIn.init_procedures], as
 * well as [vfunc@PlugIn.create_procedure].
 *
 * Since: 3.0
 **/
struct _LigmaPlugInClass
{
  GObjectClass  parent_class;

  /**
   * LigmaPlugInClass::query_procedures:
   * @plug_in: a #LigmaPlugIn.
   *
   * This method can be overridden by all plug-ins to return a newly allocated
   * list of allocated strings naming the procedures registered by this
   * plug-in. See documentation of [vfunc@PlugIn.init_procedures] for
   * differences.
   *
   * Returns: (element-type gchar*) (transfer full): the names of the procedures registered by @plug_in.
   */
  GList          * (* query_procedures) (LigmaPlugIn  *plug_in);

  /**
   * LigmaPlugInClass::init_procedures:
   * @plug_in: a #LigmaPlugIn.
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
  GList          * (* init_procedures)  (LigmaPlugIn  *plug_in);

  /**
   * LigmaPlugInClass::create_procedure:
   * @plug_in:        a #LigmaPlugIn.
   * @procedure_name: procedure name.
   *
   * This method must be overridden by all plug-ins and return a newly
   * allocated #LigmaProcedure named @name.
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
  LigmaProcedure  * (* create_procedure) (LigmaPlugIn  *plug_in,
                                         const gchar *procedure_name);

  /**
   * LigmaPlugInClass::quit:
   * @plug_in: a #LigmaPlugIn.
   *
   * This method can be overridden by a plug-in which needs to perform some
   * actions upon quitting.
   */
  void             (* quit)             (LigmaPlugIn  *plug_in);

  /**
   * LigmaPlugInClass::set_i18n:
   * @plug_in:        a #LigmaPlugIn.
   * @procedure_name: procedure name.
   * @gettext_domain: (out) (nullable): Gettext domain. If %NULL, it
   *                  defaults to the plug-in name as determined by the
   *                  directory the binary is called from.
   * @catalog_dir:    (out) (nullable): relative path to a subdirectory
   *                  of the plug-in folder containing the compiled
   *                  Gettext message catalogs. If %NULL, it defaults to
   *                  "locale/".
   *
   * This method can be overridden by all plug-ins to customize
   * internationalization of the plug-in.
   *
   * This method will be called before initializing, querying or running
   * @procedure_name (respectively with [vfunc@PlugIn.init_procedures],
   * [vfunc@PlugIn.query_procedures] or with the `run()` function set in
   * `ligma_image_procedure_new()`).
   *
   * By default, LIGMA plug-ins look up gettext compiled message catalogs
   * in the subdirectory `locale/` under the plug-in folder (same folder
   * as `ligma_get_progname()`) with a text domain equal to the plug-in
   * name (regardless @procedure_name). It is unneeded to override this
   * method if you follow this localization scheme.
   *
   * If you wish to disable localization or localize with another system,
   * simply set the method to %NULL, or possibly implement this method
   * to do something useful for your usage while returning %FALSE.
   *
   * If you wish to tweak the @gettext_domain or the @localedir, return
   * %TRUE and allocate appropriate @gettext_domain and/or @localedir
   * (these use the default if set %NULL).
   *
   * Note that @localedir must be a relative path, subdirectory of the
   * directory of `ligma_get_progname()`.
   * The domain names "ligma30-std-plug-ins", "ligma30-script-fu" and
   * "ligma30-python" are reserved and can only be used with a %NULL
   * @catalog_dir. These will use the translation catalogs installed for
   * core plug-ins, so you are not expected to use these for your
   * plug-ins, except if you are making a core plug-in. More domain
   * names may become reserved so we discourage using a gettext domain
   * starting with "ligma30-".
   *
   * When localizing your plug-in this way, LIGMA also binds
   * @gettext_domain to the UTF-8 encoding.
   *
   * Returns: %TRUE if this plug-in will use Gettext localization. You
   *          may return %FALSE if you wish to disable localization or
   *          set it up differently.
   *
   * Since: 3.0
   */
  gboolean         (* set_i18n)         (LigmaPlugIn   *plug_in,
                                         const gchar  *procedure_name,
                                         gchar       **gettext_domain,
                                         gchar       **catalog_dir);

  /* Padding for future expansion */
  /*< private >*/
  void (* _ligma_reserved1) (void);
  void (* _ligma_reserved2) (void);
  void (* _ligma_reserved3) (void);
  void (* _ligma_reserved4) (void);
  void (* _ligma_reserved5) (void);
  void (* _ligma_reserved6) (void);
  void (* _ligma_reserved7) (void);
  void (* _ligma_reserved8) (void);
};


GQuark          ligma_plug_in_error_quark            (void);

GType           ligma_plug_in_get_type               (void) G_GNUC_CONST;

void            ligma_plug_in_set_help_domain        (LigmaPlugIn    *plug_in,
                                                     const gchar   *domain_name,
                                                     GFile         *domain_uri);

void            ligma_plug_in_add_menu_branch        (LigmaPlugIn    *plug_in,
                                                     const gchar   *menu_path,
                                                     const gchar   *menu_label);

void            ligma_plug_in_add_temp_procedure     (LigmaPlugIn    *plug_in,
                                                     LigmaProcedure *procedure);
void            ligma_plug_in_remove_temp_procedure  (LigmaPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

GList         * ligma_plug_in_get_temp_procedures    (LigmaPlugIn    *plug_in);
LigmaProcedure * ligma_plug_in_get_temp_procedure     (LigmaPlugIn    *plug_in,
                                                     const gchar   *procedure_name);

void            ligma_plug_in_extension_enable       (LigmaPlugIn    *plug_in);
void            ligma_plug_in_extension_process      (LigmaPlugIn    *plug_in,
                                                     guint          timeout);

void            ligma_plug_in_set_pdb_error_handler  (LigmaPlugIn    *plug_in,
                                                     LigmaPDBErrorHandler  handler);
LigmaPDBErrorHandler
                ligma_plug_in_get_pdb_error_handler  (LigmaPlugIn    *plug_in);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (LigmaPlugIn, g_object_unref);

G_END_DECLS

#endif /* __LIGMA_PLUG_IN_H__ */
