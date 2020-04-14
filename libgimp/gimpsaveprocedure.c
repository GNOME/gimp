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


struct _GimpSaveProcedurePrivate
{
  GimpRunSaveFunc run_func;
  gpointer        run_data;
  GDestroyNotify  run_data_destroy;
};


static void   gimp_save_procedure_constructed   (GObject              *object);
static void   gimp_save_procedure_finalize      (GObject              *object);

static void   gimp_save_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_save_procedure_run           (GimpProcedure        *procedure,
                                                 const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_save_procedure_create_config (GimpProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpSaveProcedure, gimp_save_procedure,
                            GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_save_procedure_parent_class


static void
gimp_save_procedure_class_init (GimpSaveProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_save_procedure_constructed;
  object_class->finalize         = gimp_save_procedure_finalize;

  procedure_class->install       = gimp_save_procedure_install;
  procedure_class->run           = gimp_save_procedure_run;
  procedure_class->create_config = gimp_save_procedure_create_config;
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
gimp_save_procedure_install (GimpProcedure *procedure)
{
  GimpFileProcedure *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

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

#define ARG_OFFSET 4

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
