/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaimageprocedure.c
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

#include "ligmaimageprocedure.h"


/**
 * LigmaImageProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write standard plug-in
 * procedures that operate on drawables.
 *
 * It automatically adds the standard
 *
 * ( [enum@RunMode], [class@Image], [class@Drawable] )
 *
 * arguments of an image procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * "args" [struct@ValueArray] of [callback@RunImageFunc] only contains
 * additionally added arguments.
 */


struct _LigmaImageProcedurePrivate
{
  LigmaRunImageFunc run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;
};


static void   ligma_image_procedure_constructed   (GObject              *object);
static void   ligma_image_procedure_finalize      (GObject              *object);

static LigmaValueArray *
              ligma_image_procedure_run           (LigmaProcedure        *procedure,
                                                  const LigmaValueArray *args);
static LigmaProcedureConfig *
              ligma_image_procedure_create_config (LigmaProcedure        *procedure,
                                                  GParamSpec          **args,
                                                  gint                  n_args);
static gboolean
            ligma_image_procedure_set_sensitivity (LigmaProcedure        *procedure,
                                                  gint                  sensitivity_mask);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaImageProcedure, ligma_image_procedure,
                            LIGMA_TYPE_PROCEDURE)

#define parent_class ligma_image_procedure_parent_class


static void
ligma_image_procedure_class_init (LigmaImageProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaProcedureClass *procedure_class = LIGMA_PROCEDURE_CLASS (klass);

  object_class->constructed        = ligma_image_procedure_constructed;
  object_class->finalize           = ligma_image_procedure_finalize;

  procedure_class->run             = ligma_image_procedure_run;
  procedure_class->create_config   = ligma_image_procedure_create_config;
  procedure_class->set_sensitivity = ligma_image_procedure_set_sensitivity;
}

static void
ligma_image_procedure_init (LigmaImageProcedure *procedure)
{
  procedure->priv = ligma_image_procedure_get_instance_private (procedure);
}

static void
ligma_image_procedure_constructed (GObject *object)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  LIGMA_PROC_ARG_ENUM (procedure, "run-mode",
                      "Run mode",
                      "The run mode",
                      LIGMA_TYPE_RUN_MODE,
                      LIGMA_RUN_NONINTERACTIVE,
                      G_PARAM_READWRITE);

  LIGMA_PROC_ARG_IMAGE (procedure, "image",
                       "Image",
                       "The input image",
                       FALSE,
                       G_PARAM_READWRITE);

  LIGMA_PROC_ARG_INT (procedure, "num-drawables",
                     "Number of drawables",
                     "Number of input drawables",
                     0, G_MAXINT, 1,
                     G_PARAM_READWRITE);

  LIGMA_PROC_ARG_OBJECT_ARRAY (procedure, "drawables",
                              "Drawables",
                              "The input drawables",
                              LIGMA_TYPE_DRAWABLE,
                              G_PARAM_READWRITE | LIGMA_PARAM_NO_VALIDATE);
}

static void
ligma_image_procedure_finalize (GObject *object)
{
  LigmaImageProcedure *procedure = LIGMA_IMAGE_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 4

static LigmaValueArray *
ligma_image_procedure_run (LigmaProcedure        *procedure,
                          const LigmaValueArray *args)
{
  LigmaImageProcedure *image_proc = LIGMA_IMAGE_PROCEDURE (procedure);
  LigmaValueArray     *remaining;
  LigmaValueArray     *return_values;
  LigmaRunMode         run_mode;
  LigmaImage          *image;
  LigmaDrawable      **drawables;
  gint                n_drawables;
  gint                i;

  run_mode    = LIGMA_VALUES_GET_ENUM         (args, 0);
  image       = LIGMA_VALUES_GET_IMAGE        (args, 1);
  n_drawables = LIGMA_VALUES_GET_INT          (args, 2);
  drawables   = LIGMA_VALUES_GET_OBJECT_ARRAY (args, 3);

  remaining = ligma_value_array_new (ligma_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < ligma_value_array_length (args); i++)
    {
      GValue *value = ligma_value_array_index (args, i);

      ligma_value_array_append (remaining, value);
    }

  return_values = image_proc->priv->run_func (procedure,
                                              run_mode,
                                              image, n_drawables, drawables,
                                              remaining,
                                              image_proc->priv->run_data);

  ligma_value_array_unref (remaining);

  return return_values;
}

static LigmaProcedureConfig *
ligma_image_procedure_create_config (LigmaProcedure  *procedure,
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

static gboolean
ligma_image_procedure_set_sensitivity (LigmaProcedure *procedure,
                                      gint           sensitivity_mask)
{
  GParamSpec *pspec;

  g_return_val_if_fail (LIGMA_IS_IMAGE_PROCEDURE (procedure), FALSE);

  pspec = ligma_procedure_find_argument (procedure, "image");
  g_return_val_if_fail (pspec, FALSE);

  if (sensitivity_mask & LIGMA_PROCEDURE_SENSITIVE_NO_IMAGE)
    pspec->flags |= LIGMA_PARAM_NO_VALIDATE;
  else
    pspec->flags &= ~LIGMA_PARAM_NO_VALIDATE;

  return TRUE;
}

/*  public functions  */

/**
 * ligma_image_procedure_new:
 * @plug_in:          a #LigmaPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #LigmaPDBProcType.
 * @run_func:         (closure run_data): the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (destroy run_data) (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new image procedure named @name which will call @run_func
 * when invoked.
 *
 * See [ctor@Procedure.new] for information about @proc_type.
 *
 * Returns: (transfer full): a new #LigmaProcedure.
 *
 * Since: 3.0
 **/
LigmaProcedure  *
ligma_image_procedure_new (LigmaPlugIn       *plug_in,
                          const gchar      *name,
                          LigmaPDBProcType   proc_type,
                          LigmaRunImageFunc  run_func,
                          gpointer          run_data,
                          GDestroyNotify    run_data_destroy)
{
  LigmaImageProcedure *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (LIGMA_TYPE_IMAGE_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return LIGMA_PROCEDURE (procedure);
}
