/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdarg.h>
#include <sys/types.h>

#include <gobject/gvaluecollector.h>

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimpgpparams.h"
#include "gimppdb-private.h"
#include "gimpplugin-private.h"
#include "gimppdb_pdb.h"
#include "gimpplugin-private.h"
#include "gimpplugin_pdb.h"
#include "gimpprocedure-private.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


/**
 * GimpProcedure:
 *
 * Procedures are registered functions which can be run across GIMP ecosystem.
 * They can be created by plug-ins and can then run by the core application
 * when called from menus (or through other interaction depending on specific
 * procedure subclasses).
 *
 * A plug-in can also run procedures created by the core, but also the ones
 * created by other plug-ins (see [class@PDB]).
 **/

enum
{
  PROP_0,
  PROP_PLUG_IN,
  PROP_NAME,
  PROP_PROCEDURE_TYPE,
  N_PROPS
};


typedef struct _GimpProcedurePrivate
{
  GimpPlugIn       *plug_in;

  gchar            *name;
  GimpPDBProcType   proc_type;

  gchar            *image_types;

  gchar            *menu_label;
  GList            *menu_paths;

  GimpIconType      icon_type;
  gpointer          icon_data;

  gchar            *blurb;
  gchar            *help;
  gchar            *help_id;

  gchar            *authors;
  gchar            *copyright;
  gchar            *date;

  gint              sensitivity_mask;

  gint32            n_args;
  GParamSpec      **args;

  gint32            n_aux_args;
  GParamSpec      **aux_args;

  gint32            n_values;
  GParamSpec      **values;

  GimpRunFunc       run_func;
  gpointer          run_data;
  GDestroyNotify    run_data_destroy;

  gboolean          installed;

  GHashTable       *displays;
  GHashTable       *images;
  GHashTable       *items;
  GHashTable       *resources;
} GimpProcedurePrivate;


static void                  gimp_procedure_constructed        (GObject              *object);
static void                  gimp_procedure_finalize           (GObject              *object);
static void                  gimp_procedure_set_property       (GObject              *object,
                                                                guint                 property_id,
                                                                const GValue         *value,
                                                                GParamSpec           *pspec);
static void                  gimp_procedure_get_property       (GObject              *object,
                                                                guint                 property_id,
                                                                GValue               *value,
                                                                GParamSpec           *pspec);

static void                  gimp_procedure_real_install       (GimpProcedure        *procedure);
static void                  gimp_procedure_real_uninstall     (GimpProcedure        *procedure);
static GimpValueArray      * gimp_procedure_real_run           (GimpProcedure        *procedure,
                                                                const GimpValueArray *args);
static GimpProcedureConfig * gimp_procedure_real_create_config (GimpProcedure        *procedure,
                                                                GParamSpec          **args,
                                                                gint                  n_args);

static GimpProcedureConfig *
                      gimp_procedure_create_config_with_prefix (GimpProcedure        *procedure,
                                                                const gchar          *prefix,
                                                                GParamSpec          **args,
                                                                gint                  n_args);

static GimpValueArray      * gimp_procedure_new_arguments      (GimpProcedure        *procedure);
static gboolean              gimp_procedure_validate_args      (GimpProcedure        *procedure,
                                                                GParamSpec          **param_specs,
                                                                gint                  n_param_specs,
                                                                GimpValueArray       *args,
                                                                gboolean              return_vals,
                                                                GError              **error);

static void                  gimp_procedure_set_icon           (GimpProcedure        *procedure,
                                                                GimpIconType          icon_type,
                                                                gconstpointer         icon_data);


G_DEFINE_TYPE_WITH_PRIVATE (GimpProcedure, gimp_procedure, G_TYPE_OBJECT)

#define parent_class gimp_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_procedure_class_init (GimpProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_procedure_constructed;
  object_class->finalize     = gimp_procedure_finalize;
  object_class->set_property = gimp_procedure_set_property;
  object_class->get_property = gimp_procedure_get_property;

  klass->install             = gimp_procedure_real_install;
  klass->uninstall           = gimp_procedure_real_uninstall;
  klass->run                 = gimp_procedure_real_run;
  klass->create_config       = gimp_procedure_real_create_config;

  props[PROP_PLUG_IN] =
    g_param_spec_object ("plug-in",
                         "Plug-In",
                         "The GimpPlugIn of this plug-in process",
                         GIMP_TYPE_PLUG_IN,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  props[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The procedure's name",
                         NULL,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  props[PROP_PROCEDURE_TYPE] =
    g_param_spec_enum ("procedure-type",
                       "Procedure type",
                       "The procedure's type",
                       GIMP_TYPE_PDB_PROC_TYPE,
                       GIMP_PDB_PROC_TYPE_PLUGIN,
                       GIMP_PARAM_READWRITE |
                       G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_procedure_init (GimpProcedure *procedure)
{
}

static void
gimp_procedure_constructed (GObject *object)
{
  GimpProcedure        *procedure = GIMP_PROCEDURE (object);
  GimpProcedurePrivate *priv      = gimp_procedure_get_instance_private (procedure);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_PLUG_IN (priv->plug_in));
  g_assert (priv->name != NULL);
}

static void
gimp_procedure_finalize (GObject *object)
{
  GimpProcedure        *procedure = GIMP_PROCEDURE (object);
  GimpProcedurePrivate *priv      = gimp_procedure_get_instance_private (procedure);
  gint                  i;

  if (priv->run_data_destroy)
    priv->run_data_destroy (priv->run_data);

  g_clear_object  (&priv->plug_in);

  g_clear_pointer (&priv->name,        g_free);
  g_clear_pointer (&priv->image_types, g_free);
  g_clear_pointer (&priv->menu_label,  g_free);
  g_clear_pointer (&priv->blurb,       g_free);
  g_clear_pointer (&priv->help,        g_free);
  g_clear_pointer (&priv->help_id,     g_free);
  g_clear_pointer (&priv->authors,     g_free);
  g_clear_pointer (&priv->copyright,   g_free);
  g_clear_pointer (&priv->date,        g_free);

  g_list_free_full (priv->menu_paths, g_free);
  priv->menu_paths = NULL;

  gimp_procedure_set_icon (procedure, GIMP_ICON_TYPE_ICON_NAME, NULL);

  if (priv->args)
    {
      for (i = 0; i < priv->n_args; i++)
        g_param_spec_unref (priv->args[i]);

      g_clear_pointer (&priv->args, g_free);
    }

  if (priv->aux_args)
    {
      for (i = 0; i < priv->n_aux_args; i++)
        g_param_spec_unref (priv->aux_args[i]);

      g_clear_pointer (&priv->aux_args, g_free);
    }

  if (priv->values)
    {
      for (i = 0; i < priv->n_values; i++)
        g_param_spec_unref (priv->values[i]);

      g_clear_pointer (&priv->values, g_free);
    }

  _gimp_procedure_destroy_proxies (procedure);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_procedure_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  GimpProcedure        *procedure = GIMP_PROCEDURE (object);
  GimpProcedurePrivate *priv      = gimp_procedure_get_instance_private (procedure);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_set_object (&priv->plug_in, g_value_get_object (value));
      break;

    case PROP_NAME:
      g_free (priv->name);
      priv->name = g_value_dup_string (value);
      break;

    case PROP_PROCEDURE_TYPE:
      priv->proc_type = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  GimpProcedure        *procedure = GIMP_PROCEDURE (object);
  GimpProcedurePrivate *priv      = gimp_procedure_get_instance_private (procedure);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_value_set_object (value, priv->plug_in);
      break;

    case PROP_NAME:
      g_value_set_string (value, priv->name);
      break;

    case PROP_PROCEDURE_TYPE:
      g_value_set_enum (value, priv->proc_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_install_icon (GimpProcedure *procedure)
{
  GimpIconType  icon_type;
  guint8       *icon_data        = NULL;
  gsize         icon_data_length = 0;
  GBytes       *icon_bytes       = NULL;

  icon_type = gimp_procedure_get_icon_type (procedure);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      {
        icon_data = (guint8 *) gimp_procedure_get_icon_name (procedure);
        if (icon_data)
          icon_bytes = g_bytes_new_static (icon_data,
                                           strlen ((gchar *) icon_data) + 1);
      }
      break;

    case GIMP_ICON_TYPE_PIXBUF:
      {
        GdkPixbuf *pixbuf = gimp_procedure_get_icon_pixbuf (procedure);

        if (pixbuf)
          {
            if (gdk_pixbuf_save_to_buffer (pixbuf,
                                           (gchar **) &icon_data, &icon_data_length,
                                           "png", NULL, NULL))
              icon_bytes = g_bytes_new_take (icon_data, icon_data_length);
          }
      }
      break;

    case GIMP_ICON_TYPE_IMAGE_FILE:
      {
        GFile *file = gimp_procedure_get_icon_file (procedure);

        if (file)
          {
            icon_data        = (guchar *) g_file_get_uri (file);
            icon_data_length = strlen ((gchar *) icon_data) + 1;
            icon_bytes       = g_bytes_new_take (icon_data, icon_data_length);
          }
      }
      break;
    }

  if (icon_bytes)
    _gimp_pdb_set_proc_icon (gimp_procedure_get_name (procedure),
                             icon_type, icon_bytes);
  g_bytes_unref (icon_bytes);
}

static void
gimp_procedure_real_install (GimpProcedure *procedure)
{
  GimpProcedurePrivate  *priv;
  GParamSpec           **args;
  GParamSpec           **return_vals;
  gint                   n_args        = 0;
  gint                   n_return_vals = 0;
  GList                 *list;
  GimpPlugIn            *plug_in;
  GPProcInstall          proc_install;
  gint                   i;

  priv = gimp_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->installed == FALSE);

  args        = gimp_procedure_get_arguments (procedure, &n_args);
  return_vals = gimp_procedure_get_return_values (procedure, &n_return_vals);

  proc_install.name          = (gchar *) gimp_procedure_get_name (procedure);
  proc_install.type          = gimp_procedure_get_proc_type (procedure);
  proc_install.n_params      = n_args;
  proc_install.n_return_vals = n_return_vals;
  proc_install.params        = g_new0 (GPParamDef, n_args);
  proc_install.return_vals   = g_new0 (GPParamDef, n_return_vals);

  for (i = 0; i < n_args; i++)
    {
      _gimp_param_spec_to_gp_param_def (args[i],
                                        &proc_install.params[i]);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      _gimp_param_spec_to_gp_param_def (return_vals[i],
                                        &proc_install.return_vals[i]);
    }

  plug_in = gimp_procedure_get_plug_in (procedure);

  if (! gp_proc_install_write (_gimp_plug_in_get_write_channel (plug_in),
                               &proc_install, plug_in))
    gimp_quit ();

  g_free (proc_install.params);
  g_free (proc_install.return_vals);

  gimp_procedure_install_icon (procedure);

  if (priv->image_types)
    {
      _gimp_pdb_set_proc_image_types (gimp_procedure_get_name (procedure),
                                      priv->image_types);
    }

  if (priv->menu_label)
    {
      _gimp_pdb_set_proc_menu_label (gimp_procedure_get_name (procedure),
                                     priv->menu_label);
    }

  _gimp_pdb_set_proc_sensitivity_mask (gimp_procedure_get_name (procedure),
                                       priv->sensitivity_mask);

  for (list = gimp_procedure_get_menu_paths (procedure);
       list;
       list = g_list_next (list))
    {
      _gimp_pdb_add_proc_menu_path (gimp_procedure_get_name (procedure),
                                    list->data);
    }

  if (priv->blurb ||
      priv->help  ||
      priv->help_id)
    {
      _gimp_pdb_set_proc_documentation (gimp_procedure_get_name (procedure),
                                        priv->blurb,
                                        priv->help,
                                        priv->help_id);
    }

  if (priv->authors   ||
      priv->copyright ||
      priv->date)
    {
      _gimp_pdb_set_proc_attribution (gimp_procedure_get_name (procedure),
                                      priv->authors,
                                      priv->copyright,
                                      priv->date);
    }

  priv->installed = TRUE;
}

static void
gimp_procedure_real_uninstall (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv = gimp_procedure_get_instance_private (procedure);
  GimpPlugIn           *plug_in;
  GPProcUninstall       proc_uninstall;

  g_return_if_fail (priv->installed == TRUE);

  proc_uninstall.name = (gchar *) gimp_procedure_get_name (procedure);

  plug_in = gimp_procedure_get_plug_in (procedure);

  if (! gp_proc_uninstall_write (_gimp_plug_in_get_write_channel (plug_in),
                                 &proc_uninstall, plug_in))
    gimp_quit ();

  priv->installed = FALSE;
}

static GimpValueArray *
gimp_procedure_real_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  GimpProcedurePrivate *priv     = gimp_procedure_get_instance_private (procedure);
  GimpPlugIn           *plug_in;
  GimpProcedureConfig  *config;
  GimpImage            *image    = NULL;
  GimpRunMode           run_mode = GIMP_RUN_NONINTERACTIVE;
  GimpValueArray       *retvals;
  GimpPDBStatusType     status   = GIMP_PDB_EXECUTION_ERROR;

  if (gimp_value_array_length (args) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (args, 0)))
    {
      run_mode = GIMP_VALUES_GET_ENUM (args, 0);

      if (gimp_value_array_length (args) > 1 &&
          GIMP_VALUE_HOLDS_IMAGE (gimp_value_array_index (args, 1)))
        image = GIMP_VALUES_GET_IMAGE (args, 1);
    }

  config = _gimp_procedure_create_run_config (procedure);
  _gimp_procedure_config_begin_run (config, image, run_mode, args);

  retvals = priv->run_func (procedure, config, priv->run_data);

  if (retvals != NULL                       &&
      gimp_value_array_length (retvals) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (retvals, 0)))
    status = GIMP_VALUES_GET_ENUM (retvals, 0);

  _gimp_procedure_config_end_run (config, status);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = gimp_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _gimp_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the GimpProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);

  return retvals;
}

static GimpProcedureConfig *
gimp_procedure_real_create_config (GimpProcedure  *procedure,
                                   GParamSpec    **args,
                                   gint            n_args)
{
  return gimp_procedure_create_config_with_prefix (procedure,
                                                   "GimpProcedureConfigRun",
                                                   args, n_args);
}

static GimpProcedureConfig *
gimp_procedure_create_config_with_prefix (GimpProcedure  *procedure,
                                          const gchar    *prefix,
                                          GParamSpec    **args,
                                          gint            n_args)
{
  GimpProcedurePrivate *priv = gimp_procedure_get_instance_private (procedure);
  gchar                *type_name;
  GType                 type;

  type_name = g_strdup_printf ("%s-%s", prefix,
                               gimp_procedure_get_name (procedure));

  type = g_type_from_name (type_name);

  if (! type)
    {
      GParamSpec **config_args;
      gint         n_config_args;

      n_config_args = n_args + priv->n_aux_args;

      config_args = g_new0 (GParamSpec *, n_config_args);

      memcpy (config_args,
              args,
              n_args * sizeof (GParamSpec *));
      memcpy (config_args + n_args,
              priv->aux_args,
              priv->n_aux_args * sizeof (GParamSpec *));

      type = gimp_config_type_register (GIMP_TYPE_PROCEDURE_CONFIG,
                                        type_name,
                                        config_args, n_config_args);

      g_free (config_args);
    }

  g_free (type_name);

  return g_object_new (type,
                       "procedure", procedure,
                       NULL);
}


/*  public functions  */

/**
 * gimp_procedure_new:
 * @plug_in:   a #GimpPlugIn.
 * @name:      the new procedure's name.
 * @proc_type: the new procedure's #GimpPDBProcType.
 * @run_func:  the run function for the new procedure.
 * @run_data:  user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new procedure named @name which will call @run_func when
 * invoked.
 *
 * The @name parameter is mandatory and should be unique, or it will
 * overwrite an already existing procedure (overwrite procedures only
 * if you know what you're doing).
 *
 * @proc_type should be %GIMP_PDB_PROC_TYPE_PLUGIN for "normal" plug-ins.
 *
 * Using %GIMP_PDB_PROC_TYPE_EXTENSION means that the plug-in will add
 * temporary procedures. Therefore, the GIMP core will wait until the
 * %GIMP_PDB_PROC_TYPE_EXTENSION procedure has called
 * [method@Procedure.extension_ready], which means that the procedure
 * has done its initialization, installed its temporary procedures and
 * is ready to run.
 *
 * *Not calling [method@Procedure.extension_ready] from a
 * %GIMP_PDB_PROC_TYPE_EXTENSION procedure will cause the GIMP core to
 * lock up.*
 *
 * Additionally, a %GIMP_PDB_PROC_TYPE_EXTENSION procedure with no
 * arguments added is an "automatic" extension that will be
 * automatically started on each GIMP startup.
 *
 * %GIMP_PDB_PROC_TYPE_TEMPORARY must be used for temporary procedures
 * that are created during a plug-ins lifetime. They must be added to
 * the #GimpPlugIn using [method@PlugIn.add_temp_procedure].
 *
 * @run_func is called via [method@Procedure.run].
 *
 * For %GIMP_PDB_PROC_TYPE_PLUGIN and %GIMP_PDB_PROC_TYPE_EXTENSION
 * procedures the call of @run_func is basically the lifetime of the
 * plug-in.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_procedure_new (GimpPlugIn        *plug_in,
                    const gchar       *name,
                    GimpPDBProcType    proc_type,
                    GimpRunFunc        run_func,
                    gpointer           run_data,
                    GDestroyNotify     run_data_destroy)
{
  GimpProcedure        *procedure;
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  priv->run_func         = run_func;
  priv->run_data         = run_data;
  priv->run_data_destroy = run_data_destroy;

  return procedure;
}
/**
 * gimp_procedure_get_plug_in:
 * @procedure: A #GimpProcedure.
 *
 * Returns: (transfer none): The #GimpPlugIn given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
GimpPlugIn *
gimp_procedure_get_plug_in (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->plug_in;
}

/**
 * gimp_procedure_get_name:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's name given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_name (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->name;
}

/**
 * gimp_procedure_get_proc_type:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's type given in [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
GimpPDBProcType
gimp_procedure_get_proc_type (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure),
                        GIMP_PDB_PROC_TYPE_PLUGIN);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->proc_type;
}

/**
 * gimp_procedure_set_image_types:
 * @procedure:   A #GimpProcedure.
 * @image_types: The image types this procedure can operate on.
 *
 * This is a comma separated list of image types, or actually drawable
 * types, that this procedure can deal with. Wildcards are possible
 * here, so you could say "RGB*" instead of "RGB, RGBA" or "*" for all
 * image types.
 *
 * Supported types are "RGB", "GRAY", "INDEXED" and their variants
 * with alpha.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_image_types (GimpProcedure *procedure,
                                const gchar   *image_types)
{
  GimpProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  g_free (priv->image_types);
  priv->image_types = g_strdup (image_types);

  if (priv->installed)
    _gimp_pdb_set_proc_image_types (gimp_procedure_get_name (procedure),
                                    priv->image_types);
}

/**
 * gimp_procedure_get_image_types:
 * @procedure:  A #GimpProcedure.
 *
 * This function retrieves the list of image types the procedure can
 * operate on. See gimp_procedure_set_image_types().
 *
 * Returns: The image types.
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_image_types (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->image_types;
}

/**
 * gimp_procedure_set_sensitivity_mask:
 * @procedure:        A #GimpProcedure.
 * @sensitivity_mask: A binary mask of #GimpProcedureSensitivityMask.
 *
 * Sets the case when @procedure is supposed to be sensitive or not.
 * Note that it will be used by the core to determine whether to show a
 * procedure as sensitive (hence forbid running it otherwise), yet it
 * will not forbid thid-party plug-ins for instance to run manually your
 * registered procedure. Therefore you should still handle non-supported
 * cases appropriately by returning with %GIMP_PDB_EXECUTION_ERROR and a
 * suitable error message.
 *
 * Similarly third-party plug-ins should verify they are allowed to call
 * a procedure with [method@Procedure.get_sensitivity_mask] when running
 * with dynamic contents.
 *
 * Note that by default, a procedure works on an image with a single
 * drawable selected. Hence not setting the mask, setting it with 0 or
 * setting it with a mask of %GIMP_PROCEDURE_SENSITIVE_DRAWABLE only are
 * equivalent.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_sensitivity_mask (GimpProcedure *procedure,
                                     gint           sensitivity_mask)
{
  GimpProcedurePrivate *priv;
  gboolean              success = TRUE;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  if (GIMP_PROCEDURE_GET_CLASS (procedure)->set_sensitivity)
    success = GIMP_PROCEDURE_GET_CLASS (procedure)->set_sensitivity (procedure,
                                                                     sensitivity_mask);

  if (success)
    {
      priv->sensitivity_mask = sensitivity_mask;

      if (priv->installed)
        _gimp_pdb_set_proc_sensitivity_mask (gimp_procedure_get_name (procedure),
                                             priv->sensitivity_mask);
    }
}

/**
 * gimp_procedure_get_sensitivity_mask:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's sensitivity mask given in
 *          [method@Procedure.set_sensitivity_mask].
 *
 * Since: 3.0
 **/
gint
gimp_procedure_get_sensitivity_mask (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), 0);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->sensitivity_mask;
}

/**
 * gimp_procedure_set_menu_label:
 * @procedure:  A #GimpProcedure.
 * @menu_label: The @procedure's menu label.
 *
 * Sets the label to use for the @procedure's menu entry, The
 * location(s) where to register in the menu hierarchy is chosen using
 * gimp_procedure_add_menu_path().
 *
 * Plug-ins are responsible for their own translations. You are expected to send
 * localized strings to GIMP if your plug-in is internationalized.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_menu_label (GimpProcedure *procedure,
                               const gchar   *menu_label)
{
  GimpProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_label != NULL && strlen (menu_label));

  priv = gimp_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->menu_label, g_free);
  priv->menu_label = g_strdup (menu_label);

  if (priv->installed)
    _gimp_pdb_set_proc_menu_label (gimp_procedure_get_name (procedure),
                                   priv->menu_label);
}

/**
 * gimp_procedure_get_menu_label:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's menu label given in
 *          gimp_procedure_set_menu_label().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_menu_label (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->menu_label;
}

/**
 * gimp_procedure_add_menu_path:
 * @procedure: A #GimpProcedure.
 * @menu_path: The @procedure's additional menu path.
 *
 * Adds a menu path to the procedure. Only procedures which have a menu
 * label can add a menu path.
 *
 * Menu paths are untranslated paths to known menus and submenus with the
 * syntax `<Prefix>/Path/To/Submenu`, for example `<Image>/Layer/Transform`.
 * GIMP will localize these.
 * Nevertheless you should localize unknown parts of the path. For instance, say
 * you want to create procedure to create customized layers and add a `Create`
 * submenu which you want to localize from your plug-in with gettext. You could
 * call:
 *
 * ```C
 * path = g_build_path ("/", "<Image>/Layer", _("Create"), NULL);
 * gimp_procedure_add_menu_path (procedure, path);
 * g_free (path);
 * ```
 *
 * See also: gimp_plug_in_add_menu_branch().
 *
 * GIMP menus also have a concept of named section. For instance, say you are
 * creating a plug-in which you want to show next to the "Export", "Export As"
 * plug-ins in the File menu. You would add it to the menu path "File/[Export]".
 * If you actually wanted to create a submenu called "[Export]" (with square
 * brackets), double the brackets: "File/[[Export]]"
 *
 * See also: https://gitlab.gnome.org/GNOME/gimp/-/blob/master/menus/image-menu.ui.in.in
 *
 * This function will place your procedure to the bottom of the selected path or
 * section. Order is not assured relatively to other plug-ins.
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_menu_path (GimpProcedure *procedure,
                              const gchar   *menu_path)
{
  GimpProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_path != NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->menu_label != NULL);

  priv->menu_paths = g_list_append (priv->menu_paths, g_strdup (menu_path));

  if (priv->installed)
    _gimp_pdb_add_proc_menu_path (gimp_procedure_get_name (procedure),
                                  menu_path);
}

/**
 * gimp_procedure_get_menu_paths:
 * @procedure: A #GimpProcedure.
 *
 * Returns: (transfer none) (element-type gchar*): the @procedure's
 *          menu paths as added with gimp_procedure_add_menu_path().
 *
 * Since: 3.0
 **/
GList *
gimp_procedure_get_menu_paths (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->menu_paths;
}

/**
 * gimp_procedure_set_icon_name:
 * @procedure: a #GimpProcedure.
 * @icon_name: (nullable): an icon name.
 *
 * Sets the icon for @procedure to the icon referenced by @icon_name.
 *
 * Since: 3.0
 */
void
gimp_procedure_set_icon_name (GimpProcedure *procedure,
                              const gchar   *icon_name)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  gimp_procedure_set_icon (procedure,
                           GIMP_ICON_TYPE_ICON_NAME,
                           icon_name);
}

/**
 * gimp_procedure_set_icon_pixbuf:
 * @procedure: a #GimpProcedure.
 * @pixbuf:    (nullable): a #GdkPixbuf.
 *
 * Sets the icon for @procedure to @pixbuf.
 *
 * Since: 3.0
 */
void
gimp_procedure_set_icon_pixbuf (GimpProcedure *procedure,
                                GdkPixbuf     *pixbuf)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  gimp_procedure_set_icon (procedure,
                           GIMP_ICON_TYPE_PIXBUF,
                           pixbuf);
}

/**
 * gimp_procedure_set_icon_file:
 * @procedure: a #GimpProcedure.
 * @file:      (nullable): a #GFile pointing to an image file.
 *
 * Sets the icon for @procedure to the contents of an image file.
 *
 * Since: 3.0
 */
void
gimp_procedure_set_icon_file (GimpProcedure *procedure,
                              GFile         *file)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (file == NULL || G_IS_FILE (file));

  gimp_procedure_set_icon (procedure,
                           GIMP_ICON_TYPE_IMAGE_FILE,
                           file);
}

/**
 * gimp_procedure_get_icon_type:
 * @procedure: a #GimpProcedure.
 *
 * Gets the type of data set as @procedure's icon. Depending on the
 * result, you can call the relevant specific function, such as
 * [method@Procedure.get_icon_name].
 *
 * Returns: the #GimpIconType of @procedure's icon.
 *
 * Since: 3.0
 */
GimpIconType
gimp_procedure_get_icon_type (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), -1);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->icon_type;
}

/**
 * gimp_procedure_get_icon_name:
 * @procedure: a #GimpProcedure.
 *
 * Gets the name of the icon if one was set for @procedure.
 *
 * Returns: (nullable): the icon name or %NULL if no icon name was set.
 *
 * Since: 3.0
 */
const gchar *
gimp_procedure_get_icon_name (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (priv->icon_type == GIMP_ICON_TYPE_ICON_NAME)
    return (const gchar *) priv->icon_data;

  return NULL;
}

/**
 * gimp_procedure_get_icon_file:
 * @procedure: a #GimpProcedure.
 *
 * Gets the file of the icon if one was set for @procedure.
 *
 * Returns: (nullable) (transfer none): the icon #GFile or %NULL if no
 *          file was set.
 *
 * Since: 3.0
 */
GFile *
gimp_procedure_get_icon_file (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (priv->icon_type == GIMP_ICON_TYPE_IMAGE_FILE)
    return (GFile *) priv->icon_data;

  return NULL;
}

/**
 * gimp_procedure_get_icon_pixbuf:
 * @procedure: a #GimpProcedure.
 *
 * Gets the #GdkPixbuf of the icon if an icon was set this way for
 * @procedure.
 *
 * Returns: (nullable) (transfer none): the icon pixbuf or %NULL if no
 *          icon name was set.
 *
 * Since: 3.0
 */
GdkPixbuf *
gimp_procedure_get_icon_pixbuf (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (priv->icon_type == GIMP_ICON_TYPE_PIXBUF)
    return (GdkPixbuf *) priv->icon_data;

  return NULL;
}

/**
 * gimp_procedure_set_documentation:
 * @procedure:           A #GimpProcedure.
 * @blurb:               The @procedure's blurb.
 * @help:    (nullable): The @procedure's help text.
 * @help_id: (nullable): The @procedure's help ID.
 *
 * Sets various documentation strings on @procedure:
 *
 * * @blurb is used for instance as the @procedure's tooltip when represented in
 *   the UI such as a menu entry.
 * * @help is a free-form text that's meant as additional documentation for
 *   developers of scripts and plug-ins. If the @blurb and the argument names
 *   and descriptions are enough for a quite self-explanatory procedure, you may
 *   set @help to %NULL, rather than setting an uninformative @help (avoid
 *   setting the same text as @blurb or redundant information).
 *
 * Plug-ins are responsible for their own translations. You are expected to send
 * localized strings of @blurb and @help to GIMP if your plug-in is
 * internationalized.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_documentation (GimpProcedure *procedure,
                                  const gchar   *blurb,
                                  const gchar   *help,
                                  const gchar   *help_id)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->blurb,   g_free);
  g_clear_pointer (&priv->help,    g_free);
  g_clear_pointer (&priv->help_id, g_free);

  priv->blurb   = g_strdup (blurb);
  priv->help    = g_strdup (help);
  priv->help_id = g_strdup (help_id);

  if (priv->installed)
    _gimp_pdb_set_proc_documentation (gimp_procedure_get_name (procedure),
                                      priv->blurb,
                                      priv->help,
                                      priv->help_id);
}

/**
 * gimp_procedure_get_blurb:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's blurb given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_blurb (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->blurb;
}

/**
 * gimp_procedure_get_help:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's help text given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_help (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->help;
}

/**
 * gimp_procedure_get_help_id:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's help ID given in
 * [method@Procedure.set_documentation].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_help_id (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->help_id;
}

/**
 * gimp_procedure_set_attribution:
 * @procedure: A #GimpProcedure.
 * @authors:   The @procedure's author(s).
 * @copyright: The @procedure's copyright.
 * @date:      The @procedure's date (written or published).
 *
 * Sets various attribution strings on @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_attribution (GimpProcedure *procedure,
                                const gchar   *authors,
                                const gchar   *copyright,
                                const gchar   *date)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  g_free (priv->authors);
  g_free (priv->copyright);
  g_free (priv->date);

  priv->authors   = g_strdup (authors);
  priv->copyright = g_strdup (copyright);
  priv->date      = g_strdup (date);

  if (priv->installed)
    _gimp_pdb_set_proc_attribution (gimp_procedure_get_name (procedure),
                                    priv->authors,
                                    priv->copyright,
                                    priv->date);
}

/**
 * gimp_procedure_get_authors:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's authors given in [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_authors (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->authors;
}

/**
 * gimp_procedure_get_copyright:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's copyright given in
 * [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_copyright (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->copyright;
}

/**
 * gimp_procedure_get_date:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's date given in [method@Procedure.set_attribution].
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_date (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return priv->date;
}

/**
 * _gimp_procedure_add_argument:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer floating): the argument specification.
 *
 * Add a new argument to @procedure according to @pspec specifications.
 * The arguments will be ordered according to the calls order.
 *
 * If @pspec is floating, ownership will be taken over by @procedure,
 * allowing to pass directly `g*_param_spec_*()` calls as arguments.
 *
 * Returns: (transfer none): the same @pspec or %NULL in case of error.
 *
 * Since: 3.0
 **/
GParamSpec *
_gimp_procedure_add_argument (GimpProcedure *procedure,
                              GParamSpec    *pspec)
{
  GimpProcedurePrivate *priv;

  priv = gimp_procedure_get_instance_private (procedure);
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (pspec->name), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (gimp_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      g_param_spec_sink (pspec);
      return NULL;
    }

  if (gimp_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      g_param_spec_sink (pspec);
      return NULL;
    }

  priv->args = g_renew (GParamSpec *, priv->args, priv->n_args + 1);

  priv->args[priv->n_args] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_args++;

  return pspec;
}

/**
 * _gimp_procedure_add_aux_argument:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer full): the argument specification.
 *
 * Add a new auxiliary argument to @procedure according to @pspec
 * specifications.
 *
 * Auxiliary arguments are not passed to the @procedure in run() and
 * are not known to the PDB. They are however members of the
 * #GimpProcedureConfig created by gimp_procedure_create_config() and
 * can be used to persistently store whatever last used values the
 * @procedure wants to remember across invocations
 *
 * Returns: (transfer none): the same @pspec.
 *
 * Since: 3.0
 **/
GParamSpec *
_gimp_procedure_add_aux_argument (GimpProcedure *procedure,
                                  GParamSpec    *pspec)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), pspec);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), pspec);
  g_return_val_if_fail (gimp_is_canonical_identifier (pspec->name), pspec);

  priv = gimp_procedure_get_instance_private (procedure);

  if (gimp_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return pspec;
    }

  if (gimp_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return pspec;
    }

  priv->aux_args = g_renew (GParamSpec *, priv->aux_args, priv->n_aux_args + 1);

  priv->aux_args[priv->n_aux_args] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_aux_args++;

  return pspec;
}

/**
 * _gimp_procedure_add_return_value:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer full): the return value specification.
 *
 * Add a new return value to @procedure according to @pspec
 * specifications.
 *
 * The returned values will be ordered according to the calls order.
 *
 * Returns: (transfer none): the same @pspec.
 *
 * Since: 3.0
 **/
GParamSpec *
_gimp_procedure_add_return_value (GimpProcedure *procedure,
                                  GParamSpec    *pspec)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), pspec);
  g_return_val_if_fail (G_IS_PARAM_SPEC (pspec), pspec);
  g_return_val_if_fail (gimp_is_canonical_identifier (pspec->name), pspec);

  priv = gimp_procedure_get_instance_private (procedure);

  if (gimp_procedure_find_return_value (procedure, pspec->name))
    {
      g_warning ("Return value with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return pspec;
    }

  priv->values = g_renew (GParamSpec *, priv->values, priv->n_values + 1);

  priv->values[priv->n_values] = pspec;

  g_param_spec_ref_sink (pspec);

  priv->n_values++;

  return pspec;
}

/**
 * gimp_procedure_find_argument:
 * @procedure: A #GimpProcedure
 * @name:      An argument name
 *
 * Searches the @procedure's arguments for a #GParamSpec called @name.
 *
 * Returns: (transfer none): The @procedure's argument with @name if it
 *          exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_procedure_find_argument (GimpProcedure *procedure,
                              const gchar   *name)
{
  GimpProcedurePrivate *priv;
  gint                  i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  for (i = 0; i < priv->n_args; i++)
    if (! strcmp (name, priv->args[i]->name))
      return priv->args[i];

  return NULL;
}

/**
 * gimp_procedure_find_aux_argument:
 * @procedure: A #GimpProcedure
 * @name:      An auxiliary argument name
 *
 * Searches the @procedure's auxiliary arguments for a #GParamSpec
 * called @name.
 *
 * Returns: (transfer none): The @procedure's auxiliary argument with
 *          @name if it exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_procedure_find_aux_argument (GimpProcedure *procedure,
                                  const gchar   *name)
{
  GimpProcedurePrivate *priv;
  gint                  i;

  priv = gimp_procedure_get_instance_private (procedure);

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < priv->n_aux_args; i++)
    if (! strcmp (name, priv->aux_args[i]->name))
      return priv->aux_args[i];

  return NULL;
}

/**
 * gimp_procedure_find_return_value:
 * @procedure: A #GimpProcedure
 * @name:      A return value name
 *
 * Searches the @procedure's return values for a #GParamSpec called
 * @name.
 *
 * Returns: (transfer none): The @procedure's return values with @name
 *          if it exists, or %NULL otherwise.
 *
 * Since: 3.0
 **/
GParamSpec *
gimp_procedure_find_return_value (GimpProcedure *procedure,
                                  const gchar   *name)
{
  GimpProcedurePrivate *priv;
  gint                  i;

  priv = gimp_procedure_get_instance_private (procedure);

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < priv->n_values; i++)
    if (! strcmp (name, priv->values[i]->name))
      return priv->values[i];

  return NULL;
}

/**
 * gimp_procedure_get_arguments:
 * @procedure:   A #GimpProcedure.
 * @n_arguments: (out): Returns the number of arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_arguments (GimpProcedure *procedure,
                              gint          *n_arguments)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  *n_arguments = priv->n_args;

  return priv->args;
}

/**
 * gimp_procedure_get_aux_arguments:
 * @procedure:   A #GimpProcedure.
 * @n_arguments: (out): Returns the number of auxiliary arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_aux_arguments (GimpProcedure *procedure,
                                  gint          *n_arguments)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  *n_arguments = priv->n_aux_args;

  return priv->aux_args;
}

/**
 * gimp_procedure_get_return_values:
 * @procedure:       A #GimpProcedure.
 * @n_return_values: (out): Returns the number of return values.
 *
 * Returns: (transfer none) (array length=n_return_values): An array
 *          of @GParamSpec in the order they were added in.
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_return_values (GimpProcedure *procedure,
                                  gint          *n_return_values)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_return_values != NULL, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  *n_return_values = priv->n_values;

  return priv->values;
}

/**
 * gimp_procedure_set_argument_sync:
 * @procedure: a #GimpProcedure.
 * @arg_name:  the name of one of @procedure's arguments or auxiliary arguments.
 * @sync:      how to sync the argument or auxiliary argument.
 *
 * When the procedure's run() function exits, a #GimpProcedure's arguments
 * or auxiliary arguments can be automatically synced with a #GimpParasite of
 * the #GimpImage the procedure is running on.
 *
 * In order to enable this, set @sync to %GIMP_ARGUMENT_SYNC_PARASITE.
 *
 * Currently, it is possible to sync a string argument of type
 * #GParamSpecString with an image parasite of the same name, for
 * example the "gimp-comment" parasite in file save procedures.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_argument_sync (GimpProcedure    *procedure,
                                  const gchar      *arg_name,
                                  GimpArgumentSync  sync)
{
  GParamSpec *pspec;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (arg_name != NULL);

  pspec = gimp_procedure_find_argument (procedure, arg_name);

  if (! pspec)
    pspec = gimp_procedure_find_aux_argument (procedure, arg_name);

  g_return_if_fail (pspec != NULL);

  switch (sync)
    {
    case GIMP_ARGUMENT_SYNC_NONE:
      gegl_param_spec_set_property_key (pspec, "gimp-argument-sync", NULL);
      break;

    case GIMP_ARGUMENT_SYNC_PARASITE:
      gegl_param_spec_set_property_key (pspec, "gimp-argument-sync", "parasite");
      break;
    }
}

/**
 * gimp_procedure_get_argument_sync:
 * @procedure: a #GimpProcedure
 * @arg_name:  the name of one of @procedure's arguments or auxiliary arguments
 *
 * Returns: The #GimpArgumentSync value set with
 *          gimp_procedure_set_argument_sync():
 *
 * Since: 3.0
 **/
GimpArgumentSync
gimp_procedure_get_argument_sync (GimpProcedure *procedure,
                                  const gchar   *arg_name)
{
  GParamSpec       *pspec;
  GimpArgumentSync  sync = GIMP_ARGUMENT_SYNC_NONE;
  const gchar      *value;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), GIMP_ARGUMENT_SYNC_NONE);
  g_return_val_if_fail (arg_name != NULL, GIMP_ARGUMENT_SYNC_NONE);

  pspec = gimp_procedure_find_argument (procedure, arg_name);

  if (! pspec)
    pspec = gimp_procedure_find_aux_argument (procedure, arg_name);

  g_return_val_if_fail (pspec != NULL, GIMP_ARGUMENT_SYNC_NONE);

  value = gegl_param_spec_get_property_key (pspec, "gimp-argument-sync");

  if (value)
    {
      if (! strcmp (value, "parasite"))
        sync = GIMP_ARGUMENT_SYNC_PARASITE;
    }

  return sync;
}

/**
 * gimp_procedure_new_return_values:
 * @procedure: the procedure.
 * @status:    the success status of the procedure run.
 * @error:     (in) (nullable) (transfer full):
 *             an optional #GError. This parameter should be set if
 *             @status is either #GIMP_PDB_EXECUTION_ERROR or
 *             #GIMP_PDB_CALLING_ERROR.
 *
 * Format the expected return values from procedures.
 *
 * Returns: (transfer full): the expected #GimpValueArray as could be returned
 * by a [callback@RunFunc].
 *
 * Since: 3.0
 **/
GimpValueArray *
gimp_procedure_new_return_values (GimpProcedure     *procedure,
                                  GimpPDBStatusType  status,
                                  GError            *error)
{
  GimpProcedurePrivate *priv;
  GimpValueArray       *args;
  GValue                value = G_VALUE_INIT;
  gint                  i;

  g_return_val_if_fail (status != GIMP_PDB_PASS_THROUGH, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
    case GIMP_PDB_CANCEL:
      g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

      args = gimp_value_array_new (priv->n_values + 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < priv->n_values; i++)
        {
          GParamSpec *pspec = priv->values[i];

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
          g_param_value_set_default (pspec, &value);
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    case GIMP_PDB_EXECUTION_ERROR:
    case GIMP_PDB_CALLING_ERROR:
      args = gimp_value_array_new ((error && error->message) ? 2 : 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      if (error && error->message)
        {
          g_value_init (&value, G_TYPE_STRING);
          g_value_set_string (&value, error->message);
          gimp_value_array_append (args, &value);
          g_value_unset (&value);
        }
      break;

    default:
      g_return_val_if_reached (NULL);
    }

  g_clear_error (&error);

  return args;
}

/**
 * gimp_procedure_run: (skip)
 * @procedure:      the [class@Gimp.Procedure] to run.
 * @first_arg_name: the name of an argument of @procedure or %NULL to
 *                  run @procedure with default arguments.
 * @...:            the value of @first_arg_name and any more argument
 *                  names and values as needed.
 *
 * Runs the procedure named @procedure_name with arguments given as
 * list of `(name, value)` pairs, terminated by %NULL.
 *
 * The order of arguments does not matter and if any argument is missing, its
 * default value will be used. The value type must correspond to the argument
 * type as registered for @procedure_name.
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
gimp_procedure_run (GimpProcedure *procedure,
                    const gchar   *first_arg_name,
                    ...)
{
  GimpValueArray *return_values;
  va_list         args;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  va_start (args, first_arg_name);

  return_values = gimp_procedure_run_valist (procedure, first_arg_name, args);

  va_end (args);

  return return_values;
}

/**
 * gimp_procedure_run_valist: (skip)
 * @procedure:      the [class@Gimp.Procedure] to run.
 * @first_arg_name: the name of an argument of @procedure or %NULL to
 *                  run @procedure with default arguments.
 * @args            the value of @first_arg_name and any more argument
 *                  names and values as needed.
 *
 * Runs @procedure with arguments names and values, given in the order as passed
 * to [method@Procedure.run].
 *
 * Returns: (transfer full): the return values for the procedure call.
 *
 * Since: 3.0
 */
GimpValueArray *
gimp_procedure_run_valist (GimpProcedure *procedure,
                           const gchar   *first_arg_name,
                           va_list        args)
{
  GimpValueArray      *return_values;
  GimpProcedureConfig *config;
  const gchar         *arg_name;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  config   = gimp_procedure_create_config (procedure);
  arg_name = first_arg_name;

  while (arg_name != NULL)
    {
      GParamSpec *pspec;
      gchar      *error = NULL;
      GValue      value = G_VALUE_INIT;

      pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), arg_name);

      if (pspec == NULL)
        {
          g_warning ("%s: %s has no property named '%s'",
                     G_STRFUNC,
                     g_type_name (G_TYPE_FROM_INSTANCE (config)),
                     arg_name);
          g_clear_object (&config);
          return NULL;
        }

      g_value_init (&value, pspec->value_type);
      G_VALUE_COLLECT (&value, args, G_VALUE_NOCOPY_CONTENTS, &error);

      if (error)
        {
          g_warning ("%s: %s", G_STRFUNC, error);
          g_free (error);
          g_clear_object (&config);
          return NULL;
        }

      g_object_set_property (G_OBJECT (config), arg_name, &value);
      g_value_unset (&value);

      arg_name = va_arg (args, const gchar *);
    }

  return_values = gimp_procedure_run_config (procedure, config);
  g_clear_object (&config);

  return return_values;
}

/**
 * gimp_procedure_run_config: (rename-to gimp_procedure_run)
 * @procedure:          the [class@Gimp.Procedure] to run.
 * @config: (nullable): the @procedure's arguments.
 *
 * Runs @procedure, calling the run_func given in [ctor@Procedure.new].
 *
 * Create @config at default values with
 * [method@Gimp.Procedure.create_config] then set any argument you wish
 * to change from defaults with [method@GObject.Object.set].
 *
 * If @config is %NULL, the default arguments of @procedure will be used.
 *
 * Returns: (transfer full): The @procedure's return values.
 *
 * Since: 3.0
 **/
GimpValueArray *
gimp_procedure_run_config (GimpProcedure       *procedure,
                           GimpProcedureConfig *config)
{
  GimpValueArray *return_vals;
  GimpValueArray *args;
  gboolean        free_config = FALSE;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (config == NULL || GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (config == NULL || gimp_procedure_config_get_procedure (config) == procedure, NULL);

  if (config == NULL)
    {
      config = gimp_procedure_create_config (procedure);
      free_config = TRUE;
    }

  args = gimp_procedure_new_arguments (procedure);
  _gimp_procedure_config_get_values (config, args);

  return_vals = _gimp_procedure_run_array (procedure, args);

  gimp_value_array_unref (args);

  if (free_config)
    g_object_unref (config);

  return return_vals;
}

/**
 * _gimp_procedure_create_run_config:
 * @procedure: A #GimpProcedure
 *
 * Create a #GimpConfig with properties that match @procedure's arguments, to be
 * used in the run() method for @procedure.
 *
 * This is an internal function to be used only by libgimp code.
 *
 * Returns: (transfer full): The new #GimpConfig.
 *
 * Since: 3.0
 **/
GimpProcedureConfig *
_gimp_procedure_create_run_config (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->create_config (procedure,
                                                              priv->args,
                                                              priv->n_args);
}

GimpValueArray *
_gimp_procedure_run_array (GimpProcedure  *procedure,
                           GimpValueArray *args)
{
  GimpProcedurePrivate *priv;
  GimpValueArray       *return_vals;
  GError               *error = NULL;
  gint                  i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (! gimp_procedure_validate_args (procedure,
                                      priv->args,
                                      priv->n_args,
                                      args, FALSE, &error))
    {
      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_CALLING_ERROR,
                                                      error);
      return return_vals;
    }

  /*  add missing args with default values  */
  if (gimp_value_array_length (args) < priv->n_args)
    {
      GimpProcedureConfig *config;
      GObjectClass        *config_class = NULL;
      GimpValueArray      *complete;

      /*  if saved defaults exist, they override GParamSpec  */
      config = _gimp_procedure_create_run_config (procedure);
      if (gimp_procedure_config_load_default (config, NULL))
        config_class = G_OBJECT_GET_CLASS (config);
      else
        g_clear_object (&config);

      complete = gimp_value_array_new (priv->n_args);

      for (i = 0; i < priv->n_args; i++)
        {
          GParamSpec *pspec = priv->args[i];
          GValue      value = G_VALUE_INIT;

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

          if (i < gimp_value_array_length (args))
            {
              GValue *orig = gimp_value_array_index (args, i);

              g_value_copy (orig, &value);
            }
          else if (config &&
                   g_object_class_find_property (config_class, pspec->name))
            {
              g_object_get_property (G_OBJECT (config), pspec->name,
                                     &value);
            }
          else
            {
              g_param_value_set_default (pspec, &value);
            }

          gimp_value_array_append (complete, &value);
          g_value_unset (&value);
        }

      g_clear_object (&config);

      /*  call the procedure  */
      return_vals = GIMP_PROCEDURE_GET_CLASS (procedure)->run (procedure,
                                                               complete);
      gimp_value_array_unref (complete);
    }
  else
    {
      /*  call the procedure  */
      return_vals = GIMP_PROCEDURE_GET_CLASS (procedure)->run (procedure,
                                                               args);
    }

  if (! return_vals)
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      error = g_error_new (0, 0,
                           _("Procedure '%s' returned no return values"),
                           gimp_procedure_get_name (procedure));

      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_EXECUTION_ERROR,
                                                      error);
    }

  return return_vals;
}

/**
 * gimp_procedure_extension_ready:
 * @procedure: A #GimpProcedure
 *
 * Notify the main GIMP application that the extension has been
 * properly initialized and is ready to run.
 *
 * This function _must_ be called from every procedure's [callback@RunFunc]
 * that was created as #GIMP_PDB_PROC_TYPE_EXTENSION.
 *
 * Subsequently, extensions can process temporary procedure run
 * requests using either [method@PlugIn.extension_enable] or
 * [method@PlugIn.extension_process].
 *
 * See also: [ctor@Procedure.new].
 *
 * Since: 3.0
 **/
void
gimp_procedure_extension_ready (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;
  GimpPlugIn           *plug_in;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  g_return_if_fail (priv->proc_type == GIMP_PDB_PROC_TYPE_EXTENSION);

  plug_in = gimp_procedure_get_plug_in (procedure);

  if (! gp_extension_ack_write (_gimp_plug_in_get_write_channel (plug_in),
                                plug_in))
    gimp_quit ();
}

/**
 * gimp_procedure_create_config:
 * @procedure: A #GimpProcedure
 *
 * Create a #GimpConfig with properties that match @procedure's arguments, to be
 * used in [method@Procedure.run_config] method.
 *
 * Returns: (transfer full): The new #GimpConfig.
 *
 * Since: 3.0
 **/
GimpProcedureConfig *
gimp_procedure_create_config (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  return gimp_procedure_create_config_with_prefix (procedure,
                                                   "GimpProcedureConfig",
                                                   priv->args,
                                                   priv->n_args);
}


/*  private functions  */

/**
 * gimp_procedure_new_arguments:
 * @procedure: the #GimpProcedure.
 *
 * Format the expected argument values of procedures, in the order as
 * added with [method@Procedure.add_argument].
 *
 * Returns: (transfer full): the expected #GimpValueArray which could be given as
 *          arguments to run @procedure, with all values set to
 *          defaults. Free with gimp_value_array_unref().
 *
 * Since: 3.0
 **/
static GimpValueArray *
gimp_procedure_new_arguments (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;
  GimpValueArray       *args;
  GValue                value = G_VALUE_INIT;
  gint                  i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  args = gimp_value_array_new (priv->n_args);

  for (i = 0; i < priv->n_args; i++)
    {
      GParamSpec *pspec = priv->args[i];

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &value);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

static gboolean
gimp_procedure_validate_args (GimpProcedure   *procedure,
                              GParamSpec     **param_specs,
                              gint             n_param_specs,
                              GimpValueArray  *args,
                              gboolean         return_vals,
                              GError         **error)
{
  gint i;

  for (i = 0; i < MIN (gimp_value_array_length (args), n_param_specs); i++)
    {
      GValue     *arg       = gimp_value_array_index (args, i);
      GParamSpec *pspec     = param_specs[i];
      GType       arg_type  = G_VALUE_TYPE (arg);
      GType       spec_type = G_PARAM_SPEC_VALUE_TYPE (pspec);

      if (! g_type_is_a (arg_type, spec_type))
        {
          if (return_vals)
            {
              g_set_error (error,
                           GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                           _("Procedure '%s' returned a wrong value type "
                             "for return value '%s' (#%d). "
                             "Expected %s, got %s."),
                           gimp_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }
          else
            {
              g_set_error (error,
                           GIMP_PDB_ERROR, GIMP_PDB_ERROR_INVALID_ARGUMENT,
                           _("Procedure '%s' has been called with a "
                             "wrong value type for argument '%s' (#%d). "
                             "Expected %s, got %s."),
                           gimp_procedure_get_name (procedure),
                           g_param_spec_get_name (pspec),
                           i + 1, g_type_name (spec_type),
                           g_type_name (arg_type));
            }

          return FALSE;
        }
      else if (! (pspec->flags & GIMP_PARAM_NO_VALIDATE))
        {
          GValue string_value = G_VALUE_INIT;

          g_value_init (&string_value, G_TYPE_STRING);

          if (g_value_type_transformable (arg_type, G_TYPE_STRING))
            g_value_transform (arg, &string_value);
          else
            g_value_set_static_string (&string_value,
                                       "<not transformable to string>");

          if (g_param_value_validate (pspec, arg))
            {
              const gchar *value = g_value_get_string (&string_value);

              if (value == NULL)
                value = "(null)";

              if (return_vals)
                {
                  g_set_error (error,
                               GIMP_PDB_ERROR,
                               GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                               _("Procedure '%s' returned "
                                 "'%s' as return value '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               gimp_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }
              else
                {
                  g_set_error (error,
                               GIMP_PDB_ERROR,
                               GIMP_PDB_ERROR_INVALID_ARGUMENT,
                               _("Procedure '%s' has been called with "
                                 "value '%s' for argument '%s' "
                                 "(#%d, type %s). "
                                 "This value is out of range."),
                               gimp_procedure_get_name (procedure),
                               value,
                               g_param_spec_get_name (pspec),
                               i + 1, g_type_name (spec_type));
                }

              g_value_unset (&string_value);

              return FALSE;
            }

          /*  UTT-8 validate all strings  */
          if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING ||
              (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_BOXED &&
               G_PARAM_SPEC_VALUE_TYPE (pspec) == G_TYPE_STRV))
            {
              gboolean valid = TRUE;

              if (G_PARAM_SPEC_TYPE (pspec) == G_TYPE_PARAM_STRING)
                {
                  const gchar *string = g_value_get_string (arg);

                  if (string)
                    valid = g_utf8_validate (string, -1, NULL);
                }
              else
                {
                  const gchar **strings = g_value_get_boxed (arg);

                  for (const gchar **sp = strings; sp && *sp && valid; sp++)
                    valid = g_utf8_validate (*sp, -1, NULL);
                }

              if (! valid)
                {
                  if (return_vals)
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
                                   _("Procedure '%s' returned an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   gimp_procedure_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }
                  else
                    {
                      g_set_error (error,
                                   GIMP_PDB_ERROR,
                                   GIMP_PDB_ERROR_INVALID_ARGUMENT,
                                   _("Procedure '%s' has been called with an "
                                     "invalid UTF-8 string for argument '%s'."),
                                   gimp_procedure_get_name (procedure),
                                   g_param_spec_get_name (pspec));
                    }

                  g_value_unset (&string_value);

                  return FALSE;
                }
            }

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}

static void
gimp_procedure_set_icon (GimpProcedure *procedure,
                         GimpIconType   icon_type,
                         gconstpointer  icon_data)
{
  GimpProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  if (icon_data == priv->icon_data)
    return;

  switch (priv->icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      g_clear_pointer (&priv->icon_data, g_free);
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_clear_object (&priv->icon_data);
      break;
    }

  priv->icon_type = icon_type;

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      priv->icon_data = g_strdup (icon_data);
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_set_object (&priv->icon_data, (GObject *) icon_data);
      break;

    default:
      g_return_if_reached ();
    }

  if (priv->installed)
    gimp_procedure_install_icon (procedure);
}


/*  internal functions  */

GimpDisplay *
_gimp_procedure_get_display (GimpProcedure *procedure,
                             gint32         display_id)
{
  GimpProcedurePrivate *priv;
  GimpDisplay          *display = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_display_id_is_valid (display_id), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (G_UNLIKELY (! priv->displays))
    priv->displays =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  display = g_hash_table_lookup (priv->displays,
                                 GINT_TO_POINTER (display_id));

  if (! display)
    {
      display = _gimp_plug_in_get_display (priv->plug_in,
                                           display_id);

      if (display)
        g_hash_table_insert (priv->displays,
                             GINT_TO_POINTER (display_id),
                             g_object_ref (display));
    }

  return display;
}

GimpImage *
_gimp_procedure_get_image (GimpProcedure *procedure,
                           gint32         image_id)
{
  GimpProcedurePrivate *priv;
  GimpImage            *image = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_image_id_is_valid (image_id), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (G_UNLIKELY (! priv->images))
    priv->images =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  image = g_hash_table_lookup (priv->images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    {
      image = _gimp_plug_in_get_image (priv->plug_in,
                                       image_id);

      if (image)
        g_hash_table_insert (priv->images,
                             GINT_TO_POINTER (image_id),
                             g_object_ref (image));
    }

  return image;
}

GimpItem *
_gimp_procedure_get_item (GimpProcedure *procedure,
                          gint32         item_id)
{
  GimpProcedurePrivate *priv;
  GimpItem             *item = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_item_id_is_valid (item_id), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (G_UNLIKELY (! priv->items))
    priv->items =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  item = g_hash_table_lookup (priv->items,
                              GINT_TO_POINTER (item_id));

  if (! item)
    {
      item = _gimp_plug_in_get_item (priv->plug_in,
                                     item_id);

      if (item)
        g_hash_table_insert (priv->items,
                             GINT_TO_POINTER (item_id),
                             g_object_ref (item));
    }

  return item;
}

GimpResource *
_gimp_procedure_get_resource (GimpProcedure *procedure,
                              gint32         resource_id)
{
  GimpProcedurePrivate *priv;
  GimpResource         *resource = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_resource_id_is_valid (resource_id), NULL);

  priv = gimp_procedure_get_instance_private (procedure);

  if (G_UNLIKELY (! priv->resources))
    priv->resources =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  resource = g_hash_table_lookup (priv->resources,
                                  GINT_TO_POINTER (resource_id));

  if (! resource)
    {
      resource = _gimp_plug_in_get_resource (priv->plug_in,
                                             resource_id);

      if (resource)
        g_hash_table_insert (priv->resources,
                             GINT_TO_POINTER (resource_id),
                             g_object_ref (resource));
    }

  return resource;
}

void
_gimp_procedure_destroy_proxies (GimpProcedure *procedure)
{
  GimpProcedurePrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  priv = gimp_procedure_get_instance_private (procedure);

  g_clear_pointer (&priv->displays,  g_hash_table_unref);
  g_clear_pointer (&priv->images,    g_hash_table_unref);
  g_clear_pointer (&priv->items,     g_hash_table_unref);
  g_clear_pointer (&priv->resources, g_hash_table_unref);
}
