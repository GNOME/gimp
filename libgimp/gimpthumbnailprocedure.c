/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpthumbnailprocedure.c
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

#include "gimpplugin-private.h"
#include "gimpthumbnailprocedure.h"
#include "gimpprocedureconfig-private.h"


struct _GimpThumbnailProcedure
{
  GimpProcedure         parent_instance;

  GimpRunThumbnailFunc  run_func;
  gpointer              run_data;
  GDestroyNotify        run_data_destroy;
};


static void   gimp_thumbnail_procedure_constructed (GObject              *object);
static void   gimp_thumbnail_procedure_finalize    (GObject              *object);

static GimpValueArray *
              gimp_thumbnail_procedure_run         (GimpProcedure        *procedure,
                                                    const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_thumbnail_procedure_create_config
                                                   (GimpProcedure        *procedure,
                                                    GParamSpec          **args,
                                                    gint                  n_args);


G_DEFINE_TYPE (GimpThumbnailProcedure, gimp_thumbnail_procedure, GIMP_TYPE_PROCEDURE)

#define parent_class gimp_thumbnail_procedure_parent_class


static void
gimp_thumbnail_procedure_class_init (GimpThumbnailProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_thumbnail_procedure_constructed;
  object_class->finalize         = gimp_thumbnail_procedure_finalize;

  procedure_class->run           = gimp_thumbnail_procedure_run;
  procedure_class->create_config = gimp_thumbnail_procedure_create_config;
}

static void
gimp_thumbnail_procedure_init (GimpThumbnailProcedure *procedure)
{
}

static void
gimp_thumbnail_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_file_argument (procedure, "file",
                                    "File",
                                    "The file to load the thumbnail from",
                                    GIMP_FILE_CHOOSER_ACTION_OPEN,
                                    FALSE, NULL,
                                    GIMP_PARAM_READWRITE);

  gimp_procedure_add_int_argument (procedure, "thumb-size",
                                   "Thumb Size",
                                   "Preferred thumbnail size",
                                   16, 2014, 256,
                                   GIMP_PARAM_READWRITE);

  gimp_procedure_add_image_return_value (procedure, "image",
                                         "Image",
                                         "Thumbnail image",
                                         TRUE,
                                         GIMP_PARAM_READWRITE);

  gimp_procedure_add_int_return_value (procedure, "image-width",
                                       "Image width",
                                       "Width of the full-sized image (0 for unknown)",
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       GIMP_PARAM_READWRITE);

  gimp_procedure_add_int_return_value (procedure, "image-height",
                                       "Image height",
                                       "Height of the full-sized image (0 for unknown)",
                                       0, GIMP_MAX_IMAGE_SIZE, 0,
                                       GIMP_PARAM_READWRITE);

  gimp_procedure_add_enum_return_value (procedure, "image-type",
                                        "Image type",
                                        "Type of the image",
                                        GIMP_TYPE_IMAGE_TYPE,
                                        GIMP_RGB_IMAGE,
                                        GIMP_PARAM_READWRITE);

  gimp_procedure_add_int_return_value (procedure, "num-layers",
                                       "Num layers",
                                       "Number of layers in the image",
                                       1, G_MAXINT, 1,
                                       GIMP_PARAM_READWRITE);
}

static void
gimp_thumbnail_procedure_finalize (GObject *object)
{
  GimpThumbnailProcedure *procedure = GIMP_THUMBNAIL_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 2

static GimpValueArray *
gimp_thumbnail_procedure_run (GimpProcedure        *procedure,
                              const GimpValueArray *args)
{
  GimpPlugIn             *plug_in;
  GimpThumbnailProcedure *thumbnail_proc = GIMP_THUMBNAIL_PROCEDURE (procedure);
  GimpValueArray         *remaining;
  GimpValueArray         *return_values;
  GimpProcedureConfig    *config;
  GFile                  *file;
  gint                    size;
  GimpPDBStatusType       status = GIMP_PDB_EXECUTION_ERROR;
  gint                    i;

  file = GIMP_VALUES_GET_FILE (args, 0);
  size = GIMP_VALUES_GET_INT  (args, 1);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config = _gimp_procedure_create_run_config (procedure);
  _gimp_procedure_config_begin_run (config, NULL, GIMP_RUN_NONINTERACTIVE, remaining, NULL);
  gimp_value_array_unref (remaining);

  return_values = thumbnail_proc->run_func (procedure, file, size,
                                            config, thumbnail_proc->run_data);

  if (return_values != NULL                       &&
      gimp_value_array_length (return_values) > 0 &&
      G_VALUE_HOLDS_ENUM (gimp_value_array_index (return_values, 0)))
    status = GIMP_VALUES_GET_ENUM (return_values, 0);

  _gimp_procedure_config_end_run (config, status);
  /* This is debug printing to help plug-in developers figure out best
   * practices.
   */
  plug_in = gimp_procedure_get_plug_in (procedure);
  if (G_OBJECT (config)->ref_count > 1 &&
      _gimp_plug_in_manage_memory_manually (plug_in))
    g_printerr ("%s: ERROR: the GimpThumbnailProcedure config object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);

  return return_values;
}

static GimpProcedureConfig *
gimp_thumbnail_procedure_create_config (GimpProcedure  *procedure,
                                        GParamSpec    **args,
                                        gint            n_args)
{
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


/*  public functions  */

/**
 * gimp_thumbnail_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new thumbnail procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * #GimpThumbnailProcedure is a #GimpProcedure subclass that makes it easier
 * to write file thumbnail procedures.
 *
 * It automatically adds the standard
 *
 * (#GFile, size)
 *
 * arguments and the standard
 *
 * (#GimpImage, image-width, image-height, #GimpImageType, num-layers)
 *
 * return value of a thumbnail procedure. It is possible to add
 * additional arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpRunThumbnailFunc. The
 * "args" #GimpValueArray of #GimpRunThumbnailFunc only contains
 * additionally added arguments.
 *
 * #GimpRunThumbnailFunc must gimp_value_array_truncate() the returned
 * #GimpValueArray to the number of return values it actually uses.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_thumbnail_procedure_new (GimpPlugIn           *plug_in,
                              const gchar          *name,
                              GimpPDBProcType       proc_type,
                              GimpRunThumbnailFunc  run_func,
                              gpointer              run_data,
                              GDestroyNotify        run_data_destroy)
{
  GimpThumbnailProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_PERSISTENT, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_THUMBNAIL_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}
