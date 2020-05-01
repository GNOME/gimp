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
#include "gimpimagemetadata.h"

#include "gimpprocedureconfig-private.h"


/**
 * SECTION: gimpprocedureconfig
 * @title: GimpProcedureConfig
 * @short_description: Config object for procedure arguments
 *
 * #GimpProcedureConfig is the base class for #GimpProcedure specific
 * config objects and the main interface to manage aspects of
 * #GimpProcedure's arguments such as persistency of the last used
 * arguments across GIMP sessions.
 *
 * A #GimpProcedureConfig is created by a #GimpProcedure using
 * gimp_procedure_create_config() and its properties match the
 * procedure's arguments and auxiliary arguments in number, order and
 * type.
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
  GimpProcedure         *procedure;

  GimpImage             *image;
  GimpRunMode            run_mode;

  GimpMetadata          *metadata;
  gchar                 *mime_type;
  GimpMetadataSaveFlags  metadata_flags;
};


static void   gimp_procedure_config_constructed   (GObject      *object);
static void   gimp_procedure_config_dispose       (GObject      *object);
static void   gimp_procedure_config_set_property  (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static void   gimp_procedure_config_get_property  (GObject      *object,
                                                   guint         property_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);


G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GimpProcedureConfig, gimp_procedure_config,
                                     G_TYPE_OBJECT)

#define parent_class gimp_procedure_config_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static const struct
{
  const gchar           *name;
  GimpMetadataSaveFlags  flag;
}
metadata_properties[] =
{
  { "save-exif",          GIMP_METADATA_SAVE_EXIF          },
  { "save-xmp",           GIMP_METADATA_SAVE_XMP           },
  { "save-iptc",          GIMP_METADATA_SAVE_IPTC          },
  { "save-thumbnail",     GIMP_METADATA_SAVE_THUMBNAIL     },
  { "save-color-profile", GIMP_METADATA_SAVE_COLOR_PROFILE },
  { "save-comment",       GIMP_METADATA_SAVE_COMMENT       }
};


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

  config->priv->run_mode = -1;
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
  g_clear_object (&config->priv->metadata);
  g_clear_pointer (&config->priv->mime_type, g_free);

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
 * Returns: (transfer none): The #GimpProcedure which created @config.
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
  gint         n_aux_args;
  gint         n_values;
  gint         i;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  gimp_procedure_get_aux_arguments (config->priv->procedure, &n_aux_args);
  n_values = gimp_value_array_length (values);

  /* The first property is the procedure, all others are arguments. */
  g_return_if_fail (n_pspecs == n_values + n_aux_args + 1);

  for (i = 0; i < n_values; i++)
    {
      GParamSpec *pspec = pspecs[i + 1];
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
  gint         n_aux_args;
  gint         n_values;
  gint         i;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (values != NULL);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);
  gimp_procedure_get_aux_arguments (config->priv->procedure, &n_aux_args);
  n_values = gimp_value_array_length (values);

  g_return_if_fail (n_pspecs == n_values + n_aux_args);

  for (i = 0; i < n_values; i++)
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
 * @image: (nullable): a #GimpImage or %NULL
 * @run_mode: the #GimpRunMode passed to a #GimpProcedure's run()
 * @args:     the #GimpValueArray passed to a #GimpProcedure's run()
 *
 * Populates @config with values for a #GimpProcedure's run(),
 * depending on @run_mode.
 *
 * If @run_mode is %GIMP_RUN_INTERACTIVE or %GIMP_RUN_WITH_LAST_VALS,
 * the saved values from the procedure's last run() are loaded and set
 * on @config. If @image is not %NULL, the last used values for this
 * image are tried first, and if no image-spesicic values are found
 * the globally saved last used values are used. If no saved last used
 * values are found, the procedure's default argument values are used.
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
                                 GimpImage            *image,
                                 GimpRunMode           run_mode,
                                 const GimpValueArray *args)
{
  gboolean  loaded = FALSE;
  GError   *error  = NULL;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));
  g_return_if_fail (args != NULL);

  config->priv->image    = image;
  config->priv->run_mode = run_mode;

  switch (run_mode)
    {
    case GIMP_RUN_INTERACTIVE :
    case GIMP_RUN_WITH_LAST_VALS:
      if (image)
        {
          loaded = gimp_procedure_config_load_parasite (config, image,
                                                        &error);
          if (! loaded && error)
            {
              g_printerr ("Loading last used values from parasite failed: %s\n",
                          error->message);
              g_clear_error (&error);
            }
        }

      if (! loaded &&
          ! gimp_procedure_config_load_last (config, &error) && error)
        {
          g_printerr ("Loading last used values from disk failed: %s\n",
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
 * @config: a #GimpProcedureConfig
 * @status: the return status of the #GimpProcedure's run()
 *
 * This function is the counterpart of
 * gimp_procedure_config_begin_run() and must always be called in
 * pairs in a procedure's run(), before returning return values.
 *
 * If the @run_mode passed to gimp_procedure_config_end_run() was
 * %GIMP_RUN_INTERACTIVE, @config is saved as last used values to be
 * used when the procedure runs again. Additionally, the @image passed
 * gimp_procedure_config_begin_run() was not %NULL, @config is
 * attached to @image as last used values for this image using a
 * #GimpParasite and gimp_image_attach_parasite().
 *
 * If @run_mode was not %GIMP_RUN_NONINTERACTIVE, this function also
 * conveniently calls gimp_displays_flush(), which is what most
 * procedures want and doesn't do any harm if called redundantly.
 *
 * See gimp_procedure_config_begin_run().
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_end_run (GimpProcedureConfig *config,
                               GimpPDBStatusType    status)
{
  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));

  if (config->priv->run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  if (status                 == GIMP_PDB_SUCCESS &&
      config->priv->run_mode == GIMP_RUN_INTERACTIVE)
    {
      GError *error = NULL;

      if (config->priv->image)
        gimp_procedure_config_save_parasite (config, config->priv->image,
                                             NULL);

      if (! gimp_procedure_config_save_last (config, &error))
        {
          g_printerr ("Saving last used values to disk failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }
    }

  config->priv->image    = NULL;
  config->priv->run_mode = -1;
}

/**
 * gimp_procedure_config_begin_export:
 * @config:         a #GimpProcedureConfig
 * @original_image: the #GimpImage passed to run()
 * @run_mode:       the #GimpRunMode passed to a #GimpProcedure's run()
 * @args:           the #GimpValueArray passed to a #GimpProcedure's run()
 * @mime_type: (nullable):  exported file format's mime type, or %NULL.
 *
 * This is a variant of gimp_procedure_config_begin_run() to be used
 * by file export procedures using #GimpSaveProcedure. It must be
 * paired with a call to gimp_procedure_config_end_export() at the end
 * of run().
 *
 * It does everything gimp_procedure_config_begin_run() does but
 * provides additional features to automate file export:
 *
 * If @mime_type is non-%NULL, exporting metadata is handled
 * automatically, by calling gimp_image_metadata_save_prepare() and
 * syncing its returned #GimpMetadataSaveFlags with @config's
 * properties. (The corresponding gimp_image_metadata_save_finish()
 * will be called by gimp_procedure_config_end_export()).
 *
 * The following boolean arguments of the used #GimpSaveProcedure are
 * synced. The procedure can but must not provide these arguments.
 *
 * "save-exif" for %GIMP_METADATA_SAVE_EXIF.
 *
 * "save-xmp" for %GIMP_METADATA_SAVE_XMP.
 *
 * "save-iptc" for %GIMP_METADATA_SAVE_IPTC.
 *
 * "save-thumbnail" for %GIMP_METADATA_SAVE_THUMBNAIL.
 *
 * "save-color-profile" for %GIMP_METADATA_SAVE_COLOR_PROFILE.
 *
 * "save-comment" for %GIMP_METADATA_SAVE_COMMENT.
 *
 * The values from the #GimpMetadataSaveFlags will only ever be used
 * to set these properties to %FALSE, overriding the user's saved
 * default values for the procedure, but NOT overriding the last used
 * values from exporting @original_image or the last used values from
 * exporting any other image using this procedure.
 *
 * If @mime_type is %NULL, #GimpMetadata handling is skipped. The
 * procedure can still have all of the above listed boolean arguments,
 * but must take care of their default values itself. The easiest way
 * to do this is by simply using gimp_export_comment(),
 * gimp_export_exif() etc. as default values for these arguments when
 * adding them using GIMP_PROC_ARG_BOOLEAN() or
 * GIMP_PROC_AUX_ARG_BOOLEAN().
 *
 * Additionally, some procedure arguments are handled automatically
 * regardless of whether @mime_type is %NULL or not:
 *
 * If the procedure has a "comment" argument, it is synced with the
 * image's "gimp-comment" parasite, unless @run_mode is
 * %GIMP_RUN_NONINTERACTIVE and the argument is not an auxiliary
 * argument. If there is no "gimp-comment" parasite, the "comment"
 * argument is initialized with the default comment returned by
 * gimp_get_default_comment().
 *
 * Returns: (transfer none) (nullable): The #GimpMetadata to be used for this
 *          export, or %NULL if @original_image doesn't have metadata.
 *
 * Since: 3.0
 **/
GimpMetadata *
gimp_procedure_config_begin_export (GimpProcedureConfig  *config,
                                    GimpImage            *original_image,
                                    GimpRunMode           run_mode,
                                    const GimpValueArray *args,
                                    const gchar          *mime_type)
{
  GObjectClass *object_class;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (original_image), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  object_class = G_OBJECT_GET_CLASS (config);

  if (mime_type)
    {
      GimpMetadataSaveFlags metadata_flags;
      gint                  i;

      config->priv->metadata =
        gimp_image_metadata_save_prepare (original_image,
                                          mime_type,
                                          &metadata_flags);

      if (config->priv->metadata)
        {
          config->priv->mime_type      = g_strdup (mime_type);
          config->priv->metadata_flags = metadata_flags;
        }

      for (i = 0; i < G_N_ELEMENTS (metadata_properties); i++)
        {
          /*  we only disable properties based on metadata flags here
           *  and never enable them, so we don't override the user's
           *  saved default values that are passed to us via "args"
           */
          if (! (metadata_flags &  metadata_properties[i].flag))
            {
              const gchar *prop_name = metadata_properties[i].name;
              GParamSpec  *pspec;

              pspec = g_object_class_find_property (object_class, prop_name);
              if (pspec)
                g_object_set (config,
                              prop_name, FALSE,
                              NULL);
            }
        }
    }

  gimp_procedure_config_begin_run (config, original_image, run_mode, args);

  if (g_object_class_find_property (object_class, "comment"))
    {
      /*  we set the comment property from the image comment if it is
       *  an aux argument, or if we run interactively, because the
       *  image comment should be global and not depend on whatever
       *  comment another image had when last using this export
       *  procedure
       */
      if (gimp_procedure_find_aux_argument (config->priv->procedure,
                                            "comment") ||
          (run_mode != GIMP_RUN_NONINTERACTIVE))
        {
          GimpParasite *parasite;
          gchar        *comment = NULL;

          parasite = gimp_image_get_parasite (original_image, "gimp-comment");
          if (parasite)
            {
              comment = g_strndup (gimp_parasite_data (parasite),
                                   gimp_parasite_data_size (parasite));

              if (comment && ! strlen (comment))
                g_clear_pointer (&comment, g_free);

              gimp_parasite_free (parasite);
            }

          if (! comment)
            comment = gimp_get_default_comment ();

          if (comment && strlen (comment))
            g_object_set (config,
                          "comment", comment,
                          NULL);

          g_free (comment);
        }
    }

  return config->priv->metadata;
}

/**
 * gimp_procedure_config_end_export:
 * @config:         a #GimpProcedureConfig
 * @exported_image: the #GimpImage that was actually exported
 * @file:           the #GFile @exported_image was written to
 * @status:         the return status of the #GimpProcedure's run()
 *
 * This is a variant of gimp_procedure_config_end_run() to be used by
 * file export procedures using #GimpSaveProcedure. It must be paired
 * with a call to gimp_procedure_config_begin_export() at the
 * beginning of run().
 *
 * It does everything gimp_procedure_config_begin_run() does but
 * provides additional features to automate file export:
 *
 * If @status is %GIMP_PDB_SUCCESS, and
 * gimp_procedure_config_begin_export() returned a #GimpMetadata,
 * @config's export properties are synced back to the metadata's
 * #GimpMetadataSaveFlags and the metadata is written to @file using
 * gimp_image_metadata_save_finish().
 *
 * If the procedure has a "comment" argument, and it was modified
 * during an interactive run, it is synced back to the original
 * image's "gimp-comment" parasite.
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_end_export (GimpProcedureConfig  *config,
                                  GimpImage            *exported_image,
                                  GFile                *file,
                                  GimpPDBStatusType     status)
{
  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (GIMP_IS_IMAGE (exported_image));
  g_return_if_fail (G_IS_FILE (file));

  if (status == GIMP_PDB_SUCCESS)
    {
      GObjectClass *object_class = G_OBJECT_GET_CLASS (config);

      /*  we write the comment back to the image if it was modified
       *  during an interactive export
       */
      if (g_object_class_find_property (object_class, "comment") &&
          config->priv->run_mode == GIMP_RUN_INTERACTIVE)
        {
          GimpParasite *parasite;
          gchar        *comment;

          g_object_get (config,
                        "comment", &comment,
                        NULL);

          parasite = gimp_image_get_parasite (config->priv->image,
                                              "gimp-comment");
          if (parasite)
            {
              /*  it there is an image comment, always override it if
               *  the comment was changed
               */
              gchar *image_comment;

              image_comment = g_strndup (gimp_parasite_data (parasite),
                                         gimp_parasite_data_size (parasite));
              gimp_parasite_free (parasite);

              if (g_strcmp0 (comment, image_comment))
                {
                  if (comment && strlen (comment))
                    {
                      parasite = gimp_parasite_new ("gimp-comment",
                                                    GIMP_PARASITE_PERSISTENT,
                                                    strlen (comment) + 1,
                                                    comment);
                      gimp_image_attach_parasite (config->priv->image,
                                                  parasite);
                      gimp_parasite_free (parasite);
                    }
                  else
                    {
                      gimp_image_detach_parasite (config->priv->image,
                                                  "gimp-comment");
                    }
                }

              g_free (image_comment);
            }
          else
            {
              /*  otherwise, set an image comment if the comment was
               *  changed from the default comment
               */
              gchar *default_comment;

              default_comment = gimp_get_default_comment ();

              if (g_strcmp0 (comment, default_comment) &&
                  comment && strlen (comment))
                {
                  parasite = gimp_parasite_new ("gimp-comment",
                                                GIMP_PARASITE_PERSISTENT,
                                                strlen (comment) + 1,
                                                comment);
                  gimp_image_attach_parasite (config->priv->image,
                                              parasite);
                  gimp_parasite_free (parasite);
                }

              g_free (default_comment);
            }

          g_free (comment);
        }

      if (config->priv->metadata)
        {
          gint i;

          for (i = 0; i < G_N_ELEMENTS (metadata_properties); i++)
            {
              const gchar           *prop_name = metadata_properties[i].name;
              GimpMetadataSaveFlags  prop_flag = metadata_properties[i].flag;
              GParamSpec            *pspec;
              gboolean               value;

              pspec = g_object_class_find_property (object_class, prop_name);
              if (pspec)
                {
                  g_object_get (config,
                                prop_name, &value,
                                NULL);

                  if (value)
                    config->priv->metadata_flags |= prop_flag;
                  else
                    config->priv->metadata_flags &= ~prop_flag;
                }

              gimp_image_metadata_save_finish (exported_image,
                                               config->priv->mime_type,
                                               config->priv->metadata,
                                               config->priv->metadata_flags,
                                               file, NULL);
            }
        }
    }

  g_clear_object (&config->priv->metadata);
  g_clear_pointer (&config->priv->mime_type, g_free);
  config->priv->metadata_flags = 0;

  gimp_procedure_config_end_run (config, status);
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

gboolean
gimp_procedure_config_load_default (GimpProcedureConfig  *config,
                                    GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_procedure_config_get_file (config, ".default");

  success = gimp_config_deserialize_file (GIMP_CONFIG (config),
                                          file,
                                          NULL, error);

  if (! success && error && (*error)->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (error);
    }

  g_object_unref (file);

  return success;
}

gboolean
gimp_procedure_config_save_default (GimpProcedureConfig  *config,
                                    GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_procedure_config_get_file (config, ".default");

  success = gimp_config_serialize_to_file (GIMP_CONFIG (config),
                                           file,
                                           "settings",
                                           "end of settings",
                                           NULL, error);

  g_object_unref (file);

  return success;
}

gboolean
gimp_procedure_config_load_last (GimpProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_procedure_config_get_file (config, ".last");

  success = gimp_config_deserialize_file (GIMP_CONFIG (config),
                                          file,
                                          NULL, error);

  if (! success && (*error)->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (error);
    }

  g_object_unref (file);

  return success;
}

gboolean
gimp_procedure_config_save_last (GimpProcedureConfig  *config,
                                 GError              **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_procedure_config_get_file (config, ".last");

  success = gimp_config_serialize_to_file (GIMP_CONFIG (config),
                                           file,
                                           "settings",
                                           "end of settings",
                                           NULL, error);

  g_object_unref (file);

  return success;
}

static gchar *
gimp_procedure_config_parasite_name (GimpProcedureConfig *config,
                                     const gchar         *suffix)
{
  return g_strconcat (G_OBJECT_TYPE_NAME (config), suffix, NULL);
}

gboolean
gimp_procedure_config_load_parasite (GimpProcedureConfig  *config,
                                     GimpImage            *image,
                                     GError              **error)
{
  gchar        *name;
  GimpParasite *parasite;
  gboolean      success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = gimp_procedure_config_parasite_name (config, "-last");
  parasite = gimp_image_get_parasite (image, name);
  g_free (name);

  if (! parasite)
    return FALSE;

  success = gimp_config_deserialize_parasite (GIMP_CONFIG (config),
                                              parasite,
                                              NULL, error);
  gimp_parasite_free (parasite);

  return success;
}

gboolean
gimp_procedure_config_save_parasite (GimpProcedureConfig  *config,
                                     GimpImage            *image,
                                     GError              **error)
{
  gchar        *name;
  GimpParasite *parasite;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);
  g_return_val_if_fail (GIMP_IS_IMAGE (image), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  name = gimp_procedure_config_parasite_name (config, "-last");
  parasite = gimp_config_serialize_to_parasite (GIMP_CONFIG (config),
                                                name,
                                                GIMP_PARASITE_PERSISTENT,
                                                NULL);
  g_free (name);

  if (! parasite)
    return FALSE;

  gimp_image_attach_parasite (image, parasite);
  gimp_parasite_free (parasite);

  return TRUE;
}
