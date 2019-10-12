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

#include "gimp.h"

#include "libgimpbase/gimpprotocol.h"
#include "libgimpbase/gimpwire.h"

#include "gimpgpparams.h"
#include "gimppdb-private.h"
#include "gimpplugin-private.h"
#include "gimppdb_pdb.h"
#include "gimpplugin_pdb.h"
#include "gimpprocedure-private.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


enum
{
  PROP_0,
  PROP_PLUG_IN,
  PROP_NAME,
  PROP_PROCEDURE_TYPE,
  N_PROPS
};


struct _GimpProcedurePrivate
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
};


static void       gimp_procedure_constructed    (GObject              *object);
static void       gimp_procedure_finalize       (GObject              *object);
static void       gimp_procedure_set_property   (GObject              *object,
                                                 guint                 property_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void       gimp_procedure_get_property   (GObject              *object,
                                                 guint                 property_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);

static void       gimp_procedure_real_install   (GimpProcedure        *procedure);
static void       gimp_procedure_real_uninstall (GimpProcedure        *procedure);
static GimpValueArray *
                  gimp_procedure_real_run       (GimpProcedure        *procedure,
                                                 const GimpValueArray *args);
static GimpProcedureConfig *
                  gimp_procedure_real_create_config
                                                (GimpProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);

static gboolean   gimp_procedure_validate_args  (GimpProcedure        *procedure,
                                                 GParamSpec          **param_specs,
                                                 gint                  n_param_specs,
                                                 GimpValueArray       *args,
                                                 gboolean              return_vals,
                                                 GError              **error);

static void       gimp_procedure_set_icon       (GimpProcedure        *procedure,
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
  procedure->priv = gimp_procedure_get_instance_private (procedure);
}

static void
gimp_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_PLUG_IN (procedure->priv->plug_in));
  g_assert (procedure->priv->name != NULL);
}

static void
gimp_procedure_finalize (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);
  gint           i;

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  g_clear_object  (&procedure->priv->plug_in);

  g_clear_pointer (&procedure->priv->name,        g_free);
  g_clear_pointer (&procedure->priv->image_types, g_free);
  g_clear_pointer (&procedure->priv->menu_label,  g_free);
  g_clear_pointer (&procedure->priv->blurb,       g_free);
  g_clear_pointer (&procedure->priv->help,        g_free);
  g_clear_pointer (&procedure->priv->help_id,     g_free);
  g_clear_pointer (&procedure->priv->authors,     g_free);
  g_clear_pointer (&procedure->priv->copyright,   g_free);
  g_clear_pointer (&procedure->priv->date,        g_free);

  g_list_free_full (procedure->priv->menu_paths, g_free);
  procedure->priv->menu_paths = NULL;

  gimp_procedure_set_icon (procedure, GIMP_ICON_TYPE_ICON_NAME, NULL);

  if (procedure->priv->args)
    {
      for (i = 0; i < procedure->priv->n_args; i++)
        g_param_spec_unref (procedure->priv->args[i]);

      g_clear_pointer (&procedure->priv->args, g_free);
    }

  if (procedure->priv->aux_args)
    {
      for (i = 0; i < procedure->priv->n_aux_args; i++)
        g_param_spec_unref (procedure->priv->aux_args[i]);

      g_clear_pointer (&procedure->priv->aux_args, g_free);
    }

  if (procedure->priv->values)
    {
      for (i = 0; i < procedure->priv->n_values; i++)
        g_param_spec_unref (procedure->priv->values[i]);

      g_clear_pointer (&procedure->priv->values, g_free);
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
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_set_object (&procedure->priv->plug_in, g_value_get_object (value));
      break;

    case PROP_NAME:
      g_free (procedure->priv->name);
      procedure->priv->name = g_value_dup_string (value);
      break;

    case PROP_PROCEDURE_TYPE:
      procedure->priv->proc_type = g_value_get_enum (value);
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
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PLUG_IN:
      g_value_set_object (value, procedure->priv->plug_in);
      break;

    case PROP_NAME:
      g_value_set_string (value, procedure->priv->name);
      break;

    case PROP_PROCEDURE_TYPE:
      g_value_set_enum (value, procedure->priv->proc_type);
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

  icon_type = gimp_procedure_get_icon_type (procedure);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      {
        icon_data = (guint8 *) gimp_procedure_get_icon_name (procedure);
        if (icon_data)
          icon_data_length = strlen ((gchar *) icon_data) + 1;
      }
      break;

    case GIMP_ICON_TYPE_PIXBUF:
      {
        GdkPixbuf *pixbuf = gimp_procedure_get_icon_pixbuf (procedure);

        if (pixbuf)
          gdk_pixbuf_save_to_buffer (pixbuf,
                                     (gchar **) &icon_data, &icon_data_length,
                                     "png", NULL, NULL);
      }
      break;

    case GIMP_ICON_TYPE_IMAGE_FILE:
      {
        GFile *file = gimp_procedure_get_icon_file (procedure);

        if (file)
          {
            icon_data        = (guchar *) g_file_get_uri (file);
            icon_data_length = strlen ((gchar *) icon_data) + 1;
          }
      }
      break;
    }

  if (icon_data)
    _gimp_pdb_set_proc_icon (gimp_procedure_get_name (procedure),
                             icon_type, icon_data_length, icon_data);

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_free (icon_data);
      break;
    }
}

static void
gimp_procedure_real_install (GimpProcedure *procedure)
{
  GParamSpec   **args;
  GParamSpec   **return_vals;
  gint           n_args        = 0;
  gint           n_return_vals = 0;
  GList         *list;
  GimpPlugIn    *plug_in;
  GPProcInstall  proc_install;
  gint           i;

  g_return_if_fail (procedure->priv->installed == FALSE);

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

  if (procedure->priv->image_types)
    {
      _gimp_pdb_set_proc_image_types (gimp_procedure_get_name (procedure),
                                      procedure->priv->image_types);
    }

  if (procedure->priv->menu_label)
    {
      _gimp_pdb_set_proc_menu_label (gimp_procedure_get_name (procedure),
                                     procedure->priv->menu_label);
    }

  for (list = gimp_procedure_get_menu_paths (procedure);
       list;
       list = g_list_next (list))
    {
      _gimp_pdb_add_proc_menu_path (gimp_procedure_get_name (procedure),
                                    list->data);
    }

  if (procedure->priv->blurb ||
      procedure->priv->help  ||
      procedure->priv->help_id)
    {
      _gimp_pdb_set_proc_documentation (gimp_procedure_get_name (procedure),
                                        procedure->priv->blurb,
                                        procedure->priv->help,
                                        procedure->priv->help_id);
    }

  if (procedure->priv->authors   ||
      procedure->priv->copyright ||
      procedure->priv->date)
    {
      _gimp_pdb_set_proc_attribution (gimp_procedure_get_name (procedure),
                                      procedure->priv->authors,
                                      procedure->priv->copyright,
                                      procedure->priv->date);
    }

  procedure->priv->installed = TRUE;
}

static void
gimp_procedure_real_uninstall (GimpProcedure *procedure)
{
  GimpPlugIn      *plug_in;
  GPProcUninstall  proc_uninstall;

  g_return_if_fail (procedure->priv->installed == TRUE);

  proc_uninstall.name = (gchar *) gimp_procedure_get_name (procedure);

  plug_in = gimp_procedure_get_plug_in (procedure);

  if (! gp_proc_uninstall_write (_gimp_plug_in_get_write_channel (plug_in),
                                 &proc_uninstall, plug_in))
    gimp_quit ();

  procedure->priv->installed = FALSE;
}

static GimpValueArray *
gimp_procedure_real_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  return procedure->priv->run_func (procedure, args,
                                    procedure->priv->run_data);
}

static GimpProcedureConfig *
gimp_procedure_real_create_config (GimpProcedure  *procedure,
                                   GParamSpec    **args,
                                   gint            n_args)
{
  gchar *type_name;
  GType  type;

  type_name = g_strdup_printf ("GimpProcedureConfig-%s",
                               gimp_procedure_get_name (procedure));

  type = g_type_from_name (type_name);

  if (! type)
    {
      GParamSpec **config_args;
      gint         n_config_args;

      n_config_args = n_args + procedure->priv->n_aux_args;

      config_args = g_new0 (GParamSpec *, n_config_args);

      memcpy (config_args,
              args,
              n_args * sizeof (GParamSpec *));
      memcpy (config_args + n_args,
              procedure->priv->aux_args,
              procedure->priv->n_aux_args * sizeof (GParamSpec *));

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
 * gimp_procedure_extension_ready(), which means that the procedure
 * has done its initialization, installed its temporary procedures and
 * is ready to run.
 *
 * <emphasis>Not calling gimp_procedure_extension_ready() from a
 * %GIMP_PDB_PROC_TYPE_EXTENSION procedure will cause the GIMP core to
 * lock up.</emphasis>
 *
 * Additionally, a %GIMP_PDB_PROC_TYPE_EXTENSION procedure with no
 * arguments added is an "automatic" extension that will be
 * automatically started on each GIMP startup.
 *
 * %GIMP_PDB_PROC_TYPE_TEMPORARY must be used for temporary procedures
 * that are created during a plug-ins lifetime. They must be added to
 * the #GimpPlugIn using gimp_plug_in_add_temp_procedure().
 *
 * @run_func is called via gimp_procedure_run().
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
gimp_procedure_new (GimpPlugIn      *plug_in,
                    const gchar     *name,
                    GimpPDBProcType  proc_type,
                    GimpRunFunc      run_func,
                    gpointer         run_data,
                    GDestroyNotify   run_data_destroy)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return procedure;
}

/**
 * gimp_procedure_get_plug_in:
 * @procedure: A #GimpProcedure.
 *
 * Returns: (transfer none): The #GimpPlugIn given in gimp_procedure_new().
 *
 * Since: 3.0
 **/
GimpPlugIn *
gimp_procedure_get_plug_in (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->plug_in;
}

/**
 * gimp_procedure_get_name:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's name given in gimp_procedure_new().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_name (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->name;
}

/**
 * gimp_procedure_get_proc_type:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's type given in gimp_procedure_new().
 *
 * Since: 3.0
 **/
GimpPDBProcType
gimp_procedure_get_proc_type (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure),
                        GIMP_PDB_PROC_TYPE_PLUGIN);

  return procedure->priv->proc_type;
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
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  g_free (procedure->priv->image_types);
  procedure->priv->image_types = g_strdup (image_types);

  if (procedure->priv->installed)
    _gimp_pdb_set_proc_image_types (gimp_procedure_get_name (procedure),
                                    procedure->priv->image_types);
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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->image_types;
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
 * For translations of menu labels to work properly, @menu_label
 * should only be marked for translation but passed to this function
 * untranslated, for example using N_("Label"). GIMP will look up the
 * translation in the textdomain registered for the plug-in.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_menu_label (GimpProcedure *procedure,
                               const gchar   *menu_label)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_label != NULL && strlen (menu_label));

  g_clear_pointer (&procedure->priv->menu_label, g_free);
  procedure->priv->menu_label = g_strdup (menu_label);

  if (procedure->priv->installed)
    _gimp_pdb_set_proc_menu_label (gimp_procedure_get_name (procedure),
                                   procedure->priv->menu_label);
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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->menu_label;
}

/**
 * gimp_procedure_add_menu_path:
 * @procedure: A #GimpProcedure.
 * @menu_path: The @procedure's additional menu path.
 *
 * Adds a menu path to the procedure. Only procedures which have a menu
 * label can add a menu path.
 *
 * Menu paths are untranslated paths to menus and submenus with the
 * syntax:
 *
 * &lt;Prefix&gt;/Path/To/Submenu
 *
 * for instance:
 *
 * &lt;Image&gt;/Layer/Transform
 *
 * See also: gimp_plug_in_add_menu_branch().
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_menu_path (GimpProcedure *procedure,
                              const gchar   *menu_path)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (menu_path != NULL);
  g_return_if_fail (procedure->priv->menu_label != NULL);

  procedure->priv->menu_paths = g_list_append (procedure->priv->menu_paths,
                                               g_strdup (menu_path));

  if (procedure->priv->installed)
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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->menu_paths;
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
 * gimp_procedure_get_icon_name().
 *
 * Returns: the #GimpIconType of @procedure's icon.
 *
 * Since: 3.0
 */
GimpIconType
gimp_procedure_get_icon_type (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), -1);

  return procedure->priv->icon_type;
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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  if (procedure->priv->icon_type == GIMP_ICON_TYPE_ICON_NAME)
    return (const gchar *) procedure->priv->icon_data;

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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  if (procedure->priv->icon_type == GIMP_ICON_TYPE_IMAGE_FILE)
    return (GFile *) procedure->priv->icon_data;

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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  if (procedure->priv->icon_type == GIMP_ICON_TYPE_PIXBUF)
    return (GdkPixbuf *) procedure->priv->icon_data;

  return NULL;
}

/**
 * gimp_procedure_set_documentation:
 * @procedure: A #GimpProcedure.
 * @blurb:     The @procedure's blurb.
 * @help:      The @procedure's help text.
 * @help_id:   The @procedure's help ID.
 *
 * @blurb is used as the @procedure's tooltip when represented in the UI,
 * for example as a menu entry.
 *
 * For translations of tooltips to work properly, @blurb should only
 * be marked for translation but passed to this function untranslated,
 * for example using N_("Blurb"). GIMP will look up the translation in
 * the textdomain registered for the plug-in.
 *
 * @help is a free-form text that's meant as documentation for
 * developers of scripts and plug-ins.
 *
 * Sets various documentation strings on @procedure.
 *
 * Since: 3.0
 **/
void
gimp_procedure_set_documentation (GimpProcedure *procedure,
                                  const gchar   *blurb,
                                  const gchar   *help,
                                  const gchar   *help_id)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  g_clear_pointer (&procedure->priv->blurb,   g_free);
  g_clear_pointer (&procedure->priv->help,    g_free);
  g_clear_pointer (&procedure->priv->help_id, g_free);

  procedure->priv->blurb   = g_strdup (blurb);
  procedure->priv->help    = g_strdup (help);
  procedure->priv->help_id = g_strdup (help_id);

  if (procedure->priv->installed)
    _gimp_pdb_set_proc_documentation (gimp_procedure_get_name (procedure),
                                      procedure->priv->blurb,
                                      procedure->priv->help,
                                      procedure->priv->help_id);
}

/**
 * gimp_procedure_get_blurb:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's blurb given in
 *          gimp_procedure_set_documentation().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_blurb (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->blurb;
}

/**
 * gimp_procedure_get_help:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's help text given in
 *          gimp_procedure_set_documentation().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_help (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->help;
}

/**
 * gimp_procedure_get_help_id:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's help ID given in
 *          gimp_procedure_set_documentation().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_help_id (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->help_id;
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
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  g_free (procedure->priv->authors);
  g_free (procedure->priv->copyright);
  g_free (procedure->priv->date);

  procedure->priv->authors   = g_strdup (authors);
  procedure->priv->copyright = g_strdup (copyright);
  procedure->priv->date      = g_strdup (date);

  if (procedure->priv->installed)
    _gimp_pdb_set_proc_attribution (gimp_procedure_get_name (procedure),
                                    procedure->priv->authors,
                                    procedure->priv->copyright,
                                    procedure->priv->date);
}

/**
 * gimp_procedure_get_authors:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's authors given in
 *          gimp_procedure_set_attribution().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_authors (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->authors;
}

/**
 * gimp_procedure_get_copyright:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's copyright given in
 *          gimp_procedure_set_attribution().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_copyright (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->copyright;
}

/**
 * gimp_procedure_get_date:
 * @procedure: A #GimpProcedure.
 *
 * Returns: The procedure's date given in
 *          gimp_procedure_set_attribution().
 *
 * Since: 3.0
 **/
const gchar *
gimp_procedure_get_date (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return procedure->priv->date;
}

/**
 * gimp_procedure_add_argument:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer full): the argument specification.
 *
 * Add a new argument to @procedure according to @pspec specifications.
 * The arguments will be ordered according to the call order to
 * gimp_procedure_add_argument() and
 * gimp_procedure_add_argument_from_property().
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_argument (GimpProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (gimp_is_canonical_identifier (pspec->name));

  if (gimp_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return;
    }

  if (gimp_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return;
    }

  procedure->priv->args = g_renew (GParamSpec *, procedure->priv->args,
                                   procedure->priv->n_args + 1);

  procedure->priv->args[procedure->priv->n_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->n_args++;
}

/**
 * gimp_procedure_add_argument_from_property:
 * @procedure: the #GimpProcedure.
 * @config:    a #GObject.
 * @prop_name: property name in @config.
 *
 * Add a new argument to @procedure according to the specifications of
 * the property @prop_name registered on @config.
 *
 * See gimp_procedure_add_argument() for details.
 *
 * Since: 3.0
 */
void
gimp_procedure_add_argument_from_property (GimpProcedure *procedure,
                                           GObject       *config,
                                           const gchar   *prop_name)
{
  GParamSpec *pspec;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (prop_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), prop_name);

  g_return_if_fail (pspec != NULL);

  gimp_procedure_add_argument (procedure, pspec);
}

/**
 * gimp_procedure_add_aux_argument:
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
 * Since: 3.0
 **/
void
gimp_procedure_add_aux_argument (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (gimp_is_canonical_identifier (pspec->name));

  if (gimp_procedure_find_argument (procedure, pspec->name))
    {
      g_warning ("Argument with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return;
    }

  if (gimp_procedure_find_aux_argument (procedure, pspec->name))
    {
      g_warning ("Auxiliary argument with name '%s' already exists "
                 "on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return;
    }

  procedure->priv->aux_args = g_renew (GParamSpec *, procedure->priv->aux_args,
                                       procedure->priv->n_aux_args + 1);

  procedure->priv->aux_args[procedure->priv->n_aux_args] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->n_aux_args++;
}

/**
 * gimp_procedure_add_aux_argument_from_property:
 * @procedure: the #GimpProcedure.
 * @config:    a #GObject.
 * @prop_name: property name in @config.
 *
 * Add a new auxiliary argument to @procedure according to the
 * specifications of the property @prop_name registered on @config.
 *
 * See gimp_procedure_add_aux_argument() for details.
 *
 * Since: 3.0
 */
void
gimp_procedure_add_aux_argument_from_property (GimpProcedure *procedure,
                                               GObject       *config,
                                               const gchar   *prop_name)
{
  GParamSpec *pspec;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (prop_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), prop_name);

  g_return_if_fail (pspec != NULL);

  gimp_procedure_add_aux_argument (procedure, pspec);
}

/**
 * gimp_procedure_add_return_value:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer full): the return value specification.
 *
 * Add a new return value to @procedure according to @pspec
 * specifications.
 *
 * The returned values will be ordered according to the call order to
 * gimp_procedure_add_return_value() and
 * gimp_procedure_add_return_value_from_property().
 *
 * Since: 3.0
 **/
void
gimp_procedure_add_return_value (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));
  g_return_if_fail (gimp_is_canonical_identifier (pspec->name));

  if (gimp_procedure_find_return_value (procedure, pspec->name))
    {
      g_warning ("Return value with name '%s' already exists on procedure '%s'",
                 pspec->name,
                 gimp_procedure_get_name (procedure));
      return;
    }

  procedure->priv->values = g_renew (GParamSpec *, procedure->priv->values,
                                     procedure->priv->n_values + 1);

  procedure->priv->values[procedure->priv->n_values] = pspec;

  g_param_spec_ref_sink (pspec);

  procedure->priv->n_values++;
}

/**
 * gimp_procedure_add_return_value_from_property:
 * @procedure: the #GimpProcedure.
 * @config:    a #GObject.
 * @prop_name: property name in @config.
 *
 * Add a new return value to @procedure according to the specifications of
 * the property @prop_name registered on @config.
 *
 * The returned values will be ordered according to the call order to
 * gimp_procedure_add_return_value() and
 * gimp_procedure_add_return_value_from_property().
 */
void
gimp_procedure_add_return_value_from_property (GimpProcedure *procedure,
                                               GObject       *config,
                                               const gchar   *prop_name)
{
  GParamSpec *pspec;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_OBJECT (config));
  g_return_if_fail (prop_name != NULL);

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), prop_name);

  g_return_if_fail (pspec != NULL);

  gimp_procedure_add_return_value (procedure, pspec);
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
  gint i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < procedure->priv->n_args; i++)
    if (! strcmp (name, procedure->priv->args[i]->name))
      return procedure->priv->args[i];

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
  gint i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < procedure->priv->n_aux_args; i++)
    if (! strcmp (name, procedure->priv->aux_args[i]->name))
      return procedure->priv->aux_args[i];

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
  gint i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  for (i = 0; i < procedure->priv->n_values; i++)
    if (! strcmp (name, procedure->priv->values[i]->name))
      return procedure->priv->values[i];

  return NULL;
}

/**
 * gimp_procedure_get_arguments:
 * @procedure:   A #GimpProcedure.
 * @n_arguments: (out): Returns the number of arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order added with
 *          gimp_procedure_add_argument().
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_arguments (GimpProcedure *procedure,
                              gint          *n_arguments)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  *n_arguments = procedure->priv->n_args;

  return procedure->priv->args;
}

/**
 * gimp_procedure_get_aux_arguments:
 * @procedure:   A #GimpProcedure.
 * @n_arguments: (out): Returns the number of auxiliary arguments.
 *
 * Returns: (transfer none) (array length=n_arguments): An array
 *          of @GParamSpec in the order added with
 *          gimp_procedure_add_aux_argument().
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_aux_arguments (GimpProcedure *procedure,
                                  gint          *n_arguments)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_arguments != NULL, NULL);

  *n_arguments = procedure->priv->n_aux_args;

  return procedure->priv->aux_args;
}

/**
 * gimp_procedure_get_return_values:
 * @procedure:       A #GimpProcedure.
 * @n_return_values: (out): Returns the number of return values.
 *
 * Returns: (transfer none) (array length=n_return_values): An array
 *          of @GParamSpec in the order added with
 *          gimp_procedure_add_return_value().
 *
 * Since: 3.0
 **/
GParamSpec **
gimp_procedure_get_return_values (GimpProcedure *procedure,
                                  gint          *n_return_values)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (n_return_values != NULL, NULL);

  *n_return_values = procedure->priv->n_values;

  return procedure->priv->values;
}

GimpValueArray *
gimp_procedure_new_arguments (GimpProcedure *procedure)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  args = gimp_value_array_new (procedure->priv->n_args);

  for (i = 0; i < procedure->priv->n_args; i++)
    {
      GParamSpec *pspec = procedure->priv->args[i];

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &value);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

/**
 * gimp_procedure_new_return_values:
 * @procedure: the #GimpProcedure.
 * @status:    the success status of the procedure run.
 * @error:     (in) (nullable) (transfer full):
 *             an optional #GError. This parameter should be set if
 *             @status is either #GIMP_PDB_EXECUTION_ERROR or
 *             #GIMP_PDB_CALLING_ERROR.
 *
 * Format the expected return values from procedures, using the return
 * values set with gimp_procedure_add_return_value().
 *
 * Returns: the expected #GimpValueArray as could be returned by a
 *          #GimpRunFunc.
 *
 * Since: 3.0
 **/
GimpValueArray *
gimp_procedure_new_return_values (GimpProcedure     *procedure,
                                  GimpPDBStatusType  status,
                                  GError            *error)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (status != GIMP_PDB_PASS_THROUGH, NULL);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
    case GIMP_PDB_CANCEL:
      g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

      args = gimp_value_array_new (procedure->priv->n_values + 1);

      g_value_init (&value, GIMP_TYPE_PDB_STATUS_TYPE);
      g_value_set_enum (&value, status);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);

      for (i = 0; i < procedure->priv->n_values; i++)
        {
          GParamSpec *pspec = procedure->priv->values[i];

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
 * gimp_procedure_run:
 * @procedure: a @GimpProcedure.
 * @args:      the @procedure's arguments.
 *
 * Runs the procedure, calling the run_func given in gimp_procedure_new().
 *
 * Returns: (transfer full): The @procedure's return values.
 *
 * Since: 3.0
 **/
GimpValueArray *
gimp_procedure_run (GimpProcedure  *procedure,
                    GimpValueArray *args)
{
  GimpValueArray *return_vals;
  GError         *error = NULL;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  if (! gimp_procedure_validate_args (procedure,
                                      procedure->priv->args,
                                      procedure->priv->n_args,
                                      args, FALSE, &error))
    {
      return_vals = gimp_procedure_new_return_values (procedure,
                                                      GIMP_PDB_CALLING_ERROR,
                                                      error);
      g_clear_error (&error);

      return return_vals;
    }

  /*  add missing args with default values  */
  if (gimp_value_array_length (args) < procedure->priv->n_args)
    {
      GimpProcedureConfig *config;
      GObjectClass        *config_class = NULL;
      GimpValueArray      *complete;

      /*  if saved defaults exist, they override GParamSpec  */
      config = gimp_procedure_create_config (procedure);
      if (gimp_procedure_config_load_default (config, NULL))
        config_class = G_OBJECT_GET_CLASS (config);
      else
        g_clear_object (&config);

      complete = gimp_value_array_new (procedure->priv->n_args);

      for (i = 0; i < procedure->priv->n_args; i++)
        {
          GParamSpec *pspec = procedure->priv->args[i];
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
      g_clear_error (&error);
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
 * This function <emphasis>must</emphasis> be called from every
 * procedure's #GimpRunFunc that was created as
 * #GIMP_PDB_PROC_TYPE_EXTENSION.
 *
 * Subsequently, extensions can process temporary procedure run
 * requests using either gimp_plug_in_extension_enable() or
 * gimp_plug_in_extension_process().
 *
 * See also: gimp_procedure_new().
 *
 * Since: 3.0
 **/
void
gimp_procedure_extension_ready (GimpProcedure *procedure)
{
  GimpPlugIn *plug_in;

  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (procedure->priv->proc_type == GIMP_PDB_PROC_TYPE_EXTENSION);

  plug_in = gimp_procedure_get_plug_in (procedure);

  if (! gp_extension_ack_write (_gimp_plug_in_get_write_channel (plug_in),
                                plug_in))
    gimp_quit ();
}

/**
 * gimp_procedure_create_config:
 * @procedure: A #GimpProcedure
 *
 * Create a #GimpConfig with properties that match @procedure's arguments.
 *
 * Returns: (transfer full): The new #GimpConfig.
 *
 * Since: 3.0
 **/
GimpProcedureConfig *
gimp_procedure_create_config (GimpProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);

  return GIMP_PROCEDURE_GET_CLASS (procedure)->create_config (procedure,
                                                              procedure->priv->args,
                                                              procedure->priv->n_args);
}


/*  private functions  */

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

      if (arg_type != spec_type)
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
              G_PARAM_SPEC_TYPE (pspec) == GIMP_TYPE_PARAM_STRING_ARRAY)
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
                  const GimpArray *array = g_value_get_boxed (arg);

                  if (array)
                    {
                      const gchar **strings = (const gchar **) array->data;
                      gint          i;

                      for (i = 0; i < array->length && valid; i++)
                        if (strings[i])
                          valid = g_utf8_validate (strings[i], -1, NULL);
                    }
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
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  if (icon_data == procedure->priv->icon_data)
    return;

  switch (procedure->priv->icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      g_clear_pointer (&procedure->priv->icon_data, g_free);
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_clear_object (&procedure->priv->icon_data);
      break;
    }

  procedure->priv->icon_type = icon_type;

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
      procedure->priv->icon_data = g_strdup (icon_data);
      break;

    case GIMP_ICON_TYPE_PIXBUF:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      g_set_object (&procedure->priv->icon_data, (GObject *) icon_data);
      break;

    default:
      g_return_if_reached ();
    }

  if (procedure->priv->installed)
    gimp_procedure_install_icon (procedure);
}


/*  internal functions  */

GimpDisplay *
_gimp_procedure_get_display (GimpProcedure *procedure,
                             gint32         display_id)
{
  GimpDisplay *display = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_display_id_is_valid (display_id), NULL);

  if (G_UNLIKELY (! procedure->priv->displays))
    procedure->priv->displays =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  display = g_hash_table_lookup (procedure->priv->displays,
                                 GINT_TO_POINTER (display_id));

  if (! display)
    {
      display = _gimp_plug_in_get_display (procedure->priv->plug_in,
                                           display_id);

      if (display)
        g_hash_table_insert (procedure->priv->displays,
                             GINT_TO_POINTER (display_id),
                             g_object_ref (display));
    }

  return display;
}

GimpImage *
_gimp_procedure_get_image (GimpProcedure *procedure,
                           gint32         image_id)
{
  GimpImage *image = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_image_id_is_valid (image_id), NULL);

  if (G_UNLIKELY (! procedure->priv->images))
    procedure->priv->images =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  image = g_hash_table_lookup (procedure->priv->images,
                               GINT_TO_POINTER (image_id));

  if (! image)
    {
      image = _gimp_plug_in_get_image (procedure->priv->plug_in,
                                       image_id);

      if (image)
        g_hash_table_insert (procedure->priv->images,
                             GINT_TO_POINTER (image_id),
                             g_object_ref (image));
    }

  return image;
}

GimpItem *
_gimp_procedure_get_item (GimpProcedure *procedure,
                          gint32         item_id)
{
  GimpItem *item = NULL;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (gimp_item_id_is_valid (item_id), NULL);

  if (G_UNLIKELY (! procedure->priv->items))
    procedure->priv->items =
      g_hash_table_new_full (g_direct_hash,
                             g_direct_equal,
                             NULL,
                             (GDestroyNotify) g_object_unref);

  item = g_hash_table_lookup (procedure->priv->items,
                              GINT_TO_POINTER (item_id));

  if (! item)
    {
      item = _gimp_plug_in_get_item (procedure->priv->plug_in,
                                     item_id);

      if (item)
        g_hash_table_insert (procedure->priv->items,
                             GINT_TO_POINTER (item_id),
                             g_object_ref (item));
    }

  return item;
}

void
_gimp_procedure_destroy_proxies (GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));

  g_clear_pointer (&procedure->priv->displays, g_hash_table_unref);
  g_clear_pointer (&procedure->priv->images,   g_hash_table_unref);
  g_clear_pointer (&procedure->priv->items,    g_hash_table_unref);
}
