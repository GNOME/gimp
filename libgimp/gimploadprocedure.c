/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimploadprocedure.c
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

#include "gimploadprocedure.h"
#include "gimppdb_pdb.h"


struct _GimpLoadProcedurePrivate
{
  GimpRunLoadFunc  run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;

  gboolean         handles_raw;
  gchar           *thumbnail_proc;
};


static void   gimp_load_procedure_constructed   (GObject              *object);
static void   gimp_load_procedure_finalize      (GObject              *object);

static void   gimp_load_procedure_install       (GimpProcedure        *procedure);
static GimpValueArray *
              gimp_load_procedure_run           (GimpProcedure        *procedure,
                                                 const GimpValueArray *args);
static GimpProcedureConfig *
              gimp_load_procedure_create_config (GimpProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (GimpLoadProcedure, gimp_load_procedure,
                            GIMP_TYPE_FILE_PROCEDURE)

#define parent_class gimp_load_procedure_parent_class


static void
gimp_load_procedure_class_init (GimpLoadProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GimpProcedureClass *procedure_class = GIMP_PROCEDURE_CLASS (klass);

  object_class->constructed      = gimp_load_procedure_constructed;
  object_class->finalize         = gimp_load_procedure_finalize;

  procedure_class->install       = gimp_load_procedure_install;
  procedure_class->run           = gimp_load_procedure_run;
  procedure_class->create_config = gimp_load_procedure_create_config;
}

static void
gimp_load_procedure_init (GimpLoadProcedure *procedure)
{
  procedure->priv = gimp_load_procedure_get_instance_private (procedure);
}

static void
gimp_load_procedure_constructed (GObject *object)
{
  GimpProcedure *procedure = GIMP_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  GIMP_PROC_ARG_FILE (procedure, "file",
                      "File",
                      "The file to load",
                      GIMP_PARAM_READWRITE);

  GIMP_PROC_VAL_IMAGE (procedure, "image",
                       "Image",
                       "Output image",
                       TRUE,
                       GIMP_PARAM_READWRITE);
}

static void
gimp_load_procedure_finalize (GObject *object)
{
  GimpLoadProcedure *procedure = GIMP_LOAD_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  g_clear_pointer (&procedure->priv->thumbnail_proc, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_load_procedure_install (GimpProcedure *procedure)
{
  GimpLoadProcedure *load_proc = GIMP_LOAD_PROCEDURE (procedure);
  GimpFileProcedure *file_proc = GIMP_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

  GIMP_PROCEDURE_CLASS (parent_class)->install (procedure);

  _gimp_pdb_set_file_proc_load_handler (gimp_procedure_get_name (procedure),
                                        gimp_file_procedure_get_extensions (file_proc),
                                        gimp_file_procedure_get_prefixes (file_proc),
                                        gimp_file_procedure_get_magics (file_proc));

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

  if (load_proc->priv->handles_raw)
    _gimp_pdb_set_file_proc_handles_raw (gimp_procedure_get_name (procedure));

  if (load_proc->priv->thumbnail_proc)
    _gimp_pdb_set_file_proc_thumbnail_loader (gimp_procedure_get_name (procedure),
                                              load_proc->priv->thumbnail_proc);
}

#define ARG_OFFSET 2

static GimpValueArray *
gimp_load_procedure_run (GimpProcedure        *procedure,
                         const GimpValueArray *args)
{
  GimpLoadProcedure *load_proc = GIMP_LOAD_PROCEDURE (procedure);
  GimpValueArray    *remaining;
  GimpValueArray    *return_values;
  GimpRunMode        run_mode;
  GFile             *file;
  gint               i;

  run_mode = GIMP_VALUES_GET_ENUM (args, 0);
  file     = GIMP_VALUES_GET_FILE (args, 1);

  remaining = gimp_value_array_new (gimp_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < gimp_value_array_length (args); i++)
    {
      GValue *value = gimp_value_array_index (args, i);

      gimp_value_array_append (remaining, value);
    }

  return_values = load_proc->priv->run_func (procedure,
                                             run_mode,
                                             file,
                                             remaining,
                                             load_proc->priv->run_data);

  gimp_value_array_unref (remaining);

  return return_values;
}

static GimpProcedureConfig *
gimp_load_procedure_create_config (GimpProcedure  *procedure,
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
 * gimp_load_procedure_new:
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
 * #GimpLoadProcedure is a #GimpProcedure subclass that makes it easier
 * to write file load procedures.
 *
 * It automatically adds the standard
 *
 * (#GimpRunMode, #GFile)
 *
 * arguments and the standard
 *
 * (#GimpImage)
 *
 * return value of a load procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via gimp_procedure_run(), it unpacks these standard
 * arguments and calls @run_func which is a #GimpRunLoadFunc. The
 * "args" #GimpValueArray of #GimpRunLoadFunc only contains
 * additionally added arguments.
 *
 * Returns: a new #GimpProcedure.
 *
 * Since: 3.0
 **/
GimpProcedure  *
gimp_load_procedure_new (GimpPlugIn      *plug_in,
                         const gchar     *name,
                         GimpPDBProcType  proc_type,
                         GimpRunLoadFunc  run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  GimpLoadProcedure *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (gimp_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != GIMP_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (GIMP_TYPE_LOAD_PROCEDURE,
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
 * gimp_load_procedure_set_handles_raw:
 * @procedure:   A #GimpLoadProcedure.
 * @handles_raw: The procedure's handles raw flag.
 *
 * Registers a load loader procedure as capable of handling raw
 * digital camera loads.
 *
 * Since: 3.0
 **/
void
gimp_load_procedure_set_handles_raw (GimpLoadProcedure *procedure,
                                     gint               handles_raw)
{
  g_return_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure));

  procedure->priv->handles_raw = handles_raw;
}

/**
 * gimp_load_procedure_get_handles_raw:
 * @procedure: A #GimpLoadProcedure.
 *
 * Returns: The procedure's handles raw flag as set with
 *          gimp_load_procedure_set_handles_raw().
 *
 * Since: 3.0
 **/
gint
gimp_load_procedure_get_handles_raw (GimpLoadProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure), 0);

  return procedure->priv->handles_raw;
}

/**
 * gimp_load_procedure_set_thumbnail_loader:
 * @procedure:      A #GimpLoadProcedure.
 * @thumbnail_proc: The name of the thumbnail load procedure.
 *
 * Associates a thumbnail loader with a file load procedure.
 *
 * Some file formats allow for embedded thumbnails, other file formats
 * contain a scalable image or provide the image data in different
 * resolutions. A file plug-in for such a format may register a
 * special procedure that allows GIMP to load a thumbnail preview of
 * the image. This procedure is then associated with the standard
 * load procedure using this function.
 *
 * Since: 3.0
 **/
void
gimp_load_procedure_set_thumbnail_loader (GimpLoadProcedure *procedure,
                                          const gchar       *thumbnail_proc)
{
  g_return_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure));

  g_free (procedure->priv->thumbnail_proc);
  procedure->priv->thumbnail_proc = g_strdup (thumbnail_proc);
}

/**
 * gimp_load_procedure_get_thumbnail_loader:
 * @procedure: A #GimpLoadProcedure.
 *
 * Returns: The procedure's thumbnail loader procedure as set with
 *          gimp_load_procedure_set_thumbnail_loader().
 *
 * Since: 3.0
 **/
const gchar *
gimp_load_procedure_get_thumbnail_loader (GimpLoadProcedure *procedure)
{
  g_return_val_if_fail (GIMP_IS_LOAD_PROCEDURE (procedure), NULL);

  return procedure->priv->thumbnail_proc;
}
