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

#include "libgimpbase/gimpwire.h" /* FIXME kill this include */

#include "gimpimageprocedure.h"
#include "gimpplugin-private.h"
#include "gimpprocedureconfig-private.h"


/**
 * GimpImageProcedure:
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


struct _GimpImageProcedure
{
  GimpProcedure    parent_instance;

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
static gboolean
            gimp_image_procedure_set_sensitivity (GimpProcedure        *procedure,
                                                  gint                  sensitivity_mask);


G_DEFINE_TYPE (GimpImageProcedure, gimp_image_procedure, GIMP_TYPE_PROCEDURE)

#define parent_class gimp_image_procedure_parent_class


static void
gimp_image_procedure_class_init (GimpImageProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed        = gimp_image_procedure_constructed;
  object_class->finalize           = gimp_image_procedure_finalize;

  procedure_class->run             = gimp_image_procedure_run;
  procedure_class->create_config   = gimp_image_procedure_create_config;
  procedure_class->set_sensitivity = gimp_image_procedure_set_sensitivity;
}

static void
gimp_image_procedure_init (GimpImageProcedure *procedure)
{
}

static void
gimp_image_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  gimp_procedure_add_enum_argument (procedure, "run-mode",
                                    "Run mode",
                                    "The run mode",
                                    GIMP_TYPE_RUN_MODE,
                                    GIMP_RUN_NONINTERACTIVE,
                                    G_PARAM_READWRITE);

  gimp_procedure_add_image_argument (procedure, "image",
                                     "Image",
                                     "The input image",
                                     FALSE,
                                     G_PARAM_READWRITE);

  gimp_procedure_add_int_argument (procedure, "num-drawables",
                                   "Number of drawables",
                                   "Number of input drawables",
                                   0, G_MAXINT, 1,
                                   G_PARAM_READWRITE);

  gimp_procedure_add_object_array_argument (procedure, "drawables",
                                            "Drawables",
                                            "The input drawables",
                                            GIMP_TYPE_DRAWABLE,
                                            G_PARAM_READWRITE | GIMP_PARAM_NO_VALIDATE);
}

static void
gimp_image_procedure_finalize (GObject *object)
{
  GimpImageProcedure *procedure = GIMP_IMAGE_PROCEDURE (object);

  if (procedure->run_data_destroy)
    procedure->run_data_destroy (procedure->run_data);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

#define ARG_OFFSET 4

static GimpValueArray *
gimp_image_procedure_run (GimpProcedure        *procedure,
                          const GimpValueArray *args)
{
  GimpPlugIn          *plug_in;
  GimpImageProcedure  *image_proc = GIMP_IMAGE_PROCEDURE (procedure);
  GimpPDBStatusType    status     = GIMP_PDB_EXECUTION_ERROR;
  GimpProcedureConfig *config;
  GimpValueArray      *remaining;
  GimpValueArray      *return_values;
  GimpRunMode          run_mode;
  GimpImage           *image;
  GimpDrawable       **drawables;
  gint                 n_drawables;
  gint                 i;

  run_mode    = GIMP_VALUES_GET_ENUM         (args, 0);
  image       = GIMP_VALUES_GET_IMAGE        (args, 1);
  n_drawables = GIMP_VALUES_GET_INT          (args, 2);
  drawables   = GIMP_VALUES_GET_OBJECT_ARRAY (args, 3);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  config = _gimp_procedure_create_run_config (procedure);
  _gimp_procedure_config_begin_run (config, image, run_mode, remaining);

  return_values = image_proc->run_func (procedure, run_mode,
                                        image, n_drawables, drawables,
                                        config, image_proc->run_data);

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
    g_printerr ("%s: ERROR: the GimpProcedureConfig object was refed "
                "by plug-in, it MUST NOT do that!\n", G_STRFUNC);

  g_object_unref (config);
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

static gboolean
gimp_image_procedure_set_sensitivity (GimpProcedure *procedure,
                                      gint           sensitivity_mask)
{
  GParamSpec *pspec;

  g_return_val_if_fail (GIMP_IS_IMAGE_PROCEDURE (procedure), FALSE);

  pspec = gimp_procedure_find_argument (procedure, "image");
  g_return_val_if_fail (pspec, FALSE);

  if (sensitivity_mask & GIMP_PROCEDURE_SENSITIVE_NO_IMAGE)
    pspec->flags |= GIMP_PARAM_NO_VALIDATE;
  else
    pspec->flags &= ~GIMP_PARAM_NO_VALIDATE;

  return TRUE;
}

/*  public functions  */

/**
 * gimp_image_procedure_new:
 * @plug_in:          a #GimpPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #GimpPDBProcType.
 * @run_func:         (closure run_data): the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (destroy run_data) (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new image procedure named @name which will call @run_func
 * when invoked.
 *
 * See [ctor@Procedure.new] for information about @proc_type.
 *
 * Returns: (transfer full): a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure *
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

  procedure->run_func         = run_func;
  procedure->run_data         = run_data;
  procedure->run_data_destroy = run_data_destroy;

  return GIMP_PROCEDURE (procedure);
}
