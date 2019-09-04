/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptemporaryprocedure.c
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

#include "libgimpbase/gimpbase.h"

#include "plug-in-types.h"

#include "core/gimp.h"

#include "gimpplugin.h"
#define __YES_I_NEED_GIMP_PLUG_IN_MANAGER_CALL__
#include "gimppluginmanager-call.h"
#include "gimptemporaryprocedure.h"

#include "gimp-intl.h"


static void             gimp_temporary_procedure_finalize (GObject        *object);

static GimpValueArray * gimp_temporary_procedure_execute  (GimpProcedure  *procedure,
                                                           Gimp           *gimp,
                                                           GimpContext    *context,
                                                           GimpProgress   *progress,
                                                           GimpValueArray *args,
                                                           GError        **error);
static void        gimp_temporary_procedure_execute_async (GimpProcedure  *procedure,
                                                           Gimp           *gimp,
                                                           GimpContext    *context,
                                                           GimpProgress   *progress,
                                                           GimpValueArray *args,
                                                           GimpDisplay    *display);

static GFile     * gimp_temporary_procedure_get_file      (GimpPlugInProcedure *procedure);


G_DEFINE_TYPE (GimpTemporaryProcedure, gimp_temporary_procedure,
               GIMP_TYPE_PLUG_IN_PROCEDURE)

#define parent_class gimp_temporary_procedure_parent_class


static void
gimp_temporary_procedure_class_init (GimpTemporaryProcedureClass *klass)
{
  GObjectClass             *object_class = G_OBJECT_CLASS (klass);
  GimpProcedureClass       *proc_class   = GIMP_PROCEDURE_CLASS (klass);
  GimpPlugInProcedureClass *plug_class   = GIMP_PLUG_IN_PROCEDURE_CLASS (klass);

  object_class->finalize    = gimp_temporary_procedure_finalize;

  proc_class->execute       = gimp_temporary_procedure_execute;
  proc_class->execute_async = gimp_temporary_procedure_execute_async;

  plug_class->get_file      = gimp_temporary_procedure_get_file;
}

static void
gimp_temporary_procedure_init (GimpTemporaryProcedure *proc)
{
  GIMP_PROCEDURE (proc)->proc_type = GIMP_PDB_PROC_TYPE_TEMPORARY;
}

static void
gimp_temporary_procedure_finalize (GObject *object)
{
  /* GimpTemporaryProcedure *proc = GIMP_TEMPORARY_PROCEDURE (object); */

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GimpValueArray *
gimp_temporary_procedure_execute (GimpProcedure   *procedure,
                                  Gimp            *gimp,
                                  GimpContext     *context,
                                  GimpProgress    *progress,
                                  GimpValueArray  *args,
                                  GError         **error)
{
  return gimp_plug_in_manager_call_run_temp (gimp->plug_in_manager,
                                             context, progress,
                                             GIMP_TEMPORARY_PROCEDURE (procedure),
                                             args);
}

static void
gimp_temporary_procedure_execute_async (GimpProcedure  *procedure,
                                        Gimp           *gimp,
                                        GimpContext    *context,
                                        GimpProgress   *progress,
                                        GimpValueArray *args,
                                        GimpDisplay    *display)
{
  GimpTemporaryProcedure *temp_procedure = GIMP_TEMPORARY_PROCEDURE (procedure);
  GimpValueArray         *return_vals;

  return_vals = gimp_plug_in_manager_call_run_temp (gimp->plug_in_manager,
                                                    context, progress,
                                                    temp_procedure,
                                                    args);

  if (return_vals)
    {
      GimpPlugInProcedure *proc = GIMP_PLUG_IN_PROCEDURE (procedure);

      gimp_plug_in_procedure_handle_return_values (proc,
                                                   gimp, progress,
                                                   return_vals);
      gimp_value_array_unref (return_vals);
    }
}

static GFile *
gimp_temporary_procedure_get_file (GimpPlugInProcedure *procedure)
{
  return GIMP_TEMPORARY_PROCEDURE (procedure)->plug_in->file;
}


/*  public functions  */

GimpProcedure *
gimp_temporary_procedure_new (GimpPlugIn *plug_in)
{
  GimpTemporaryProcedure *proc;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), NULL);

  proc = g_object_new (GIMP_TYPE_TEMPORARY_PROCEDURE, NULL);

  proc->plug_in = plug_in;

  GIMP_PLUG_IN_PROCEDURE (proc)->file = g_file_new_for_path ("none");

  return GIMP_PROCEDURE (proc);
}
