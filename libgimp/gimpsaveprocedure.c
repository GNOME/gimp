/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpsaveprocedure.c
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

#include "gimpsaveprocedure.h"
#include "gimppdb_pdb.h"


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

struct _GimpSaveProcedurePrivate
{
  GimpRunSaveFunc run_func;
  gpointer        run_data;
  GDestroyNotify  run_data_destroy;

  gboolean        supports_exif;
  gboolean        supports_iptc;
  gboolean        supports_xmp;
  gboolean        supports_profile;
  gboolean        supports_thumbnail;
  gboolean        supports_comment;
};


static void   gimp_save_procedure_constructed   (GObject              *object);
static void   gimp_save_procedure_finalize      (GObject              *object);
static void   gimp_save_procedure_set_property  (GObject              *object,
                                                 guint                 property_id,
                                                 const GValue         *value,
                                                 GParamSpec           *pspec);
static void   gimp_save_procedure_get_property  (GObject              *object,
                                                 guint                 property_id,
                                                 GValue               *value,
                                                 GParamSpec           *pspec);

static void   gimp_save_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_save_procedure_run           (GimpProcedure        *procedure,
                                                 const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_save_procedure_create_config (GimpProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);

static void   gimp_save_procedure_add_metadata  (GimpSaveProcedure    *save_procedure);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSaveProcedure, gimp_save_procedure,
                            GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_save_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };

static void
gimp_save_procedure_class_init (GimpSaveProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_save_procedure_constructed;
  object_class->finalize         = gimp_save_procedure_finalize;
  object_class->get_property     = gimp_save_procedure_get_property;
  object_class->set_property     = gimp_save_procedure_set_property;

  procedure_class->install       = gimp_save_procedure_install;
  procedure_class->run           = gimp_save_procedure_run;
  procedure_class->create_config = gimp_save_procedure_create_config;

  /**
   * GimpSaveProcedure:supports-exif:
   *
   * Whether the save procedure supports EXIF.
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
   * GimpSaveProcedure:supports-iptc:
   *
   * Whether the save procedure supports IPTC.
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
   * GimpSaveProcedure:supports-xmp:
   *
   * Whether the save procedure supports XMP.
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
   * GimpSaveProcedure:supports-profile:
   *
   * Whether the save procedure supports ICC color profiles.
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
   * GimpSaveProcedure:supports-thumbnail:
   *
   * Whether the save procedure supports storing a thumbnail.
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
   * GimpSaveProcedure:supports-comment:
   *
   * Whether the save procedure supports storing a comment.
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
gimp_save_procedure_init (GimpSaveProcedure *procedure)
{
  procedure->priv = gimp_save_procedure_get_instance_private (procedure);
}

static void
gimp_save_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GIMP_PROC_ARG_IMAGE (procedure, "image",
                       "Image",
                       "The image to save",
                       FALSE,
                       G_PARAM_READWRITE);

  GIMP_PROC_ARG_INT (procedure, "num-drawables",
                     "Number of drawables",
                     "Number of drawables to be saved",
                     0, G_MAXINT, 1,
                     G_PARAM_READWRITE);

  GIMP_PROC_ARG_OBJECT_ARRAY (procedure, "drawables",
                              "Drawables",
                              "The drawables to save",
                              GIMP_TYPE_DRAWABLE,
                              G_PARAM_READWRITE | GIMP_PARAM_NO_VALIDATE);

  GIMP_PROC_ARG_FILE (procedure, "file",
                      "File",
                      "The file to save to",
                      GIMP_PARAM_READWRITE);
}

static void
gimp_save_procedure_finalize (GObject *object)
{
  GimpSaveProcedure *procedure = GIMP_SAVE_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_save_procedure_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  GimpSaveProcedure *procedure = GIMP_SAVE_PROCEDURE (object);

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
gimp_save_procedure_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  GimpSaveProcedure *procedure = GIMP_SAVE_PROCEDURE (object);

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
gimp_save_procedure_install (GimpProcedure *procedure)
{
  GimpFileProcedure *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

  gimp_save_procedure_add_metadata (GIMP_SAVE_PROCEDURE (procedure));
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

#define ARG_OFFSET 5

static GimpValueArray *
gimp_save_procedure_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  GimpSaveProcedure *save_proc = GIMP_SAVE_PROCEDURE (procedure);
  GimpValueArray    *remaining;
  GimpValueArray    *return_values;
  GimpRunMode        run_mode;
  GimpImage         *image;
  GimpDrawable     **drawables;
  GFile             *file;
  gint               n_drawables;
  gint               i;

  run_mode    = GIMP_VALUES_GET_ENUM         (args, 0);
  image       = GIMP_VALUES_GET_IMAGE        (args, 1);
  n_drawables = GIMP_VALUES_GET_INT          (args, 2);
  drawables   = GIMP_VALUES_GET_OBJECT_ARRAY (args, 3);
  file        = GIMP_VALUES_GET_FILE         (args, 4);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  return_values = save_proc->priv->run_func (procedure,
                                             run_mode,
                                             image,
                                             n_drawables,
                                             drawables,
                                             file,
                                             remaining,
                                             save_proc->priv->run_data);
  gimp_value_array_unref (remaining);

  return return_values;
}

static GimpProcedureConfig *
gimp_save_procedure_create_config (GimpProcedure  *procedure,
                                   GParamSpec    **args,
                                   gint            n_args)
{
  gimp_save_procedure_add_metadata (GIMP_SAVE_PROCEDURE (procedure));

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
gimp_save_procedure_add_metadata (GimpSaveProcedure *save_procedure)
{
  GimpProcedure   *procedure = GIMP_PROCEDURE (save_procedure);
  static gboolean  ran_once = FALSE;

  if (ran_once)
    return;

  if (save_procedure->priv->supports_exif)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-exif",
                               "Save _Exif",
                               "Save Exif (Exchangeable image file format) metadata",
                               gimp_export_exif (),
                               G_PARAM_READWRITE);
  if (save_procedure->priv->supports_iptc)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-iptc",
                               "Save _IPTC",
                               "Save IPTC (International Press Telecommunications Council) metadata",
                               gimp_export_iptc (),
                               G_PARAM_READWRITE);
  if (save_procedure->priv->supports_xmp)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-xmp",
                               "Save _XMP",
                               "Save XMP (Extensible Metadata Platform) metadata",
                               gimp_export_xmp (),
                               G_PARAM_READWRITE);
  if (save_procedure->priv->supports_profile)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-color-profile",
                               "Save color _profile",
                               "Save the ICC color profile as metadata",
                               gimp_export_color_profile (),
                               G_PARAM_READWRITE);
  if (save_procedure->priv->supports_thumbnail)
    GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-thumbnail",
                               "Save _thumbnail",
                               "Save a smaller representation of the image as metadata",
                               gimp_export_thumbnail (),
                               G_PARAM_READWRITE);
  if (save_procedure->priv->supports_comment)
    {
      GIMP_PROC_AUX_ARG_BOOLEAN (procedure, "save-comment",
                                 "Save c_omment",
                                 "Save a comment as metadata",
                                 gimp_export_comment (),
                                 G_PARAM_READWRITE);
      GIMP_PROC_AUX_ARG_STRING (procedure, "gimp-comment",
                                "Comment",
                                "Image comment",
                                gimp_get_default_comment (),
                                G_PARAM_READWRITE);

      gimp_procedure_set_argument_sync (procedure, "gimp-comment",
                                        GIMP_ARGUMENT_SYNC_PARASITE);
    }

  ran_once = TRUE;
}


/*  public functions  */

/**
 * gimp_save_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new save procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * #GimpSaveProcedure is a #GimpProcedure subclass that makes it easier
 * to write file save procedures.
 *
 * It automatically adds the standard
 *
 * (#GimpRunMode, #GimpImage, #GimpDrawable, #GFile)
 *
 * arguments of a save procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpRunSaveFunc. The
 * "args" #GimpValueArray of #GimpRunSaveFunc only contains
 * additionally added arguments.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_save_procedure_new (GimpPlugIn      *plug_in,
                         const gchar     *name,
                         GimpPDBProcType  proc_type,
                         GimpRunSaveFunc  run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  GimpSaveProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_SAVE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}

/**
 * gimp_save_procedure_set_support_exif:
 * @procedure: a #GimpProcedure.
 * @supports:  whether Exif metadata are supported.
 *
 * Determine whether @procedure supports saving Exif data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument "save-exif" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be gimp_export_exif().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_exif (GimpSaveProcedure *procedure,
                                      gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-exif", supports,
                NULL);
}

/**
 * gimp_save_procedure_set_support_iptc:
 * @procedure: a #GimpProcedure.
 * @supports:  whether IPTC metadata are supported.
 *
 * Determine whether @procedure supports saving IPTC data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument "save-iptc" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be gimp_export_iptc().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_iptc (GimpSaveProcedure *procedure,
                                      gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-iptc", supports,
                NULL);
}

/**
 * gimp_save_procedure_set_support_xmp:
 * @procedure: a #GimpProcedure.
 * @supports:  whether XMP metadata are supported.
 *
 * Determine whether @procedure supports saving XMP data. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument "save-xmp" in the
 *   end of the argument list of @procedure, with relevant blurb and
 *   description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be gimp_export_xmp().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_xmp (GimpSaveProcedure *procedure,
                                     gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-xmp", supports,
                NULL);
}

/**
 * gimp_save_procedure_set_support_profile:
 * @procedure: a #GimpProcedure.
 * @supports:  whether color profiles can be stored.
 *
 * Determine whether @procedure supports saving ICC color profiles. By
 * default, it won't (so there is usually no reason to run this function
 * with %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument
 *   "save-color-profile" in the end of the argument list of @procedure,
 *   with relevant blurb and description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be
 * gimp_export_color_profile().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_profile (GimpSaveProcedure *procedure,
                                         gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-profile", supports,
                NULL);
}

/**
 * gimp_save_procedure_set_support_thumbnail:
 * @procedure: a #GimpProcedure.
 * @supports:  whether a thumbnail can be stored.
 *
 * Determine whether @procedure supports saving a thumbnail. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument "save-thumbnail"
 *   in the end of the argument list of @procedure, with relevant blurb
 *   and description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be
 * gimp_export_thumbnail().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_thumbnail (GimpSaveProcedure *procedure,
                                           gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-thumbnail", supports,
                NULL);
}

/**
 * gimp_save_procedure_set_support_comment:
 * @procedure: a #GimpProcedure.
 * @supports:  whether a comment can be stored.
 *
 * Determine whether @procedure supports saving a comment. By default,
 * it won't (so there is usually no reason to run this function with
 * %FALSE).
 * This will have several consequence:
 * - Automatically adds a standard auxiliary argument "save-comment"
 *   in the end of the argument list of @procedure, with relevant blurb
 *   and description.
 * - If used with other gimp_save_procedure_set_support_*() functions,
 *   they will always be ordered the same (the order of the calls don't
 *   matter), keeping all save procedures consistent.
 * - Generated GimpSaveProcedureDialog will contain the metadata
 *   options, once again always in the same order and with consistent
 *   GUI style across plug-ins.
 * - API from #GimpProcedureConfig will automatically process these
 *   properties to decide whether to save a given metadata or not.
 *
 * Note that since this is an auxiliary argument, it won't be part of
 * the PDB arguments. By default, the value will be
 * gimp_export_comment().
 * Since: 3.0
 **/
void
gimp_save_procedure_set_support_comment (GimpSaveProcedure *procedure,
                                         gboolean           supports)
{
  g_return_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure));

  g_object_set (procedure,
                "supports-comment", supports,
                NULL);
}

/**
 * gimp_save_procedure_get_support_exif:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports Exif saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_exif (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_exif;
}

/**
 * gimp_save_procedure_get_support_iptc:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports IPTC saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_iptc (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_iptc;
}

/**
 * gimp_save_procedure_get_support_xmp:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports XMP saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_xmp (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_xmp;
}

/**
 * gimp_save_procedure_get_support_profile:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports ICC color profile saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_profile (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_profile;
}

/**
 * gimp_save_procedure_get_support_thumbnail:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports thumbnail saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_thumbnail (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_thumbnail;
}

/**
 * gimp_save_procedure_get_support_comment:
 * @procedure: a #GimpProcedure.
 *
 * Returns: %TRUE if @procedure supports comment saving.
 *
 * Since: 3.0
 **/
gboolean
gimp_save_procedure_get_support_comment (GimpSaveProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_SAVE_PROCEDURE (procedure), FALSE);

  return procedure->priv->supports_comment;
}
