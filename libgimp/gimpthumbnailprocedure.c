/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmathumbnailprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@ligma.org>
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

#include "ligma.h"

#include "ligmathumbnailprocedure.h"


struct _LigmaThumbnailProcedurePrivate
{
  LigmaRunThumbnailFunc  run_func;
  gpointer              run_data;
  GDestroyNotify        run_data_destroy;
};


static void   ligma_thumbnail_procedure_constructed (GObject              *object);
static void   ligma_thumbnail_procedure_finalize    (GObject              *object);

static LigmaValueArray *
              ligma_thumbnail_procedure_run         (LigmaProcedure        *procedure,
                                                    const LigmaValueArray *args);
static LigmaProcedureConfig *
              ligma_thumbnail_procedure_create_config
                                                   (LigmaProcedure        *procedure,
                                                    GParamSpec          **args,
                                                    gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaThumbnailProcedure, ligma_thumbnail_procedure,
                            LIGMA_TYPE_PROCEDURE)

#define parent_class ligma_thumbnail_procedure_parent_class


static void
ligma_thumbnail_procedure_class_init (LigmaThumbnailProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaProcedureClass *procedure_class = LIGMA_PROCEDURE_CLASS (klass);

  object_class->constructed      = ligma_thumbnail_procedure_constructed;
  object_class->finalize         = ligma_thumbnail_procedure_finalize;

  procedure_class->run           = ligma_thumbnail_procedure_run;
  procedure_class->create_config = ligma_thumbnail_procedure_create_config;
}

static void
ligma_thumbnail_procedure_init (LigmaThumbnailProcedure *procedure)
{
  procedure->priv = ligma_thumbnail_procedure_get_instance_private (procedure);
}

static void
ligma_thumbnail_procedure_constructed (GObject *object)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  LIGMA_PROC_ARG_FILE (procedure, "file",
                      "File",
                      "The file to load the thumbnail from",
                      LIGMA_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "thumb-size",
                     "Thumb Size",
                     "Preferred thumbnail size",
                     16, 2014, 256,
                     LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_IMAGE (procedure, "image",
                       "Image",
                       "Thumbnail image",
                       TRUE,
                       LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_INT (procedure, "image-width",
                     "Image width",
                     "Width of the full-sized image (0 for unknown)",
                     0, LIGMA_MAX_IMAGE_SIZE, 0,
                     LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_INT (procedure, "image-height",
                     "Image height",
                     "Height of the full-sized image (0 for unknown)",
                     0, LIGMA_MAX_IMAGE_SIZE, 0,
                     LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_ENUM (procedure, "image-type",
                      "Image type",
                      "Type of the image",
                      LIGMA_TYPE_IMAGE_TYPE,
                      LIGMA_RGB_IMAGE,
                      LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_INT (procedure, "num-layers",
                     "Num layers",
                     "Number of layers in the image",
                     1, G_MAXINT, 1,
                     LIGMA_PARAM_READWRITE);
}

static void
ligma_thumbnail_procedure_finalize (GObject *object)
{
  LigmaThumbnailProcedure *procedure = LIGMA_THUMBNAIL_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 2

static LigmaValueArray *
ligma_thumbnail_procedure_run (LigmaProcedure        *procedure,
                              const LigmaValueArray *args)
{
  LigmaThumbnailProcedure *thumbnail_proc = LIGMA_THUMBNAIL_PROCEDURE (procedure);
  LigmaValueArray         *remaining;
  LigmaValueArray         *return_values;
  GFile                  *file;
  gint                    size;
  gint                    i;

  file = LIGMA_VALUES_GET_FILE (args, 0);
  size = LIGMA_VALUES_GET_INT  (args, 1);

  remaining = ligma_value_array_new (ligma_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < ligma_value_array_length (args); i++)
    {
      GValue *value = ligma_value_array_index (args, i);

      ligma_value_array_append (remaining, value);
    }

  return_values = thumbnail_proc->priv->run_func (procedure,
                                                  file,
                                                  size,
                                                  remaining,
                                                  thumbnail_proc->priv->run_data);

  ligma_value_array_unref (remaining);

  return return_values;
}

static LigmaProcedureConfig *
ligma_thumbnail_procedure_create_config (LigmaProcedure  *procedure,
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

  return LIGMA_PROCEDURE_CLASS (parent_class)->create_config (procedure,
                                                             args,
                                                             n_args);
}


/*  public functions  */

/**
 * ligma_thumbnail_procedure_new:
 * @plug_in:          a #LigmaPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #LigmaPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new thumbnail procedure named @name which will call @run_func
 * when invoked.
 *
 * See ligma_procedure_new() for information about @proc_type.
 *
 * #LigmaThumbnailProcedure is a #LigmaProcedure subclass that makes it easier
 * to write file thumbnail procedures.
 *
 * It automatically adds the standard
 *
 * (#GFile, size)
 *
 * arguments and the standard
 *
 * (#LigmaImage, image-width, image-height, #LigmaImageType, num-layers)
 *
 * return value of a thumbnail procedure. It is possible to add
 * additional arguments.
 *
 * When invoked via ligma_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #LigmaRunThumbnailFunc. The
 * "args" #LigmaValueArray of #LigmaRunThumbnailFunc only contains
 * additionally added arguments.
 *
 * #LigmaRunThumbnailFunc must ligma_value_array_truncate() the returned
 * #LigmaValueArray to the number of return values it actually uses.
 *
 * Returns: a new #LigmaProcedure.
 *
 * Since: 3.0
 **/
LigmaProcedure  *
ligma_thumbnail_procedure_new (LigmaPlugIn           *plug_in,
                              const gchar          *name,
                              LigmaPDBProcType       proc_type,
                              LigmaRunThumbnailFunc  run_func,
                              gpointer              run_data,
                              GDestroyNotify        run_data_destroy)
{
  LigmaThumbnailProcedure *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (LIGMA_TYPE_THUMBNAIL_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return LIGMA_PROCEDURE (procedure);
}
