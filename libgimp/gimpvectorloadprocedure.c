/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpvectorloadprocedure.c
 * Copyright (C) 2024 Jehan
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

#include <math.h>

#include "gimp.h"

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimpvectorloadprocedure.h"
#include "gimppdb_pdb.h"
#include "gimpplugin-private.h"
#include "gimpprocedureconfig-private.h"

#include "libgimp-intl.h"


#define GIMP_VECTOR_LOAD_DEFAULT_PIXEL_DENSITY 300.0

/**
 * GimpVectorLoadProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write load procedures
 * for vector image formats.
 *
 * It automatically adds the standard arguments:
 * ([enum@RunMode], [iface@Gio.File], int width, int height)
 *
 * and the standard return value: ( [class@Image] )
 *
 * It is possible to add additional arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * [class@ProcedureConfig] of [callback@Gimp.RunVectorLoadFunc] contains
 * additionally added arguments but also the arguments added by this class.
 */


struct _GimpVectorLoadProcedure
{
  GimpLoadProcedure      parent_instance;

  GimpRunVectorLoadFunc  run_func;
  gpointer               run_data;
  GDestroyNotify         run_data_destroy;

  GimpExtractVectorFunc  extract_func;
  gpointer               extract_data;
  GDestroyNotify         extract_data_destroy;
};

static void                  gimp_vector_load_procedure_constructed   (GObject              *object);
static void                  gimp_vector_load_procedure_finalize      (GObject              *object);

static void                  gimp_vector_load_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray      * gimp_vector_load_procedure_run           (GimpProcedure        *procedure,
                                                                       const GimpValueArray *args);


G_DEFINE_FINAL_TYPE (GimpVectorLoadProcedure, gimp_vector_load_procedure, GIMP_TYPE_LOAD_PROCEDURE)

#define parent_class gimp_vector_load_procedure_parent_class


static void
gimp_vector_load_procedure_class_init (GimpVectorLoadProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_vector_load_procedure_constructed;
  object_class->finalize         = gimp_vector_load_procedure_finalize;

  procedure_class->install       = gimp_vector_load_procedure_install;
  procedure_class->run           = gimp_vector_load_procedure_run;
}

static void
gimp_vector_load_procedure_init (GimpVectorLoadProcedure *procedure)
{
}

static void
gimp_vector_load_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_int_argument (procedure, "width",
                                   _("_Width (pixels)"),
                                     "Width (in pixels) to load the image in. "
                                     "(0 for the corresponding width per native ratio)",
                                   0, GIMP_MAX_IMAGE_SIZE, 0,
                                   GIMP_PARAM_READWRITE);

  gimp_procedure_add_int_argument (procedure, "height",
                                   _("_Height (pixels)"),
                                     "Height (in pixels) to load the image in. "
                                     "(0 for the corresponding height per native ratio)",
                                   0, GIMP_MAX_IMAGE_SIZE, 0,
                                   GIMP_PARAM_READWRITE);

  gimp_procedure_add_boolean_argument (procedure, "keep-ratio",
                                       _("_Keep aspect ratio"),
                                       _("Force dimensions with aspect ratio"),
                                       TRUE,
                                       G_PARAM_READWRITE);
  gimp_procedure_add_boolean_argument (procedure, "prefer-native-dimensions",
                                       _("_Prefer native dimensions"),
                                       _("Load and use dimensions from source file"),
                                       FALSE,
                                       G_PARAM_READWRITE);

  /* Note: the "pixel-density" is saved in pixels per inch. "physical-unit"
   * property is only there for display.
   */
  gimp_procedure_add_double_aux_argument (procedure, "pixel-density",
                                          _("Resolu_tion"),
                                          _("Pixel Density: number of pixels per physical unit"),
                                          GIMP_MIN_RESOLUTION, GIMP_MAX_RESOLUTION,
                                          GIMP_VECTOR_LOAD_DEFAULT_PIXEL_DENSITY,
                                          G_PARAM_READWRITE);
  gimp_procedure_add_unit_aux_argument (procedure, "physical-unit",
                                        _("Unit"),
                                        _("Physical unit"),
                                        FALSE, FALSE, gimp_unit_inch (),
                                        G_PARAM_READWRITE);
}

static void
gimp_vector_load_procedure_finalize (GObject *object)
{
  GimpVectorLoadProcedure *procedure = GIMP_VECTOR_LOAD_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  if (procedure->extract_data_destroy)
    procedure->extract_data_destroy (procedure->extract_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_vector_load_procedure_install (GimpProcedure *procedure)
{
  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  _gimp_pdb_set_file_proc_handles_vector (gimp_procedure_get_name (procedure));
}

static GimpValueArray *
gimp_vector_load_procedure_run (GimpProcedure        *procedure,
                                const GimpValueArray *args)
{
  GimpPlugIn              *plug_in;
  GimpVectorLoadProcedure *load_proc = GIMP_VECTOR_LOAD_PROCEDURE (procedure);
  GimpValueArray          *remaining;
  GimpValueArray          *return_values;
  GimpProcedureConfig     *config;
  GimpImage               *image                = NULL;
  GimpMetadata            *metadata             = NULL;
  gchar                   *mimetype             = NULL;
  GError                  *error                = NULL;
  GimpVectorLoadData       extracted_dimensions = { 0 };
  gpointer                 data_for_run         = NULL;
  GDestroyNotify           data_for_run_destroy = NULL;
  GimpMetadataLoadFlags    flags                = GIMP_METADATA_LOAD_ALL;
  GimpPDBStatusType        status               = GIMP_PDB_EXECUTION_ERROR;
  GimpRunMode              run_mode;
  GFile                   *file;
  gint                     width;
  gint                     height;
  gdouble                  resolution;
  gint                     arg_offset = 0;
  gint                     i;

  run_mode = GIMP_VALUES_GET_ENUM (args, arg_offset++);
  file     = GIMP_VALUES_GET_FILE (args, arg_offset++);
  width    = GIMP_VALUES_GET_INT  (args, arg_offset);
  height   = GIMP_VALUES_GET_INT  (args, arg_offset + 1);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - arg_offset);

  for (i = arg_offset; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config   = _gimp_procedure_create_run_config (procedure);
  mimetype = (gchar *) gimp_file_procedure_get_mime_types (GIMP_FILE_PROCEDURE (procedure));

  if (mimetype != NULL)
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

      metadata = gimp_metadata_load_from_file (file, NULL);
      g_free (mimetype);
    }
  else
    {
      flags = GIMP_METADATA_LOAD_NONE;
    }

  if (metadata == NULL)
    metadata = gimp_metadata_new ();

  _gimp_procedure_config_begin_run (config, image, run_mode, remaining);

  g_object_get (config, "pixel-density", &resolution, NULL);

  if (load_proc->extract_func)
    {
      gboolean extract_success;

      extract_success = load_proc->extract_func (procedure, run_mode, file, metadata, config,
                                                 &extracted_dimensions,
                                                 &data_for_run, &data_for_run_destroy,
                                                 load_proc->extract_data, &error);

      if (extract_success)
        {
          gdouble  default_pixel_width     = 0.0;
          gdouble  default_pixel_height    = 0.0;
          gdouble  default_resolution      = GIMP_VECTOR_LOAD_DEFAULT_PIXEL_DENSITY;
          gdouble  res_pixel_width         = 0.0;
          gdouble  res_pixel_height        = 0.0;
          gboolean keep_ratio              = TRUE;
          gboolean prefer_native_dimension = FALSE;

          g_object_get (config,
                        "keep-ratio",               &keep_ratio,
                        "prefer-native-dimensions", &prefer_native_dimension,
                        NULL);

          if (extracted_dimensions.width != 0 && extracted_dimensions.height != 0)
            {
              if (extracted_dimensions.pixel_density > 0.0 &&
                  extracted_dimensions.density_unit != gimp_unit_percent () &&
                  extracted_dimensions.density_unit != gimp_unit_pixel ())
                {
                  if (extracted_dimensions.density_unit == gimp_unit_inch ())
                    {
                      resolution = extracted_dimensions.pixel_density;
                    }
                  else
                    {
                      resolution = extracted_dimensions.pixel_density / gimp_unit_get_factor (extracted_dimensions.density_unit);
                    }

                  default_resolution = resolution;
                }

              if (extracted_dimensions.width_unit == gimp_unit_pixel () ||
                  /* This is kinda bogus, but it at least gives ratio data. */
                  extracted_dimensions.width_unit == gimp_unit_percent ())
                {
                  default_pixel_width = extracted_dimensions.width;
                  res_pixel_width     = (gint) default_pixel_width;
                }
              else
                {
                  gdouble default_inch_width;

                  default_inch_width = extracted_dimensions.width / gimp_unit_get_factor (extracted_dimensions.width_unit);

                  default_pixel_width = default_inch_width * default_resolution;
                  res_pixel_width     = (gint) ceil (default_inch_width * resolution);
                }

              if (extracted_dimensions.height_unit == gimp_unit_pixel () ||
                  extracted_dimensions.height_unit == gimp_unit_percent ())
                {
                  default_pixel_height = extracted_dimensions.height;
                  res_pixel_height     = (gint) default_pixel_height;
                }
              else
                {
                  gdouble default_inch_height;

                  default_inch_height = extracted_dimensions.height / gimp_unit_get_factor (extracted_dimensions.height_unit);

                  default_pixel_height = default_inch_height * default_resolution;
                  res_pixel_height     = (gint) ceil (default_inch_height * resolution);
                }
            }

          if (default_pixel_width != 0.0 && default_pixel_height != 0.0)
            {
              GParamSpec *spec;

              if (prefer_native_dimension || (width == 0 && height == 0))
                {
                  width  = (gint) ceil (res_pixel_width);
                  height = (gint) ceil (res_pixel_height);
                }
              else if (keep_ratio || width == 0 || height == 0)
                {
                  gdouble ratio;

                  ratio = default_pixel_width / default_pixel_height;

                  if (width == 0)
                    width = (gint) ceil (ratio * height);
                  else if (height == 0)
                    height = (gint) ceil (width / ratio);
                  else if (ratio * height <= width)
                    width = (gint) ceil (ratio * height);
                  else
                    height = (gint) ceil (width / ratio);
                }

              spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "width");
              G_PARAM_SPEC_INT (spec)->default_value = (gint) ceil (default_pixel_width);

              spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "height");
              G_PARAM_SPEC_INT (spec)->default_value = (gint) ceil (default_pixel_height);

              if (default_resolution != 0.0)
                {
                  spec = g_object_class_find_property (G_OBJECT_GET_CLASS (config), "pixel-density");
                  G_PARAM_SPEC_DOUBLE (spec)->default_value = default_resolution;
                }
            }

          if (width == 0 || height == 0)
            {
              /* If width or height are still 0, something was very wrong. */
              g_set_error_literal (&error, GIMP_PLUG_IN_ERROR, 0,
                                   _("dimensions could neither be extracted nor computed "
                                     "from the vector image's data."));
            }
        }

      g_prefix_error_literal (&error, _("Vector image loading plug-in failed: "));
    }

  /* One or both dimensions are still zero at this point. */
  if (width == 0 || height == 0)
    {
      if (run_mode == GIMP_RUN_INTERACTIVE)
        {
          /* These values are utter-bogus but we just some value to start and
           * let people select proper values in the interactive dialog.
           */
          if (width != 0)
            height = width;
          else if (height != 0)
            width = width;
          else
            width = height = 500;
        }
      else if (! error)
        {
          /* Except for interactive case where we can always ask interactively,
           * non-interactive (including "with-last-vals") require valid values.
           */
          g_set_error_literal (&error, GIMP_PLUG_IN_ERROR, 0,
                               _("Dimensions cannot be 0 and no native dimensions "
                                 "could be extracted from the vector image."));
        }
    }

  if (error)
    {
      return_values = gimp_procedure_new_return_values (procedure, status, error);
    }
  else
    {
      g_object_set (config,
                    "width",         width,
                    "height",        height,
                    "pixel-density", resolution,
                    NULL);
      /* In future, when we'll have vector layers, a vector load proc should be
       * able to advertize when it can return a vector layer, and when so, we
       * can even bypass the dialog (by running non-interactively) and just use
       * the defaults, unless it's all bogus.
       */
      return_values = load_proc->run_func (procedure,
                                           run_mode,
                                           file, width, height,
                                           extracted_dimensions,
                                           metadata, &flags,
                                           config,
                                           data_for_run,
                                           load_proc->run_data);

      if (return_values != NULL                       &&
          gimp_value_array_length (return_values) > 0 &&
          G_VALUE_HOLDS_ENUM (gimp_value_array_index (return_values, 0)))
        status = GIMP_VALUES_GET_ENUM (return_values, 0);
    }

  _gimp_procedure_config_end_run (config, status);

  if (data_for_run_destroy)
    data_for_run_destroy (data_for_run);

  if (status == GIMP_PDB_SUCCESS)
    {
      if (gimp_value_array_length (return_values) < 2 ||
          ! GIMP_VALUE_HOLDS_IMAGE (gimp_value_array_index (return_values, 1)))
        {
          status = GIMP_PDB_EXECUTION_ERROR;
          g_set_error (&error, GIMP_PLUG_IN_ERROR, 0,
                       _("This file loading plug-in returned SUCCESS as a status without an image. "
                         "This is a bug in the plug-in code. Contact the plug-in developer."));
          gimp_value_array_unref (return_values);
          return_values = gimp_procedure_new_return_values (procedure, status, error);
        }
      else
        {
          image = GIMP_VALUES_GET_IMAGE (return_values, 1);
        }
    }

  if (image != NULL && metadata != NULL && flags != GIMP_METADATA_LOAD_NONE)
    gimp_image_metadata_load_finish (image, NULL, metadata, flags);

  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = gimp_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _gimp_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the GimpProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
  g_clear_object (&metadata);
  gimp_value_array_unref (remaining);

  return return_values;
}


/*  public functions  */

/**
 * gimp_vector_load_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new load procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * Returns: (transfer full): a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_vector_load_procedure_new (GimpPlugIn            *plug_in,
                                const gchar           *name,
                                GimpPDBProcType        proc_type,
                                GimpExtractVectorFunc  extract_func,
                                gpointer               extract_data,
                                GDestroyNotify         extract_data_destroy,
                                GimpRunVectorLoadFunc  run_func,
                                gpointer               run_data,
                                GDestroyNotify         run_data_destroy)
{
  GimpVectorLoadProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_VECTOR_LOAD_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func             = run_func;
  procedure->run_data             = run_data;
  procedure->run_data_destroy     = run_data_destroy;
  procedure->extract_func         = extract_func;
  procedure->extract_data         = extract_data;
  procedure->extract_data_destroy = extract_data_destroy;

  return GIMP_PROCEDURE (procedure);
}

/**
 * gimp_vector_load_procedure_extract_dimensions:
 * @procedure:    the associated #GimpVectorLoadProcedure.
 * @file:         a [iface@Gio.File] which can be processed by @procedure.
 * @data: (out):  the returned dimension data.
 * @error: (out): the #GError in case of error.
 *
 * Extracts native or suggested dimensions from @file, which must be a vector
 * file in the right format supported by @procedure. It is considered a
 * programming error to pass a file of invalid format.
 *
 * Returns: %TRUE if dimensions could be extracted.
 */
gboolean
gimp_vector_load_procedure_extract_dimensions (GimpVectorLoadProcedure  *procedure,
                                               GFile                    *file,
                                               GimpVectorLoadData       *data,
                                               GError                  **error)
{
  gboolean extract_success = FALSE;

  g_return_val_if_fail (GIMP_IS_VECTOR_LOAD_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (data != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  if (procedure->extract_func)
    {
      gchar          *mimetype             = NULL;
      GimpMetadata   *metadata             = NULL;
      gpointer        data_for_run         = NULL;
      GDestroyNotify  data_for_run_destroy = NULL;

      mimetype = (gchar *) gimp_file_procedure_get_mime_types (GIMP_FILE_PROCEDURE (procedure));

      if (mimetype != NULL)
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

          metadata = gimp_metadata_load_from_file (file, NULL);
          g_free (mimetype);
        }

      if (metadata == NULL)
        metadata = gimp_metadata_new ();

      extract_success = procedure->extract_func (GIMP_PROCEDURE (procedure),
                                                 GIMP_RUN_NONINTERACTIVE,
                                                 file, metadata, NULL,
                                                 data, &data_for_run, &data_for_run_destroy,
                                                 procedure->extract_data, error);

      if (data_for_run_destroy)
        data_for_run_destroy (data_for_run);

      g_clear_object (&metadata);
    }

  return extract_success;
}
