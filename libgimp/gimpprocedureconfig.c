/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * gimpprocedureconfig.c
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

#include "config.h"

#include "gimp.h"


/**
 * SECTION: gimpprocedureconfig
 * @title: GimpProcedureConfig
 * @short_description: Config object for procedure arguments
 *
 * #GimpProcedureConfig base class for #GimpProcedure-specific config
 * objects and the main interface to manage aspects of
 * #GimpProcedure's arguments such as persistency of the last used
 * arguments across GIMP sessions.
 *
 * A #GimpProcedureConfig is created by a #GimpProcedure using
 * gimp_procedure_create_config() and its properties match the
 * procedure's arguments in number, order and type.
 *
 * It implements the #GimpConfig interface and therefore has all its
 * serialization and deserialization features.
 *
 * Since: 3.0
 **/


enum
{
  PROP_0,
  PROP_PROCEDURE,
  N_PROPS
};


struct _GimpProcedureConfigPrivate
{
  GimpProcedure *procedure;
};


static void   gimp_procedure_config_constructed   (GObject              *object);
static void   gimp_procedure_config_dispose       (GObject              *object);
static void   gimp_procedure_config_set_property  (GObject              *object,
                                                   guint                 property_id,
                                                   const GValue         *value,
                                                   GParamSpec           *pspec);
static void   gimp_procedure_config_get_property  (GObject              *object,
                                                   guint                 property_id,
                                                   GValue               *value,
                                                   GParamSpec           *pspec);

static GFile    * gimp_procedure_config_get_file  (GimpProcedureConfig *config,
                                                   const gchar         *extension);
static gboolean   gimp_procedure_config_load_last (GimpProcedureConfig  *config,
                                                   GError              **error);
static gboolean   gimp_procedure_config_save_last (GimpProcedureConfig  *config,
                                                   GError              **error);



G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpProcedureConfig, gimp_procedure_config,
                                     G_TYPE_OBJECT)

#define parent_class gimp_procedure_config_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
gimp_procedure_config_class_init (GimpProcedureConfigClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructed  = gimp_procedure_config_constructed;
  object_class->dispose      = gimp_procedure_config_dispose;
  object_class->set_property = gimp_procedure_config_set_property;
  object_class->get_property = gimp_procedure_config_get_property;

  props[PROP_PROCEDURE] =
    g_param_spec_object ("procedure",
                         "Procedure",
                         "The procedure this config object is used for",
                         GIMP_TYPE_PROCEDURE,
                         GIMP_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_procedure_config_init (GimpProcedureConfig *config)
{
  config->priv = gimp_procedure_config_get_instance_private (config);
}

static void
gimp_procedure_config_constructed (GObject *object)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_PROCEDURE (config->priv->procedure));
}

static void
gimp_procedure_config_dispose (GObject *object)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (object);

  g_clear_object (&config->priv->procedure);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_procedure_config_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      config->priv->procedure = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_procedure_config_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (object);

  switch (property_id)
    {
    case PROP_PROCEDURE:
      g_value_set_object (value, config->priv->procedure);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * gimp_procedure_config_get_procedure:
 * @config: a #GimpProcedureConfig
 *
 * This function returns the #GimpProcedure which created @config, see
 * gimp_procedure_create_config().
 *
 * Returns: The #GimpProcedure which created @config.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_procedure_config_get_procedure (GimpProcedureConfig *config)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);

  return config->priv->procedure;
}

/**
 * gimp_procedure_config_set_values:
 * @config: a #GimpProcedureConfig
 * @values: a #GimpValueArray
 *
 * Sets the values from @values on @config's properties.
 *
 * The number, order and types of values in @values must match the
 * number, order and types of @config's properties.
 *
 * This function is meant to be used on @values which are passed as
 * arguments to the run() function of the #GimpProcedure which created
 * this @config. See gimp_procedure_create_config().
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_set_values (GimpProcedureConfig  *config,
                                  const GimpValueArray *values)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);

  g_return_if_fail (n_pspecs == gimp_value_array_length (values));

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue     *value = gimp_value_array_index (values, i);

      g_object_set_property (G_OBJECT (config), pspec->name, value);
    }

  g_free (pspecs);
}

/**
 * gimp_procedure_config_get_values:
 * @config: a #GimpProcedureConfig
 * @values: a #GimpValueArray
 *
 * Gets the values from @config's properties and stores them in
 * @values.
 *
 * See gimp_procedure_config_set_values().
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_get_values (GimpProcedureConfig  *config,
                                  GimpValueArray       *values)
{
  GParamSpec **pspecs;
  guint        n_pspecs;
  gint         i;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);

  g_return_if_fail (n_pspecs == gimp_value_array_length (values));

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];
      GValue     *value = gimp_value_array_index (values, i);

      g_object_get_property (G_OBJECT (config), pspec->name, value);
    }

  g_free (pspecs);
}

/**
 * gimp_procedure_config_begin_run:
 * @config:   a #GimpProcedureConfig
 * @run_mode: the #GimpRunMode passed to a #GimpProcedure's run()
 * @args:     the #GimpValueArray passed to a #GimpProcedure's run()
 *
 * Populates @config with values for a #GimpProcedure's run(),
 * depending on @run_mode.
 *
 * If @run_mode is %GIMP_RUN_INTERACTIVE or %GIMP_RUN_WITH_LAST_VALS,
 * the saved values from the procedure's last run() are loaded and set
 * on @config. If no saved last values are found, the procedure's
 * default argument values are used.
 *
 * If @run_mode is %GIMP_RUN_NONINTERACTIVE, the contents of @args are
 * set on @config using gimp_procedure_config_set_values().
 *
 * After calling this function, the @args passed to run() should be
 * left alone and @config be treated as the procedure's arguments.
 *
 * It is possible to get @config's resulting values back into @args by
 * calling gimp_procedure_config_get_values(), as long as modified
 * @args are written back to @config using
 * gimp_procedure_config_set_values() before the call to
 * gimp_procedure_config_end_run().
 *
 * This function should be used at the beginning of a procedure's
 * run() and be paired with a call to gimp_procedure_config_end_run()
 * at the end of run().
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_begin_run (GimpProcedureConfig  *config,
                                 GimpRunMode           run_mode,
                                 const GimpValueArray *args)
{
  GError *error = NULL;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (args != NULL);

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE :
    case GIMP_RUN_WITH_LAST_VALS:
      if (! gimp_procedure_config_load_last (config, &error))
        {
          g_printerr ("Saving last values failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }
      break;

    case GIMP_RUN_NONINTERACTIVE:
      gimp_procedure_config_set_values (config, args);
      break;
    }
}

/**
 * gimp_procedure_config_end_run:
 * @config:   a #GimpProcedureConfig
 * @run_mode: the #GimpRunMode passed to a #GimpProcedure's run()
 *
 * This function is the counterpart of
 * gimp_procedure_conig_begin_run() and should be used upon successful
 * completion of a procedure's run(), before returning
 * %GIMP_PDB_SUCCESS return values.
 *
 * If @run_mode is %GIMP_RUN_INTERACTIVE, @config is saved as last
 * values to be used when the procedure runs again.
 *
 * If @run_mode is not %GIMP_RUN_NONINTERACTIVE, this function also
 * conveniently calls gimp_display_flush(), which is what most
 * procedures want and doesn't do any harm if called redundantly.
 *
 * See gimp_procedure_config_begin_run().
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_end_run (GimpProcedureConfig  *config,
                               GimpRunMode           run_mode)
{
  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));

  if (run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  if (run_mode == GIMP_RUN_INTERACTIVE)
    {
      GError *error = NULL;

      if (! gimp_procedure_config_save_last (config, &error))
        {
          g_printerr ("Saving last values failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }
    }
}


/*  private functions  */

static GFile *
gimp_procedure_config_get_file (GimpProcedureConfig *config,
                                const gchar         *extension)
{
  GFile *file;
  gchar *basename;

  basename = g_strconcat (G_OBJECT_TYPE_NAME (config), extension, NULL);
  file = gimp_directory_file ("plug-in-settings", basename, NULL);
  g_free (basename);

  return file;
}

static gboolean
gimp_procedure_config_load_last (GimpProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file = gimp_procedure_config_get_file (config, ".last");
  gboolean  success;

  success = gimp_config_deserialize_file (GIMP_CONFIG (config),
                                          file,
                                          NULL, error);

  if (! success && (*error)->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (error);
      success = TRUE;
    }

  g_object_unref (file);

  return TRUE;
}

static gboolean
gimp_procedure_config_save_last (GimpProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file = gimp_procedure_config_get_file (config, ".last");
  gboolean  success;

  success = gimp_config_serialize_to_file (GIMP_CONFIG (config),
                                           file,
                                           "settings",
                                           "end of settings",
                                           NULL, error);

  g_object_unref (file);

  return success;
}
