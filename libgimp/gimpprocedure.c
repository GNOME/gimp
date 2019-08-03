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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "gimp.h"
#include "gimpprocedure-private.h"

#include "libgimp-intl.h"


typedef enum
{
  GIMP_PDB_ERROR_FAILED,  /* generic error condition */
  GIMP_PDB_ERROR_CANCELLED,
  GIMP_PDB_ERROR_PROCEDURE_NOT_FOUND,
  GIMP_PDB_ERROR_INVALID_ARGUMENT,
  GIMP_PDB_ERROR_INVALID_RETURN_VALUE,
  GIMP_PDB_ERROR_INTERNAL_ERROR
} GimpPdbErrorCode;

#define GIMP_PDB_ERROR (gimp_pdb_error_quark ())

static GQuark
gimp_pdb_error_quark (void)
{
  return g_quark_from_static_string ("gimp-pdb-error-quark");
}


struct _GimpProcedurePrivate
{
  GimpPlugIn       *plug_in;        /* the procedure's plug-in        */
  GimpPDBProcType   proc_type;      /* procedure type                 */

  gchar            *name;           /* procedure name                 */
  gchar            *menu_label;
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;
  gchar            *authors;        /* Authors field                  */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */
  gchar            *image_types;

  GimpIconType      icon_type;
  guint8           *icon_data;
  gint              icon_data_length;

  GList            *menu_paths;

  gint32            n_args;         /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            n_values;       /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  GimpRunFunc       run_func;
  gpointer          run_data;
  GDestroyNotify    run_data_destroy;
};


static void       gimp_procedure_finalize      (GObject         *object);

static gboolean   gimp_procedure_validate_args (GimpProcedure   *procedure,
                                                GParamSpec     **param_specs,
                                                gint             n_param_specs,
                                                GimpValueArray  *args,
                                                gboolean         return_vals,
                                                GError         **error);


G_DEFINE_TYPE_WITH_PRIVATE (GimpProcedure, gimp_procedure, G_TYPE_OBJECT)

#define parent_class gimp_procedure_parent_class


static void
gimp_procedure_class_init (GimpProcedureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = gimp_procedure_finalize;
}

static void
gimp_procedure_init (GimpProcedure *procedure)
{
  procedure->priv = gimp_procedure_get_instance_private (procedure);
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
  g_clear_pointer (&procedure->priv->menu_label,  g_free);
  g_clear_pointer (&procedure->priv->blurb,       g_free);
  g_clear_pointer (&procedure->priv->help,        g_free);
  g_clear_pointer (&procedure->priv->help_id,     g_free);
  g_clear_pointer (&procedure->priv->authors,     g_free);
  g_clear_pointer (&procedure->priv->copyright,   g_free);
  g_clear_pointer (&procedure->priv->date,        g_free);
  g_clear_pointer (&procedure->priv->image_types, g_free);

  g_clear_pointer (&procedure->priv->icon_data, g_free);
  procedure->priv->icon_data_length = 0;

  g_list_free_full (procedure->priv->menu_paths, g_free);
  procedure->priv->menu_paths = NULL;

  if (procedure->priv->args)
    {
      for (i = 0; i < procedure->priv->n_args; i++)
        g_param_spec_unref (procedure->priv->args[i]);

      g_clear_pointer (&procedure->priv->args, g_free);
    }

  if (procedure->priv->values)
    {
      for (i = 0; i < procedure->priv->n_values; i++)
        g_param_spec_unref (procedure->priv->values[i]);

      g_clear_pointer (&procedure->priv->values, g_free);
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
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
 * @proc_type should be %GIMP_PLUGIN for "normal" plug-ins.
 *
 * Using %GIMP_EXTENSION means that the plug-in will add temporary
 * procedures. Therefore, the GIMP core will wait until the
 * %GIMP_EXTENSION procedure has called
 * gimp_procedure_extension_ready(), which means that the procedure
 * has done its initialization, installed its temporary procedures and
 * is ready to run.
 *
 * <emphasis>Not calling gimp_procedure_extension_reads() from a
 * %GIMP_EXTENSION procedure will cause the GIMP core to lock
 * up.</emphasis>
 *
 * Additionally, a %GIMP_EXTENSION procedure with no arguments added
 * is an "automatic" extension that will be automatically started on
 * each GIMP startup.
 *
 * %GIMP_TEMPORARY must be used for temporary procedures that are
 * created during a plug-ins lifetime. They must be added to the
 * #GimpPlugIn using gimp_plug_in_add_temp_procedure().
 *
 * @run_func is called via gimp_procedure_run().
 *
 * For %GIMP_PLUGIN and %GIMP_EXTENSION procedures the call of
 * @run_func is basically the lifetime of the plug-in.
 *
 * Returns: a new #GimpProcedure.
 **/
GimpProcedure  *
gimp_procedure_new (GimpPlugIn      *plug_in,
                    const gchar     *name,
                    GimpPDBProcType  proc_type,
                    GimpRunFunc      run_func,
                    gpointer         run_data,
                    GDestroyNotify   run_data_destroy)
{
  GimpProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (proc_type != GIMP_INTERNAL, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_PROCEDURE, NULL);

  procedure->priv->plug_in           = g_object_ref (plug_in);
  procedure->priv->proc_type         = proc_type;
  procedure->priv->name              = g_strdup (name);
  procedure->priv->run_func          = run_func;
  procedure->priv->run_data          = run_data;
  procedure->priv->run_data_destroy  = run_data_destroy;

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
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), GIMP_PLUGIN);

  return procedure->priv->proc_type;
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

  g_clear_pointer (&procedure->priv->menu_label, g_free);
  procedure->priv->menu_label = g_strdup (menu_label);
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
}

/**
 * gimp_procedure_get_author:
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
 * gimp_procedure_set_image_types:
 * @image_types the image types this procedure can operate on.
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
}

/**
 * gimp_procedure_get_image_types:
 *
 * This procedure retrieves the list of image types the procedure can
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

void
gimp_procedure_set_icon (GimpProcedure *procedure,
                         GimpIconType   icon_type,
                         const guint8  *icon_data)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (icon_data != NULL);
  g_return_if_fail (icon_data != procedure->priv->icon_data);

  g_clear_pointer (&procedure->priv->icon_data, g_free);
  procedure->priv->icon_data_length = 0;

  procedure->priv->icon_type = icon_type;

  switch (icon_type)
    {
    case GIMP_ICON_TYPE_ICON_NAME:
    case GIMP_ICON_TYPE_IMAGE_FILE:
      procedure->priv->icon_data =
        (guint8 *) g_strdup ((const gchar *) icon_data);

      procedure->priv->icon_data_length =
        strlen ((const gchar *) icon_data);
      break;

    case GIMP_ICON_TYPE_INLINE_PIXBUF:
      g_return_if_fail (g_ntohl (*((gint32 *) icon_data)) == 0x47646b50);

      procedure->priv->icon_data_length =
        g_ntohl (*((gint32 *) (icon_data + 4)));

      procedure->priv->icon_data =
        g_memdup (icon_data, procedure->priv->icon_data_length);
      break;

    default:
      g_return_if_reached ();
    }
}

GimpIconType
gimp_procedure_get_icon (GimpProcedure  *procedure,
                         const guint8  **icon_data,
                         gint           *icon_data_length)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), -1);
  g_return_val_if_fail (icon_data != NULL, -1);
  g_return_val_if_fail (icon_data_length != NULL, -1);

  *icon_data        = (guint8 *) procedure->priv->icon_data;
  *icon_data_length = procedure->priv->icon_data_length;

  return procedure->priv->icon_type;
}

/**
 * gimp_procedure_add_menu_path:
 * @procedure: A #GimpProcedure.
 * @menu_path: The @procedure's additional menu path.
 *
 * Adds a menu path to te procedure. Only procedures which have a menu
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
 * gimp_procedure_add_argument:
 * @procedure: the #GimpProcedure.
 * @pspec:     (transfer full): the argument specification.
 *
 * Add a new argument to @procedure according to @pspec specifications.
 * The arguments will be ordered according to the call order to
 * gimp_procedure_add_argument() and
 * gimp_procedure_add_argument_from_property().
 **/
void
gimp_procedure_add_argument (GimpProcedure *procedure,
                             GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

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
 * The arguments will be ordered according to the call order to
 * gimp_procedure_add_argument() and
 * gimp_procedure_add_argument_from_property().
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
 **/
void
gimp_procedure_add_return_value (GimpProcedure *procedure,
                                 GParamSpec    *pspec)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (G_IS_PARAM_SPEC (pspec));

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
 **/
GimpValueArray *
gimp_procedure_new_return_values (GimpProcedure     *procedure,
                                  GimpPDBStatusType  status,
                                  GError            *error)
{
  GimpValueArray *args;
  GValue          value = G_VALUE_INIT;
  gint            i;

  g_return_val_if_fail (GIMP_IS_PROCEDURE (procedure), NULL);
  g_return_val_if_fail (status != GIMP_PDB_PASS_THROUGH, NULL);

  switch (status)
    {
    case GIMP_PDB_SUCCESS:
    case GIMP_PDB_CANCEL:
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
  for (i = gimp_value_array_length (args); i < procedure->priv->n_args; i++)
    {
      GParamSpec *pspec = procedure->priv->args[i];
      GValue      value = G_VALUE_INIT;

      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
      g_param_value_set_default (pspec, &value);
      gimp_value_array_append (args, &value);
      g_value_unset (&value);
    }

  /*  call the procedure  */
  return_vals = procedure->priv->run_func (procedure, args,
                                           procedure->priv->run_data);

  if (! return_vals)
    {
      g_warning ("%s: no return values, shouldn't happen", G_STRFUNC);

      error = g_error_new (0, 0, 0,
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
 *
 * Notify the main GIMP application that the extension has been
 * properly initialized and is ready to run.
 *
 * This function <emphasis>must</emphasis> be called from every
 * procedure's #GimpRunFunc that was created as #GIMP_EXTENSION.
 *
 * Subsequently, extensions can process temporary procedure run
 * requests using either gimp_extension_enable() or
 * gimp_extension_process().
 *
 * See also: gimp_procedure_new().
 *
 * Since: 3.0
 **/
void
gimp_procedure_extension_ready (GimpProcedure *procedure)
{
  g_return_if_fail (GIMP_IS_PROCEDURE (procedure));
  g_return_if_fail (procedure->priv->proc_type == GIMP_EXTENSION);

  _gimp_procedure_extension_ready (procedure);
}


/*  private functions  */

static gboolean
gimp_procedure_validate_args (GimpProcedure  *procedure,
                              GParamSpec    **param_specs,
                              gint            n_param_specs,
                              GimpValueArray *args,
                              gboolean        return_vals,
                              GError        **error)
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
      else /* if (! (pspec->flags & GIMP_PARAM_NO_VALIDATE)) */
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

          g_value_unset (&string_value);
        }
    }

  return TRUE;
}
