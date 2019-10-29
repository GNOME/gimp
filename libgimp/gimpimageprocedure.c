/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpimageprocedure.c
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

#include "gimpimageprocedure.h"


struct _GimpImageProcedurePrivate
{
  GimpRunImageFunc run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;
};


static void   gimp_image_procedure_constructed   (GObject              *object);
static void   gimp_image_procedure_finalize      (GObject              *object);

static GimpValueArray *
              gimp_image_procedure_run           (GimpProcedure        *procedure,
                                                  const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_image_procedure_create_config (GimpProcedure        *procedure,
                                                  GParamSpec          **args,
                                                  gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpImageProcedure, gimp_image_procedure,
                            GIMP_TYPE_PROCEDURE)

#define parent_class gimp_image_procedure_parent_class


static void
gimp_image_procedure_class_init (GimpImageProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_image_procedure_constructed;
  object_class->finalize         = gimp_image_procedure_finalize;

  procedure_class->run           = gimp_image_procedure_run;
  procedure_class->create_config = gimp_image_procedure_create_config;
}

static void
gimp_image_procedure_init (GimpImageProcedure *procedure)
{
  procedure->priv = gimp_image_procedure_get_instance_private (procedure);
}

static void
gimp_image_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GIMP_PROC_ARG_ENUM (procedure, "run-mode",
                      "Run mode",
                      "The run mode",
                      GIMP_TYPE_RUN_MODE,
                      GIMP_RUN_NONINTERACTIVE,
                      G_PARAM_READWRITE);

  GIMP_PROC_ARG_IMAGE (procedure, "image",
                       "Image",
                       "The input image",
                       FALSE,
                       G_PARAM_READWRITE);

  GIMP_PROC_ARG_DRAWABLE (procedure, "drawable",
                          "Drawable",
                          "The input drawable",
                          TRUE,
                          G_PARAM_READWRITE);
}

static void
gimp_image_procedure_finalize (GObject *object)
{
  GimpImageProcedure *procedure = GIMP_IMAGE_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 3

static GimpValueArray *
gimp_image_procedure_run (GimpProcedure        *procedure,
                          const GimpValueArray *args)
{
  GimpImageProcedure *image_proc = GIMP_IMAGE_PROCEDURE (procedure);
  GimpValueArray     *remaining;
  GimpValueArray     *return_values;
  GimpRunMode         run_mode;
  GimpImage          *image;
  GimpDrawable       *drawable;
  gint                i;

  run_mode = GIMP_VALUES_GET_ENUM     (args, 0);
  image    = GIMP_VALUES_GET_IMAGE    (args, 1);
  drawable = GIMP_VALUES_GET_DRAWABLE (args, 2);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  return_values = image_proc->priv->run_func (procedure,
                                              run_mode,
                                              image, drawable,
                                              remaining,
                                              image_proc->priv->run_data);

  gimp_value_array_unref (remaining);

  return return_values;
}

static GimpProcedureConfig *
gimp_image_procedure_create_config (GimpProcedure  *procedure,
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
 * gimp_image_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new image procedure named @name which will call @run_func
 * when invoked.
 *
 * See gimp_procedure_new() for information about @proc_type.
 *
 * #GimpImageProcedure is a #GimpProcedure subclass that makes it easier
 * to write standard plug-in procedures that operate on drawables.
 *
 * It automatically adds the standard
 *
 * (#GimpRunMode, #GimpImage, #GimpDrawable)
 *
 * arguments of an image procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpRunImageFunc. The
 * "args" #GimpValueArray of #GimpRunImageFunc only contains
 * additionally added arguments.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_image_procedure_new (GimpPlugIn       *plug_in,
                          const gchar      *name,
                          GimpPDBProcType   proc_type,
                          GimpRunImageFunc  run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  GimpImageProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_IMAGE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}
