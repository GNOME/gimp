/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaloadprocedure.c
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

#include "ligmaloadprocedure.h"
#include "ligmapdb_pdb.h"


/**
 * LigmaLoadProcedure:
 *
 * A [class@Procedure] subclass that makes it easier to write file load
 * procedures.
 *
 * It automatically adds the standard
 *
 * ( [enum@RunMode], [iface@Gio.File] )
 *
 * arguments and the standard
 *
 * ( [class@Image] )
 *
 * return value of a load procedure. It is possible to add additional
 * arguments.
 *
 * When invoked via [method@Procedure.run], it unpacks these standard
 * arguments and calls @run_func which is a [callback@RunImageFunc]. The
 * "args" [struct@ValueArray] of [callback@RunImageFunc] only contains
 * additionally added arguments.
 */


struct _LigmaLoadProcedurePrivate
{
  LigmaRunLoadFunc  run_func;
  gpointer         run_data;
  GDestroyNotify   run_data_destroy;

  gboolean         handles_raw;
  gchar           *thumbnail_proc;
};


static void   ligma_load_procedure_constructed   (GObject              *object);
static void   ligma_load_procedure_finalize      (GObject              *object);

static void   ligma_load_procedure_install       (LigmaProcedure        *procedure);
static LigmaValueArray *
              ligma_load_procedure_run           (LigmaProcedure        *procedure,
                                                 const LigmaValueArray *args);
static LigmaProcedureConfig *
              ligma_load_procedure_create_config (LigmaProcedure        *procedure,
                                                 GParamSpec          **args,
                                                 gint                  n_args);


G_DEFINE_TYPE_WITH_PRIVATE (LigmaLoadProcedure, ligma_load_procedure,
                            LIGMA_TYPE_FILE_PROCEDURE)

#define parent_class ligma_load_procedure_parent_class


static void
ligma_load_procedure_class_init (LigmaLoadProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  LigmaProcedureClass *procedure_class = LIGMA_PROCEDURE_CLASS (klass);

  object_class->constructed      = ligma_load_procedure_constructed;
  object_class->finalize         = ligma_load_procedure_finalize;

  procedure_class->install       = ligma_load_procedure_install;
  procedure_class->run           = ligma_load_procedure_run;
  procedure_class->create_config = ligma_load_procedure_create_config;
}

static void
ligma_load_procedure_init (LigmaLoadProcedure *procedure)
{
  procedure->priv = ligma_load_procedure_get_instance_private (procedure);
}

static void
ligma_load_procedure_constructed (GObject *object)
{
  LigmaProcedure *procedure = LIGMA_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  LIGMA_PROC_ARG_FILE (procedure, "file",
                      "File",
                      "The file to load",
                      LIGMA_PARAM_READWRITE);

  LIGMA_PROC_VAL_IMAGE (procedure, "image",
                       "Image",
                       "Output image",
                       TRUE,
                       LIGMA_PARAM_READWRITE);
}

static void
ligma_load_procedure_finalize (GObject *object)
{
  LigmaLoadProcedure *procedure = LIGMA_LOAD_PROCEDURE (object);

  if (procedure->priv->run_data_destroy)
    procedure->priv->run_data_destroy (procedure->priv->run_data);

  g_clear_pointer (&procedure->priv->thumbnail_proc, g_free);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
ligma_load_procedure_install (LigmaProcedure *procedure)
{
  LigmaLoadProcedure *load_proc = LIGMA_LOAD_PROCEDURE (procedure);
  LigmaFileProcedure *file_proc = LIGMA_FILE_PROCEDURE (procedure);
  const gchar       *mime_types;
  gint               priority;

  LIGMA_PROCEDURE_CLASS (parent_class)->install (procedure);

  _ligma_pdb_set_file_proc_load_handler (ligma_procedure_get_name (procedure),
                                        ligma_file_procedure_get_extensions (file_proc),
                                        ligma_file_procedure_get_prefixes (file_proc),
                                        ligma_file_procedure_get_magics (file_proc));

  if (ligma_file_procedure_get_handles_remote (file_proc))
    _ligma_pdb_set_file_proc_handles_remote (ligma_procedure_get_name (procedure));

  mime_types = ligma_file_procedure_get_mime_types (file_proc);
  if (mime_types)
    _ligma_pdb_set_file_proc_mime_types (ligma_procedure_get_name (procedure),
                                        mime_types);

  priority = ligma_file_procedure_get_priority (file_proc);
  if (priority != 0)
    _ligma_pdb_set_file_proc_priority (ligma_procedure_get_name (procedure),
                                      priority);

  if (load_proc->priv->handles_raw)
    _ligma_pdb_set_file_proc_handles_raw (ligma_procedure_get_name (procedure));

  if (load_proc->priv->thumbnail_proc)
    _ligma_pdb_set_file_proc_thumbnail_loader (ligma_procedure_get_name (procedure),
                                              load_proc->priv->thumbnail_proc);
}

#define ARG_OFFSET 2

static LigmaValueArray *
ligma_load_procedure_run (LigmaProcedure        *procedure,
                         const LigmaValueArray *args)
{
  LigmaLoadProcedure *load_proc = LIGMA_LOAD_PROCEDURE (procedure);
  LigmaValueArray    *remaining;
  LigmaValueArray    *return_values;
  LigmaRunMode        run_mode;
  GFile             *file;
  gint               i;

  run_mode = LIGMA_VALUES_GET_ENUM (args, 0);
  file     = LIGMA_VALUES_GET_FILE (args, 1);

  remaining = ligma_value_array_new (ligma_value_array_length (args) - ARG_OFFSET);

  for (i = ARG_OFFSET; i < ligma_value_array_length (args); i++)
    {
      GValue *value = ligma_value_array_index (args, i);

      ligma_value_array_append (remaining, value);
    }

  return_values = load_proc->priv->run_func (procedure,
                                             run_mode,
                                             file,
                                             remaining,
                                             load_proc->priv->run_data);

  ligma_value_array_unref (remaining);

  return return_values;
}

static LigmaProcedureConfig *
ligma_load_procedure_create_config (LigmaProcedure  *procedure,
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
 * ligma_load_procedure_new:
 * @plug_in:          a #LigmaPlugIn.
 * @name:             the new procedure's name.
 * @proc_type:        the new procedure's #LigmaPDBProcType.
 * @run_func:         the run function for the new procedure.
 * @run_data:         user data passed to @run_func.
 * @run_data_destroy: (nullable): free function for @run_data, or %NULL.
 *
 * Creates a new load procedure named @name which will call @run_func
 * when invoked.
 *
 * See ligma_procedure_new() for information about @proc_type.
 *
 * Returns: (transfer full): a new #LigmaProcedure.
 *
 * Since: 3.0
 **/
LigmaProcedure  *
ligma_load_procedure_new (LigmaPlugIn      *plug_in,
                         const gchar     *name,
                         LigmaPDBProcType  proc_type,
                         LigmaRunLoadFunc  run_func,
                         gpointer         run_data,
                         GDestroyNotify   run_data_destroy)
{
  LigmaLoadProcedure *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);
  g_return_val_if_fail (ligma_is_canonical_identifier (name), NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_INTERNAL, NULL);
  g_return_val_if_fail (proc_type != LIGMA_PDB_PROC_TYPE_EXTENSION, NULL);
  g_return_val_if_fail (run_func != NULL, NULL);

  procedure = g_object_new (LIGMA_TYPE_LOAD_PROCEDURE,
                            "plug-in",        plug_in,
                            "name",           name,
                            "procedure-type", proc_type,
                            NULL);

  procedure->priv->run_func         = run_func;
  procedure->priv->run_data         = run_data;
  procedure->priv->run_data_destroy = run_data_destroy;

  return LIGMA_PROCEDURE (procedure);
}

/**
 * ligma_load_procedure_set_handles_raw:
 * @procedure:   A load procedure object.
 * @handles_raw: The procedure's handles raw flag.
 *
 * Registers a load procedure as capable of handling raw digital camera loads.
 *
 * Since: 3.0
 **/
void
ligma_load_procedure_set_handles_raw (LigmaLoadProcedure *procedure,
                                     gint               handles_raw)
{
  g_return_if_fail (LIGMA_IS_LOAD_PROCEDURE (procedure));

  procedure->priv->handles_raw = handles_raw;
}

/**
 * ligma_load_procedure_get_handles_raw:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's 'handles raw' flag as set with
 * [method@LigmaLoadProcedure.set_handles_raw].
 *
 * Returns: The procedure's 'handles raw' flag.
 *
 * Since: 3.0
 **/
gint
ligma_load_procedure_get_handles_raw (LigmaLoadProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_LOAD_PROCEDURE (procedure), 0);

  return procedure->priv->handles_raw;
}

/**
 * ligma_load_procedure_set_thumbnail_loader:
 * @procedure:      A load procedure object.
 * @thumbnail_proc: The name of the thumbnail load procedure.
 *
 * Associates a thumbnail loader with a file load procedure.
 *
 * Some file formats allow for embedded thumbnails, other file formats
 * contain a scalable image or provide the image data in different
 * resolutions. A file plug-in for such a format may register a
 * special procedure that allows LIGMA to load a thumbnail preview of
 * the image. This procedure is then associated with the standard
 * load procedure using this function.
 *
 * Since: 3.0
 **/
void
ligma_load_procedure_set_thumbnail_loader (LigmaLoadProcedure *procedure,
                                          const gchar       *thumbnail_proc)
{
  g_return_if_fail (LIGMA_IS_LOAD_PROCEDURE (procedure));

  g_free (procedure->priv->thumbnail_proc);
  procedure->priv->thumbnail_proc = g_strdup (thumbnail_proc);
}

/**
 * ligma_load_procedure_get_thumbnail_loader:
 * @procedure: A load procedure object.
 *
 * Returns the procedure's thumbnail loader procedure as set with
 * [method@LigmaLoadProcedure.set_thumbnail_loader].
 *
 * Returns: The procedure's thumbnail loader procedure
 *
 * Since: 3.0
 **/
const gchar *
ligma_load_procedure_get_thumbnail_loader (LigmaLoadProcedure *procedure)
{
  g_return_val_if_fail (LIGMA_IS_LOAD_PROCEDURE (procedure), NULL);

  return procedure->priv->thumbnail_proc;
}
