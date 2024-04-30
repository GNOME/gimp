/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpexportprocedure.c
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

#include "gimp.h"

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimppdb_pdb.h"
#include "gimpplugin-private.h"
#include "gimpexportprocedure.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


/**
 * GimpExportProcedure:
 *
 * Export procedures implement image export.
 *
 * Registered export procedures will be automatically available in the export
 * interfaces and functions of GIMP. The detection (to decide which file is
 * redirected to which plug-in procedure) depends on the various methods set
 * with [class@FileProcedure] API.
 **/

enum
{
  PROP_0,
  PROP_SUPPORTS_EXIF,
  PROP_SUPPORTS_IPTC,
  PROP_SUPPORTS_XMP,
  PROP_SUPPORTS_PROFILE,
  PROP_SUPPORTS_THUMBNAIL,
  PROP_SUPPORTS_COMMENT,
  N_PROPS
};

struct _GimpExportProcedurePrivate
{
  GimpRunExportFunc  run_func;
  gpointer           run_data;
  GDestroyNotify     run_data_destroy;

  gboolean           supports_exif;
  gboolean           supports_iptc;
  gboolean           supports_xmp;
  gboolean           supports_profile;
  gboolean           supports_thumbnail;
  gboolean           supports_comment;

  gboolean           export_metadata;
};


static void   gimp_export_procedure_constructed   (GObject              *object);
static void   gimp_export_procedure_finalize      (GObject              *object);
static void   gimp_export_procedure_set_property  (GObject              *object,
                                                   guint                 property_id,
                                                   const GValue         *value,
                                                   GParamSpec           *pspec);
static void   gimp_export_procedure_get_property  (GObject              *object,
                                                   guint                 property_id,
                                                   GValue               *value,
                                                   GParamSpec           *pspec);

static void   gimp_export_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_export_procedure_run           (GimpProcedure        *procedure,
                                                   const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_export_procedure_create_config (GimpProcedure        *procedure,
                                                   GParamSpec          **args,
                                                   gint                  n_args);

static void   gimp_export_procedure_add_metadata  (GimpExportProcedure  *export_procedure);


G_DEFINE_TYPE_WITH_PRIVATE (GimpExportProcedure, gimp_export_procedure,
                            GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_export_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static void
gimp_export_procedure_class_init (GimpExportProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_export_procedure_constructed;
  object_class->finalize         = gimp_export_procedure_finalize;
  object_class->get_property     = gimp_export_procedure_get_property;
  object_class->set_property     = gimp_export_procedure_set_property;

  procedure_class->install       = gimp_export_procedure_install;
  procedure_class->run           = gimp_export_procedure_run;
  procedure_class->create_config = gimp_export_procedure_create_config;

  /**
   * GimpExportProcedure:supports-exif:
   *
   * Whether the export procedure supports EXIF.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_EXIF] = g_param_spec_boolean ("supports-exif",
                                                    "Supports EXIF metadata storage",
                                                    NULL,
                                                    FALSE,
                                                    G_PARAM_CONSTRUCT |
                                                    GIMP_PARAM_READWRITE);
  /**
   * GimpExportProcedure:supports-iptc:
   *
   * Whether the export procedure supports IPTC.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_IPTC] = g_param_spec_boolean ("supports-iptc",
                                                    "Supports IPTC metadata storage",
                                                    NULL,
                                                    FALSE,
                                                    G_PARAM_CONSTRUCT |
                                                    GIMP_PARAM_READWRITE);
  /**
   * GimpExportProcedure:supports-xmp:
   *
   * Whether the export procedure supports XMP.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_XMP] = g_param_spec_boolean ("supports-xmp",
                                                   "Supports XMP metadata storage",
                                                   NULL,
                                                   FALSE,
                                                   G_PARAM_CONSTRUCT |
                                                   GIMP_PARAM_READWRITE);
  /**
   * GimpExportProcedure:supports-profile:
   *
   * Whether the export procedure supports ICC color profiles.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_PROFILE] = g_param_spec_boolean ("supports-profile",
                                                       "Supports color profile storage",
                                                       NULL,
                                                       FALSE,
                                                       G_PARAM_CONSTRUCT |
                                                       GIMP_PARAM_READWRITE);
  /**
   * GimpExportProcedure:supports-thumbnail:
   *
   * Whether the export procedure supports storing a thumbnail.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_THUMBNAIL] = g_param_spec_boolean ("supports-thumbnail",
                                                         "Supports thumbnail storage",
                                                         NULL,
                                                         FALSE,
                                                         G_PARAM_CONSTRUCT |
                                                         GIMP_PARAM_READWRITE);
  /**
   * GimpExportProcedure:supports-comment:
   *
   * Whether the export procedure supports storing a comment.
   *
   * Since: 3.0.0
   */
  props[PROP_SUPPORTS_COMMENT] = g_param_spec_boolean ("supports-comment",
                                                       "Supports comment storage",
                                                       NULL,
                                                       FALSE,
                                                       G_PARAM_CONSTRUCT |
                                                       GIMP_PARAM_READWRITE);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
gimp_export_procedure_init (GimpExportProcedure *procedure)
{
  procedure->priv = gimp_export_procedure_get_instance_private (procedure);

  procedure->priv->export_metadata = FALSE;
}

static void
gimp_export_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GIMP_PROC_ARG_IMAGE (procedure, "image",
                       "Image",
                       "The image to export",
                       FALSE,
                       G_PARAM_READWRITE);

  GIMP_PROC_ARG_FILE (procedure, "file",
                      "File",
                      "The file to export to",
                      GIMP_PARAM_READWRITE);
}

static void
gimp_export_procedure_finalize (GObject *object)
{
  GimpExportProcedure *procedure = GIMP_EXPORT_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_export_procedure_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  GimpExportProcedure *procedure = GIMP_EXPORT_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_SUPPORTS_EXIF:
      procedure->priv->supports_exif = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_IPTC:
      procedure->priv->supports_iptc = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_XMP:
      procedure->priv->supports_xmp = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_PROFILE:
      procedure->priv->supports_profile = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_THUMBNAIL:
      procedure->priv->supports_thumbnail = g_value_get_boolean (value);
      break;
    case PROP_SUPPORTS_COMMENT:
      procedure->priv->supports_comment = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_export_procedure_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  GimpExportProcedure *procedure = GIMP_EXPORT_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_SUPPORTS_EXIF:
      g_value_set_boolean (value, procedure->priv->supports_exif);
      break;
    case PROP_SUPPORTS_IPTC:
      g_value_set_boolean (value, procedure->priv->supports_iptc);
      break;
    case PROP_SUPPORTS_XMP:
      g_value_set_boolean (value, procedure->priv->supports_xmp);
      break;
    case PROP_SUPPORTS_PROFILE:
      g_value_set_boolean (value, procedure->priv->supports_profile);
      break;
    case PROP_SUPPORTS_THUMBNAIL:
      g_value_set_boolean (value, procedure->priv->supports_thumbnail);
      break;
    case PROP_SUPPORTS_COMMENT:
      g_value_set_boolean (value, procedure->priv->supports_comment);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_export_procedure_install (GimpProcedure *procedure)
{
  GimpFileProcedure *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

  gimp_export_procedure_add_metadata (GIMP_EXPORT_PROCEDURE (procedure));
  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  _gimp_pdb_set_file_proc_save_handler (gimp_procedure_get_name (procedure),
                                        gimp_file_procedure_get_extensions (file_proc),
                                        gimp_file_procedure_get_prefixes (file_proc));

  if (gimp_file_procedure_get_handles_remote (file_proc))
    _gimp_pdb_set_file_proc_handles_remote (gimp_procedure_get_name (procedure));

  mime_types = gimp_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    _gimp_pdb_set_file_proc_mime_types (gimp_procedure_get_name (procedure),
                                        mime_types);

  priority = gimp_file_procedure_get_priority (file_proc);
  if (priority != 0)
    _gimp_pdb_set_file_proc_priority (gimp_procedure_get_name (procedure),
                                      priority);
}

#define ARG_OFFSET 3

static GimpValueArray *
gimp_export_procedure_run (GimpProcedure        *procedure,
                           const GimpValueArray *args)
{
  GimpPlugIn           *plug_in;
  GimpExportProcedure  *export_proc = GIMP_EXPORT_PROCEDURE (procedure);
  GimpValueArray       *remaining;
  GimpValueArray       *return_values;
  GimpProcedureConfig  *config;
  GimpRunMode           run_mode;
  GimpImage            *image;
  GFile                *file;
  GimpMetadata         *metadata;
  gchar                *mimetype = NULL;
  GimpPDBStatusType     status   = GIMP_PDB_EXECUTION_ERROR;

  run_mode = GIMP_VALUES_GET_ENUM  (args, 0);
  image    = GIMP_VALUES_GET_IMAGE (args, 1);
  file     = GIMP_VALUES_GET_FILE  (args, 2);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (gint i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config = gimp_procedure_create_config (procedure);
  if (export_proc->priv->export_metadata)
    {
      mimetype = (gchar *) gimp_file_procedure_get_mime_types (GIMP_FILE_PROCEDURE (procedure));
      if (mimetype == NULL)
        {
          g_printerr ("%s: ERROR: the GimpExportProcedureConfig object was created "
                      "with export_metadata but you didn't set any MimeType with gimp_file_procedure_set_mime_types()!\n",
                      G_STRFUNC);
        }
      else
        {
          char *delim;

          mimetype = g_strdup (mimetype);
          mimetype = g_strstrip (mimetype);
          delim = strstr (mimetype, ",");
          if (delim)
            *delim = '\0';
          /* Though docs only writes about the list being comma-separated, our
           * code apparently also split by spaces.
           */
          delim = strstr (mimetype, " ");
          if (delim)
            *delim = '\0';
          delim = strstr (mimetype, "\t");
          if (delim)
            *delim = '\0';
        }
    }
  metadata = _gimp_procedure_config_begin_export (config, image, run_mode, remaining, mimetype);
  g_free (mimetype);

  return_values = export_proc->priv->run_func (procedure, run_mode, image,
                                               file, metadata, config,
                                               export_proc->priv->run_data);

  if (return_values != NULL                       &&
      gimp_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (return_values, 0)))
    status = GIMP_VALUES_GET_ENUM (return_values, 0);

  _gimp_procedure_config_end_export (config, image, file, status);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = gimp_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _gimp_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the GimpExportProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
  gimp_value_array_unref (remaining);

  return return_values;
}

static GimpProcedureConfig *
gimp_export_procedure_create_config (GimpProcedure  *procedure,
                                     GParamSpec    **args,
                                     gint            n_args)
{
  gimp_export_procedure_add_metadata (GIMP_EXPORT_PROCEDURE (procedure));

  if (n_args > ARG_OFFSET)
    {
      args   += ARG_OFFSET;
      n_args -= ARG_OFFSET;
    }
  else
    {
      args   = NULL;
      n_args = 0;
    }

  return GIMP_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}

static void
gimp_export_procedure_add_metadata (GimpExportProcedure *export_procedure)
{
  GimpProcedure   *procedure = GIMP_PROCEDURE (export_procedure);
  static gboolean  ran_once = FALSE;

  if (ran_once)
    return;

  if (export_procedure->priv->supports_exif)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-exif",
                               _("Save _Exif"),
                               _("Save Exif (Exchangeable image file format) metadata"),
                               gimp_export_exif (),
                               G_PARAM_READWRITE);
  if (export_procedure->priv->supports_iptc)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-iptc",
                               _("Save _IPTC"),
                               _("Save IPTC (International Press Telecommunications Council) metadata"),
                               gimp_export_iptc (),
                               G_PARAM_READWRITE);
  if (export_procedure->priv->supports_xmp)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-xmp",
                               _("Save _XMP"),
                               _("Save XMP (Extensible Metadata Platform) metadata"),
                               gimp_export_xmp (),
                               G_PARAM_READWRITE);
  if (export_procedure->priv->supports_profile)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-color-profile",
                               _("Save color _profile"),
                               _("Save the ICC color profile as metadata"),
                               gimp_export_color_profile (),
                               G_PARAM_READWRITE);
  if (export_procedure->priv->supports_thumbnail)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-thumbnail",
                               _("Save _thumbnail"),
                               _("Save a smaller representation of the image as metadata"),
                               gimp_export_thumbnail (),
                               G_PARAM_READWRITE);
  if (export_procedure->priv->supports_comment)
    {
      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-comment",
                                 _("Save c_omment"),
                                 _("Save a comment as metadata"),
                                 gimp_export_comment (),
                                 G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_STRING (procedure, "gimp-comment",
                                _("Comment"),
                                _("Image comment"),
                                gimp_get_default_comment (),
                                G_PARAM_READWRITE);

      gimp_procedure_set_argument_sync (procedure, "gimp-comment",
                                        GIMP_ARGUMENT_SYNC_PARASITE);
    }

  ran_once = TRUE;
}


/*  public functions  */

/**
 * gimp_export_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @export_metadata:  whether GIMP should handle metadata exporting.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new export procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * #GimpExportProcedure is a #GimpProcedure subclass that makes it easier
 * to write file export procedures.
 *
 * It automatically adds the standard
 *
 * (#GimpRunMode, #GimpImage, #GimpDrawable, #GFile)
 *
 * arguments of an export procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpRunExportFunc. The
 * #GimpProcedureConfig of #GimpRunExportFunc only contains additionally added
 * arguments.
 *
 * If @export_metadata is TRUE, then the class will also handle the metadata
 * export if the format is supported by our backend. This requires you to also
 * set appropriate MimeType with gimp_file_procedure_set_mime_types().
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure *
gimp_export_procedure_new (GimpPlugIn       *plug_in,
                           const gchar      *name,
                           GimpPDBProcType   proc_type,
                           gboolean          export_metadata,
                           GimpRunExportFunc run_func,
                           gpointer          run_data,
                           GDestroyNotify    run_data_destroy)
{
  GimpExportProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_EXPORT_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->export_metadata  = export_metadata;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}

/**
 * gimp_export_procedure_set_support_exif:
 * @procedure: a #GimpProcedure.
 * @supports:  whether Exif metadata are supported.
 *
 * Determine whether @procedure supports exporting Exif data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument "save-exif" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be [func@export_exif].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_exif (GimpExportProcedure *procedure,
                                        gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-exif", supports,
                NULL);
}

/**
 * gimp_export_procedure_set_support_iptc:
 * @procedure: a #GimpProcedure.
 * @supports:  whether IPTC metadata are supported.
 *
 * Determine whether @procedure supports exporting IPTC data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument "save-iptc" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be [func@export_iptc].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_iptc (GimpExportProcedure *procedure,
                                        gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-iptc", supports,
                NULL);
}

/**
 * gimp_export_procedure_set_support_xmp:
 * @procedure: a #GimpProcedure.
 * @supports:  whether XMP metadata are supported.
 *
 * Determine whether @procedure supports exporting XMP data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument "save-xmp" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be [func@export_xmp].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_xmp (GimpExportProcedure *procedure,
                                       gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-xmp", supports,
                NULL);
}

/**
 * gimp_export_procedure_set_support_profile:
 * @procedure: a #GimpProcedure.
 * @supports:  whether color profiles can be stored.
 *
 * Determine whether @procedure supports exporting ICC color profiles. By
 * default, it won't (so there is usually no reason to run this function
 * with %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument
 *   "save-color-profile" in the end of the argument list of @procedure,
 *   with relevant blurb and description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be [func@export_color_profile].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_profile (GimpExportProcedure *procedure,
                                           gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-profile", supports,
                NULL);
}

/**
 * gimp_export_procedure_set_support_thumbnail:
 * @procedure: a #GimpProcedure.
 * @supports:  whether a thumbnail can be stored.
 *
 * Determine whether @procedure supports exporting a thumbnail. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument "save-thumbnail"
 *   in the end of the argument list of @procedure, with relevant blurb
 *   and description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be
 * [func@export_thumbnail].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_thumbnail (GimpExportProcedure *procedure,
                                             gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-thumbnail", supports,
                NULL);
}

/**
 * gimp_export_procedure_set_support_comment:
 * @procedure: a #GimpProcedure.
 * @supports:  whether a comment can be stored.
 *
 * Determine whether @procedure supports exporting a comment. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 *
 * This will have several consequences:
 *
 * - Automatically adds a standard auxiliary argument "save-comment"
 *   in the end of the argument list of @procedure, with relevant blurb
 *   and description.
 * - If used with other gimp_export_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all export procedures consistent.
 * - Generated GimpExportProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from [class@ProcedureConfig] will automatically process these
 *   properties to decide whether to export a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be
 * [func@export_comment].
 *
 * Since: 3.0
 **/
void
gimp_export_procedure_set_support_comment (GimpExportProcedure *procedure,
                                           gboolean             supports)
{
  g_return_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-comment", supports,
                NULL);
}

/**
 * gimp_export_procedure_get_support_exif:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports Exif exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_exif (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_exif;
}

/**
 * gimp_export_procedure_get_support_iptc:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports IPTC exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_iptc (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_iptc;
}

/**
 * gimp_export_procedure_get_support_xmp:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports XMP exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_xmp (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_xmp;
}

/**
 * gimp_export_procedure_get_support_profile:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports ICC color profile exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_profile (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_profile;
}

/**
 * gimp_export_procedure_get_support_thumbnail:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports thumbnail exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_thumbnail (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_thumbnail;
}

/**
 * gimp_export_procedure_get_support_comment:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports comment exporting.
 *
 * Since: 3.0
 **/
gboolean
gimp_export_procedure_get_support_comment (GimpExportProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_EXPORT_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_comment;
}
