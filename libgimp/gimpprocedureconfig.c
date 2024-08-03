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
 * GimpProcedureConfig:
 *
 * The base class for [class@Procedure] specific config objects and the main
 * interface to manage aspects of [class@Procedure]'s arguments such as
 * persistency of the last used arguments across GIMP sessions.
 *
 * A procedure config is created by a [class@Procedure] using
 * [method@Procedure.create_config] and its properties match the
 * procedure's arguments and auxiliary arguments in number, order and
 * type.
 *
 * It implements the [struct@Config] interface and therefore has all its
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


typedef struct _GimpProcedureConfigPrivate
{
  GimpProcedure         *procedure;

  GimpImage             *image;
  GimpRunMode            run_mode;

  GimpMetadata          *metadata;
  gchar                 *mime_type;
  GimpMetadataSaveFlags  metadata_flags;
  gboolean               metadata_saved;
} GimpProcedureConfigPrivate;

#define GET_PRIVATE(obj) ((GimpProcedureConfigPrivate *) gimp_procedure_config_get_instance_private ((GimpProcedureConfig *) (obj)))


static void       gimp_procedure_config_constructed   (GObject             *object);
static void       gimp_procedure_config_dispose       (GObject             *object);
static void       gimp_procedure_config_set_property  (GObject             *object,
                                                       guint                property_id,
                                                       const GValue        *value,
                                                       GParamSpec          *pspec);
static void       gimp_procedure_config_get_property  (GObject             *object,
                                                       guint                property_id,
                                                       GValue              *value,
                                                       GParamSpec          *pspec);


static void       gimp_procedure_config_set_values    (GimpProcedureConfig  *config,
                                                       const GimpValueArray *values);

static gboolean   gimp_procedure_config_load_last     (GimpProcedureConfig  *config,
                                                        GError              **error);
static gboolean   gimp_procedure_config_save_last     (GimpProcedureConfig  *config,
                                                       GError              **error);

static gboolean   gimp_procedure_config_load_parasite (GimpProcedureConfig  *config,
                                                       GimpImage            *image,
                                                       GError              **error);
static gboolean   gimp_procedure_config_save_parasite (GimpProcedureConfig  *config,
                                                       GimpImage            *image,
                                                       GError              **error);

static GFile    * gimp_procedure_config_get_file      (GimpProcedureConfig *config,
                                                       const gchar         *extension);
static gchar    * gimp_procedure_config_parasite_name (GimpProcedureConfig *config,
                                                       const gchar         *suffix);


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
  GET_PRIVATE (config)->run_mode = -1;
}

static void
gimp_procedure_config_constructed (GObject *object)
{
  GimpProcedureConfig *config = GIMP_PROCEDURE_CONFIG (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (GIMP_IS_PROCEDURE (GET_PRIVATE (config)->procedure));
}

static void
gimp_procedure_config_dispose (GObject *object)
{
  GimpProcedureConfig        *config = GIMP_PROCEDURE_CONFIG (object);
  GimpProcedureConfigPrivate *priv = GET_PRIVATE (config);

  g_clear_object (&priv->procedure);
  g_clear_object (&priv->metadata);
  g_clear_pointer (&priv->mime_type, g_free);

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
      GET_PRIVATE (config)->procedure = g_value_dup_object (value);
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
      g_value_set_object (value, GET_PRIVATE (config)->procedure);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


/*  public functions  */

/**
 * gimp_procedure_config_get_procedure:
 * @config: a procedure config
 *
 * This function returns the [class@Procedure] which created @config, see
 * [method@Procedure.create_config].
 *
 * Returns: (transfer none): The procedure which created this config.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_procedure_config_get_procedure (GimpProcedureConfig *config)
{
  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);

  return GET_PRIVATE (config)->procedure;
}

static void
gimp_procedure_config_get_parasite (GimpProcedureConfig *config,
                                    GParamSpec          *pspec)
{
  GimpParasite *parasite;
  gchar        *value = NULL;

  /*  for now we only support strings  */
  if (! GET_PRIVATE (config)->image ||
      ! G_IS_PARAM_SPEC_STRING (pspec))
    return;

  parasite = gimp_image_get_parasite (GET_PRIVATE (config)->image,
                                      pspec->name);

  if (parasite)
    {
      guint32 parasite_size;

      value = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      value = g_strndup (value, parasite_size);
      gimp_parasite_free (parasite);

      if (value && ! strlen (value))
        g_clear_pointer (&value, g_free);
    }

  if (! value)
    {
      /*  special case "gimp-comment" here, yes this is bad hack  */
      if (! strcmp (pspec->name, "gimp-comment"))
        {
          value = gimp_get_default_comment ();
        }
    }

  if (value && strlen (value))
    g_object_set (config,
                  pspec->name, value,
                  NULL);

  g_free (value);
}

static void
gimp_procedure_config_set_parasite (GimpProcedureConfig *config,
                                    GParamSpec          *pspec)
{
  GimpProcedureConfigPrivate *priv = GET_PRIVATE (config);
  GimpParasite *parasite;
  gchar        *value;

  /*  for now we only support strings  */
  if (! priv->image || ! G_IS_PARAM_SPEC_STRING (pspec))
    return;

  g_object_get (config,
                pspec->name, &value,
                NULL);

  parasite = gimp_image_get_parasite (priv->image, pspec->name);

  if (parasite)
    {
      /*  it there is a parasite, always override it if its value was
       *  changed
       */
      gchar   *image_value;
      guint32  parasite_size;

      image_value = (gchar *) gimp_parasite_get_data (parasite, &parasite_size);
      image_value = g_strndup (image_value, parasite_size);
      gimp_parasite_free (parasite);

      if (g_strcmp0 (value, image_value))
        {
          if (value && strlen (value))
            {
              parasite = gimp_parasite_new (pspec->name,
                                            GIMP_PARASITE_PERSISTENT,
                                            strlen (value) + 1,
                                            value);
              gimp_image_attach_parasite (priv->image, parasite);
              gimp_parasite_free (parasite);
            }
          else
            {
              gimp_image_detach_parasite (priv->image, pspec->name);
            }
        }

      g_free (image_value);
    }
  else
    {
      /*  otherwise, set the parasite if the value was changed from
       *  the default value
       */
      gchar *default_value = NULL;

      /*  special case "gimp-comment" here, yes this is bad hack  */
      if (! strcmp (pspec->name, "gimp-comment"))
        {
          default_value = gimp_get_default_comment ();
        }

      if (g_strcmp0 (value, default_value) &&
          value && strlen (value))
        {
          parasite = gimp_parasite_new (pspec->name,
                                        GIMP_PARASITE_PERSISTENT,
                                        strlen (value) + 1,
                                        value);
          gimp_image_attach_parasite (priv->image, parasite);
          gimp_parasite_free (parasite);
        }

      g_free (default_value);
    }

  g_free (value);
}

/**
 * gimp_procedure_config_save_metadata:
 * @config:         a #GimpProcedureConfig
 * @exported_image: the image that was actually exported
 * @file:           the file @exported_image was written to
 *
 * Note: There is normally no need to call this function because it's
 * already called by [class@ExportProcedure] at the end of the `run()` callback.
 *
 * Only use this function if the [class@Metadata] passed as argument of a
 * [class@ExportProcedure]'s run() method needs to be written at a specific
 * point of the export, other than its end.
 *
 * This function syncs back @config's export properties to the
 * metadata's [flags@MetadataSaveFlags] and writes the metadata to @file
 * using [method@Image.metadata_save_finish].
 *
 * The metadata is only ever written once. If this function has been
 * called explicitly, it will do nothing when called a second time at the end of
 * the `run()` callback.
 *
 * Since: 3.0
 **/
void
gimp_procedure_config_save_metadata (GimpProcedureConfig *config,
                                     GimpImage           *exported_image,
                                     GFile               *file)
{
  GimpProcedureConfigPrivate *priv = GET_PRIVATE (config);

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (GIMP_IS_IMAGE (exported_image));
  g_return_if_fail (G_IS_FILE (file));

  if (priv->metadata && ! priv->metadata_saved)
    {
      GObjectClass *object_class = G_OBJECT_GET_CLASS (config);
      GError       *error        = NULL;
      gint          i;

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
                priv->metadata_flags |= prop_flag;
              else
                priv->metadata_flags &= ~prop_flag;
            }
          else
            {
              priv->metadata_flags &= ~prop_flag;
            }
        }

      if (! gimp_image_metadata_save_finish (exported_image,
                                             priv->mime_type,
                                             priv->metadata,
                                             priv->metadata_flags,
                                             file, &error))
        {
          if (error)
            {
              /* Even though a failure to write metadata is not enough
                 reason to say we failed to save the image, we should
                 still notify the user about the problem. */
              g_message ("%s: saving metadata failed: %s",
                         G_STRFUNC, error->message);
              g_error_free (error);
            }
        }

      priv->metadata_saved = TRUE;
    }
}

/**
 * gimp_procedure_config_get_choice_id:
 * @config:        a #GimpProcedureConfig
 * @property_name: the name of a [struct@ParamSpecChoice] property.
 *
 * A utility function which will get the current string value of a
 * [struct@ParamSpecChoice] property in @config and convert it to the integer ID
 * mapped to this value.
 * This makes it easy to work with an Enum type locally, within a plug-in code.
 *
 * Since: 3.0
 **/
gint
gimp_procedure_config_get_choice_id (GimpProcedureConfig *config,
                                     const gchar         *property_name)
{
  GParamSpec          *param_spec;
  GimpParamSpecChoice *cspec;
  gchar               *value = NULL;
  gint                 id;

  param_spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                             property_name);

  if (! param_spec)
    {
      g_warning ("%s: %s has no property named '%s'",
                 G_STRFUNC,
                 g_type_name (G_TYPE_FROM_INSTANCE (config)),
                 property_name);
      return 0;
    }

  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (param_spec), GIMP_TYPE_PARAM_CHOICE))
    {
      g_warning ("%s: property '%s' of %s is not a GimpParamSpecChoice.",
                 G_STRFUNC,
                 param_spec->name,
                 g_type_name (param_spec->owner_type));
      return 0;
    }

  cspec = GIMP_PARAM_SPEC_CHOICE (param_spec);
  g_object_get (config,
                property_name, &value,
                NULL);
  id = gimp_choice_get_id (cspec->choice, value);

  g_free (value);

  return id;
}


/*  Functions only used by GimpProcedure classes  */

/**
 * _gimp_procedure_config_get_values:
 * @config: a #GimpProcedureConfig
 * @values: a #GimpValueArray
 *
 * Gets the values from @config's properties and stores them in
 * @values.
 *
 * See [method@ProcedureConfig.set_values].
 *
 * Since: 3.0
 **/
void
_gimp_procedure_config_get_values (GimpProcedureConfig  *config,
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
  gimp_procedure_get_aux_arguments (GET_PRIVATE (config)->procedure, &n_aux_args);
  n_values = gimp_value_array_length (values);

  /* The config will have 1 additional property: "procedure". */
  g_return_if_fail (n_pspecs == n_values + n_aux_args + 1);

  for (i = 0; i < n_values; i++)
    {
      GParamSpec *pspec = pspecs[i + 1];
      GValue     *value = gimp_value_array_index (values, i);

      g_object_get_property (G_OBJECT (config), pspec->name, value);
    }

  g_free (pspecs);
}

/**
 * _gimp_procedure_config_begin_run:
 * @config:   a #GimpProcedureConfig
 * @image: (nullable): a #GimpImage or %NULL
 * @run_mode: the #GimpRunMode passed to a [class@Procedure]'s run()
 * @args:     the #GimpValueArray passed to a [class@Procedure]'s run()
 *
 * Populates @config with values for a [class@Procedure]'s run(),
 * depending on @run_mode.
 *
 * If @run_mode is %GIMP_RUN_INTERACTIVE or %GIMP_RUN_WITH_LAST_VALS,
 * the saved values from the procedure's last run() are loaded and set
 * on @config. If @image is not %NULL, the last used values for this
 * image are tried first, and if no image-specific values are found
 * the globally saved last used values are used. If no saved last used
 * values are found, the procedure's default argument values are used.
 *
 * If @run_mode is %GIMP_RUN_NONINTERACTIVE, the contents of @args are
 * set on @config using [method@ProcedureConfig.set_values].
 *
 * After setting @config's properties like described above, arguments
 * and auxiliary arguments are automatically synced from image
 * parasites of the same name if they were set to
 * %GIMP_ARGUMENT_SYNC_PARASITE with [class@Procedure.set_argument_sync]:
 *
 * String properties are set to the value of the image parasite if
 * @run_mode is %GIMP_RUN_INTERACTIVE or %GIMP_RUN_WITH_LAST_VALS, or
 * if the corresponding argument is an auxiliary argument. As a
 * special case, a property named "gimp-comment" will default to
 * [func@get_default_comment] if there is no "gimp-comment" parasite.
 *
 * After calling this function, the @args passed to run() should be
 * left alone and @config be treated as the procedure's arguments.
 *
 * It is possible to get @config's resulting values back into @args by
 * calling [method@ProcedureConfig.get_values], as long as modified
 * @args are written back to @config using [method@ProcedureConfig.set_values]
 * before the call to [method@ProcedureConfig.end_run].
 *
 * This function should be used at the beginning of a procedure's
 * run() and be paired with a call to [method@ProcedureConfig.end_run]
 * at the end of run().
 *
 * Since: 3.0
 **/
void
_gimp_procedure_config_begin_run (GimpProcedureConfig  *config,
                                  GimpImage            *image,
                                  GimpRunMode           run_mode,
                                  const GimpValueArray *args)
{
  GimpProcedureConfigPrivate  *priv;
  GParamSpec                  *run_mode_pspec;
  GParamSpec                 **pspecs;
  guint                        n_pspecs;
  gint                         i;
  gboolean                     loaded = FALSE;
  GError                      *error  = NULL;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (image == NULL || GIMP_IS_IMAGE (image));
  g_return_if_fail (args != NULL);

  priv = GET_PRIVATE (config);

  priv->image    = image;
  priv->run_mode = run_mode;

  gimp_procedure_config_set_values (config, args);

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
      break;
    }

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

      /*  skip our own properties  */
      if (pspec->owner_type == GIMP_TYPE_PROCEDURE_CONFIG)
        continue;

      switch (gimp_procedure_get_argument_sync (priv->procedure, pspec->name))
        {
        case GIMP_ARGUMENT_SYNC_PARASITE:
          /*  we sync the property from the image parasite if it is an
           *  aux argument, or if we run interactively, because the
           *  parasite should be global to the image and not depend on
           *  whatever parasite another image had when last using this
           *  procedure
           */
          if (gimp_procedure_find_aux_argument (priv->procedure,
                                                pspec->name) ||
              (run_mode != GIMP_RUN_NONINTERACTIVE))
            {
              gimp_procedure_config_get_parasite (config, pspec);
            }
          break;

        default:
          break;
        }
    }

  /* Always set the run-mode in the end, which should be influenced neither by
   * the default property value, nor last run's value.
   */
  run_mode_pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "run-mode");
  if (run_mode_pspec != NULL && G_IS_PARAM_SPEC_ENUM (run_mode_pspec))
    g_object_set (config, "run-mode", run_mode, NULL);

  g_free (pspecs);
}

/**
 * _gimp_procedure_config_end_run:
 * @config: a #GimpProcedureConfig
 * @status: the return status of the [class@Procedure]'s run()
 *
 * This function is the counterpart of
 * [method@ProcedureConfig.begin_run] and must always be called in
 * pairs in a procedure's run(), before returning return values.
 *
 * If the @run_mode passed to [method@ProcedureConfig.end_run] was
 * %GIMP_RUN_INTERACTIVE, @config is saved as last used values to be
 * used when the procedure runs again. Additionally, if the #GimpImage
 * passed to [method@ProcedureConfig.begin_run] was not %NULL, @config is
 * attached to @image as last used values for this image using a
 * #GimpParasite and [method@Image.attach_parasite].
 *
 * If @run_mode was not %GIMP_RUN_NONINTERACTIVE, this function also
 * conveniently calls [func@displays_flush], which is what most
 * procedures want and doesn't do any harm if called redundantly.
 *
 * After a %GIMP_RUN_INTERACTIVE run, %GIMP_ARGUMENT_SYNC_PARASITE
 * values that have been changed are written back to their
 * corresponding image parasite.
 *
 * See [method@ProcedureConfig.begin_run].
 *
 * Since: 3.0
 **/
void
_gimp_procedure_config_end_run (GimpProcedureConfig *config,
                                GimpPDBStatusType    status)
{
  GimpProcedureConfigPrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));

  priv = GET_PRIVATE (config);

  if (priv->run_mode != GIMP_RUN_NONINTERACTIVE)
    gimp_displays_flush ();

  if (status == GIMP_PDB_SUCCESS &&
      priv->run_mode == GIMP_RUN_INTERACTIVE)
    {
      GParamSpec **pspecs;
      guint        n_pspecs;
      gint         i;
      GError      *error = NULL;

      if (priv->image)
        gimp_procedure_config_save_parasite (config, priv->image,
                                             NULL);

      if (! gimp_procedure_config_save_last (config, &error))
        {
          g_printerr ("Saving last used values to disk failed: %s\n",
                      error->message);
          g_clear_error (&error);
        }

      pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (config),
                                               &n_pspecs);

      for (i = 0; i < n_pspecs; i++)
        {
          GParamSpec *pspec = pspecs[i];

          /*  skip our own properties  */
          if (pspec->owner_type == GIMP_TYPE_PROCEDURE_CONFIG)
            continue;

          switch (gimp_procedure_get_argument_sync (priv->procedure,
                                                    pspec->name))
            {
            case GIMP_ARGUMENT_SYNC_PARASITE:
              gimp_procedure_config_set_parasite (config, pspec);
              break;

            default:
              break;
            }
        }

      g_free (pspecs);
    }

  priv->image    = NULL;
  priv->run_mode = -1;
}

/**
 * _gimp_procedure_config_begin_export:
 * @config:         a #GimpProcedureConfig
 * @original_image: the image passed to run()
 * @run_mode:       the #GimpRunMode passed to a [class@Procedure]'s run()
 * @args:           the value array passed to a [class@Procedure]'s run()
 * @mime_type: (nullable):  exported file format's mime type, or %NULL.
 *
 * This is a variant of [method@ProcedureConfig.begin_run] to be used
 * by file export procedures using [class@ExportProcedure]. It must be
 * paired with a call to [method@ProcedureConfig.end_export] at the end
 * of run().
 *
 * It does everything [method@ProcedureConfig.begin_run] does but
 * provides additional features to automate file export:
 *
 * If @mime_type is non-%NULL, exporting metadata is handled
 * automatically, by calling [method@Image.metadata_save_prepare] and
 * syncing its returned [flags@MetadataSaveFlags] with @config's
 * properties. (The corresponding [method@Image.metadata_save_finish]
 * will be called by [method@ProcedureConfig.end_export]).
 *
 * The following boolean arguments of the used [class@ExportProcedure] are
 * synced. The procedure can but must not provide these arguments.
 *
 * - "save-exif" for %GIMP_METADATA_SAVE_EXIF.
 * - "save-xmp" for %GIMP_METADATA_SAVE_XMP.
 * - "save-iptc" for %GIMP_METADATA_SAVE_IPTC.
 * - "save-thumbnail" for %GIMP_METADATA_SAVE_THUMBNAIL.
 * - "save-color-profile" for %GIMP_METADATA_SAVE_COLOR_PROFILE.
 * - "save-comment" for %GIMP_METADATA_SAVE_COMMENT.
 *
 * The values from the [flags@MetadataSaveFlags] will only ever be used
 * to set these properties to %FALSE, overriding the user's saved
 * default values for the procedure, but NOT overriding the last used
 * values from exporting @original_image or the last used values from
 * exporting any other image using this procedure.
 *
 * If @mime_type is %NULL, [class@Metadata] handling is skipped. The
 * procedure can still have all of the above listed boolean arguments,
 * but must take care of their default values itself. The easiest way
 * to do this is by simply using [func@export_comment], [func@export_exif] etc.
 * as default values for these arguments when adding them using
 * gimp_procedure_add_boolean_argument() or
 * gimp_procedure_add_boolean_aux_argument().
 *
 * Returns: (transfer none) (nullable): The metadata to be used
 *          for this export, or %NULL if @original_image doesn't have
 *          metadata.
 *
 * Since: 3.0
 **/
GimpMetadata *
_gimp_procedure_config_begin_export (GimpProcedureConfig  *config,
                                     GimpImage            *original_image,
                                     GimpRunMode           run_mode,
                                     const GimpValueArray *args,
                                     const gchar          *mime_type)
{
  GimpProcedureConfigPrivate *priv;
  GObjectClass               *object_class;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), NULL);
  g_return_val_if_fail (GIMP_IS_IMAGE (original_image), NULL);
  g_return_val_if_fail (args != NULL, NULL);

  priv = GET_PRIVATE (config);

  object_class = G_OBJECT_GET_CLASS (config);

  if (mime_type)
    {
      GimpMetadataSaveFlags metadata_flags;
      gint                  i;

      priv->metadata =
        gimp_image_metadata_save_prepare (original_image,
                                          mime_type,
                                          &metadata_flags);

      if (priv->metadata)
        {
          priv->mime_type      = g_strdup (mime_type);
          priv->metadata_flags = metadata_flags;
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

  _gimp_procedure_config_begin_run (config, original_image, run_mode, args);

  return priv->metadata;
}

/**
 * _gimp_procedure_config_end_export:
 * @config:         a #GimpProcedureConfig
 * @exported_image: the image that was actually exported
 * @file:           the #GFile @exported_image was written to
 * @status:         the return status of the [class@Procedure]'s run()
 *
 * This is a variant of [method@ProcedureConfig.end_run] to be used by
 * file export procedures using [class@ExportProcedure]. It must be paired
 * with a call to [method@ProcedureConfig.begin_export] at the
 * beginning of run().
 *
 * It does everything [method@ProcedureConfig.begin_run] does but
 * provides additional features to automate file export:
 *
 * If @status is %GIMP_PDB_SUCCESS, and
 * [method@ProcedureConfig.begin_export] returned a [class@Metadata], this
 * function calls [method@ProcedureConfig.save_metadata], which syncs
 * back @config's export properties to the metadata's
 * [flags@MetadataSaveFlags] and writes metadata to @file using
 * [method@Image.metadata_save_finish].
 *
 * Since: 3.0
 **/
void
_gimp_procedure_config_end_export (GimpProcedureConfig *config,
                                   GimpImage           *exported_image,
                                   GFile               *file,
                                   GimpPDBStatusType    status)
{
  GimpProcedureConfigPrivate *priv;

  g_return_if_fail (GIMP_IS_PROCEDURE_CONFIG (config));
  g_return_if_fail (GIMP_IS_IMAGE (exported_image));
  g_return_if_fail (G_IS_FILE (file));

  priv = GET_PRIVATE (config);

  if (status == GIMP_PDB_SUCCESS)
    {
      gimp_procedure_config_save_metadata (config, exported_image, file);
    }

  g_clear_object (&priv->metadata);
  g_clear_pointer (&priv->mime_type, g_free);
  priv->metadata_flags = 0;
  priv->metadata_saved = FALSE;

  _gimp_procedure_config_end_run (config, status);
}

gboolean
gimp_procedure_config_has_default (GimpProcedureConfig *config)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_PROCEDURE_CONFIG (config), FALSE);

  file = gimp_procedure_config_get_file (config, ".default");

  success = g_file_query_exists (file, NULL);
  g_object_unref (file);

  return success;
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


/*  private functions  */

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
 * arguments to the run() function of the [class@Procedure] which created
 * this @config. See [method@Procedure.create_config].
 *
 * Since: 3.0
 **/
static void
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
  gimp_procedure_get_aux_arguments (GET_PRIVATE (config)->procedure,
                                    &n_aux_args);
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

static gboolean
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

static gboolean
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

static gboolean
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

static gboolean
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

static gchar *
gimp_procedure_config_parasite_name (GimpProcedureConfig *config,
                                     const gchar         *suffix)
{
  return g_strconcat (G_OBJECT_TYPE_NAME (config), suffix, NULL);
}
