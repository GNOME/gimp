/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmatemporaryprocedure.c
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "plug-in-types.h"

#include "core/ligma.h"

#include "ligmaplugin.h"
#define __YES_I_NEED_LIGMA_PLUG_IN_MANAGER_CALL__
#include "ligmapluginmanager-call.h"
#include "ligmatemporaryprocedure.h"

#include "ligma-intl.h"


static void             ligma_temporary_procedure_finalize (GObject        *object);

static LigmaValueArray * ligma_temporary_procedure_execute  (LigmaProcedure  *procedure,
                                                           Ligma           *ligma,
                                                           LigmaContext    *context,
                                                           LigmaProgress   *progress,
                                                           LigmaValueArray *args,
                                                           GError        **error);
static void        ligma_temporary_procedure_execute_async (LigmaProcedure  *procedure,
                                                           Ligma           *ligma,
                                                           LigmaContext    *context,
                                                           LigmaProgress   *progress,
                                                           LigmaValueArray *args,
                                                           LigmaDisplay    *display);

static GFile     * ligma_temporary_procedure_get_file      (LigmaPlugInProcedure *procedure);


G_DEFINE_TYPE (LigmaTemporaryProcedure, ligma_temporary_procedure,
               LIGMA_TYPE_PLUG_IN_PROCEDURE)

#define parent_class ligma_temporary_procedure_parent_class


static void
ligma_temporary_procedure_class_init (LigmaTemporaryProcedureClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  LigmaProcedureClass       *proc_class   = LIGMA_PROCEDURE_CLASS (klass);
  LigmaPlugInProcedureClass *plug_class   = LIGMA_PLUG_IN_PROCEDURE_CLASS (klass);

  object_class->finalize    = ligma_temporary_procedure_finalize;

  proc_class->execute       = ligma_temporary_procedure_execute;
  proc_class->execute_async = ligma_temporary_procedure_execute_async;

  plug_class->get_file      = ligma_temporary_procedure_get_file;
}

static void
ligma_temporary_procedure_init (LigmaTemporaryProcedure *proc)
{
  LIGMA_PROCEDURE (proc)->proc_type = LIGMA_PDB_PROC_TYPE_TEMPORARY;
}

static void
ligma_temporary_procedure_finalize (GObject *object)
{
  /* LigmaTemporaryProcedure *proc = LIGMA_TEMPORARY_PROCEDURE (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static LigmaValueArray *
ligma_temporary_procedure_execute (LigmaProcedure   *procedure,
                                  Ligma            *ligma,
                                  LigmaContext     *context,
                                  LigmaProgress    *progress,
                                  LigmaValueArray  *args,
                                  GError         **error)
{
  return ligma_plug_in_manager_call_run_temp (ligma->plug_in_manager,
                                             context, progress,
                                             LIGMA_TEMPORARY_PROCEDURE (procedure),
                                             args);
}

static void
ligma_temporary_procedure_execute_async (LigmaProcedure  *procedure,
                                        Ligma           *ligma,
                                        LigmaContext    *context,
                                        LigmaProgress   *progress,
                                        LigmaValueArray *args,
                                        LigmaDisplay    *display)
{
  LigmaTemporaryProcedure *temp_procedure = LIGMA_TEMPORARY_PROCEDURE (procedure);
  LigmaValueArray         *return_vals;

  return_vals = ligma_plug_in_manager_call_run_temp (ligma->plug_in_manager,
                                                    context, progress,
                                                    temp_procedure,
                                                    args);

  if (return_vals)
    {
      LigmaPlugInProcedure *proc = LIGMA_PLUG_IN_PROCEDURE (procedure);

      ligma_plug_in_procedure_handle_return_values (proc,
                                                   ligma, progress,
                                                   return_vals);
      ligma_value_array_unref (return_vals);
    }
}

static GFile *
ligma_temporary_procedure_get_file (LigmaPlugInProcedure *procedure)
{
  return LIGMA_TEMPORARY_PROCEDURE (procedure)->plug_in->file;
}


/*  public functions  */

LigmaProcedure *
ligma_temporary_procedure_new (LigmaPlugIn *plug_in)
{
  LigmaTemporaryProcedure *proc;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), NULL);

  proc = g_object_new (LIGMA_TYPE_TEMPORARY_PROCEDURE, NULL);

  proc->plug_in = plug_in;

  LIGMA_PLUG_IN_PROCEDURE (proc)->file = g_file_new_for_path ("none");

  return LIGMA_PROCEDURE (proc);
}
