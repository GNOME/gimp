/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-progress.c
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
#include "core/ligmadisplay.h"
#include "core/ligmaparamspecs.h"
#include "core/ligmapdbprogress.h"
#include "core/ligmaprogress.h"

#include "pdb/ligmapdb.h"
#include "pdb/ligmapdberror.h"

#include "ligmaplugin.h"
#include "ligmaplugin-progress.h"
#include "ligmapluginmanager.h"
#include "ligmatemporaryprocedure.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void   ligma_plug_in_progress_cancel_callback (LigmaProgress *progress,
                                                     LigmaPlugIn   *plug_in);


/*  public functions  */

gint
ligma_plug_in_progress_attach (LigmaProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (LIGMA_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count++;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

gint
ligma_plug_in_progress_detach (LigmaProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (LIGMA_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count--;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

void
ligma_plug_in_progress_start (LigmaPlugIn  *plug_in,
                             const gchar *message,
                             LigmaDisplay *display)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (display == NULL || LIGMA_IS_DISPLAY (display));

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress)
    {
      proc_frame->progress = ligma_new_progress (plug_in->manager->ligma,
                                                display);

      if (proc_frame->progress)
        {
          proc_frame->progress_created = TRUE;

          g_object_ref (proc_frame->progress);

          ligma_plug_in_progress_attach (proc_frame->progress);
        }
    }

  if (proc_frame->progress)
    {
      if (! proc_frame->progress_cancel_id)
        {
          g_object_add_weak_pointer (G_OBJECT (proc_frame->progress),
                                     (gpointer) &proc_frame->progress);

          proc_frame->progress_cancel_id =
            g_signal_connect (proc_frame->progress, "cancel",
                              G_CALLBACK (ligma_plug_in_progress_cancel_callback),
                              plug_in);
        }

      if (ligma_progress_is_active (proc_frame->progress))
        {
          if (message)
            ligma_progress_set_text_literal (proc_frame->progress, message);

          if (ligma_progress_get_value (proc_frame->progress) > 0.0)
            ligma_progress_set_value (proc_frame->progress, 0.0);
        }
      else
        {
          ligma_progress_start (proc_frame->progress, TRUE,
                               "%s", message ? message : "");
        }
    }
}

void
ligma_plug_in_progress_end (LigmaPlugIn          *plug_in,
                           LigmaPlugInProcFrame *proc_frame)
{
  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));
  g_return_if_fail (proc_frame != NULL);

  if (proc_frame->progress)
    {
      if (proc_frame->progress_cancel_id)
        {
          g_signal_handler_disconnect (proc_frame->progress,
                                       proc_frame->progress_cancel_id);
          proc_frame->progress_cancel_id = 0;

          g_object_remove_weak_pointer (G_OBJECT (proc_frame->progress),
                                        (gpointer) &proc_frame->progress);
        }

      if (ligma_plug_in_progress_detach (proc_frame->progress) < 1 &&
          ligma_progress_is_active (proc_frame->progress))
        {
          ligma_progress_end (proc_frame->progress);
        }

      if (proc_frame->progress_created)
        {
          ligma_free_progress (plug_in->manager->ligma, proc_frame->progress);
          g_clear_object (&proc_frame->progress);
        }
    }
}

void
ligma_plug_in_progress_set_text (LigmaPlugIn  *plug_in,
                                const gchar *message)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    ligma_progress_set_text_literal (proc_frame->progress, message);
}

void
ligma_plug_in_progress_set_value (LigmaPlugIn *plug_in,
                                 gdouble     percentage)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! ligma_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      ligma_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && ligma_progress_is_active (proc_frame->progress))
    ligma_progress_set_value (proc_frame->progress, percentage);
}

void
ligma_plug_in_progress_pulse (LigmaPlugIn *plug_in)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_if_fail (LIGMA_IS_PLUG_IN (plug_in));

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! ligma_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      ligma_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && ligma_progress_is_active (proc_frame->progress))
    ligma_progress_pulse (proc_frame->progress);
}

guint32
ligma_plug_in_progress_get_window_id (LigmaPlugIn *plug_in)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), 0);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    return ligma_progress_get_window_id (proc_frame->progress);

  return 0;
}

gboolean
ligma_plug_in_progress_install (LigmaPlugIn  *plug_in,
                               const gchar *progress_callback)
{
  LigmaPlugInProcFrame *proc_frame;
  LigmaProcedure       *procedure;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  procedure = ligma_pdb_lookup_procedure (plug_in->manager->ligma->pdb,
                                         progress_callback);

  if (! LIGMA_IS_TEMPORARY_PROCEDURE (procedure)                ||
      LIGMA_TEMPORARY_PROCEDURE (procedure)->plug_in != plug_in ||
      procedure->num_args                           != 3       ||
      ! G_IS_PARAM_SPEC_INT    (procedure->args[0])            ||
      ! G_IS_PARAM_SPEC_STRING (procedure->args[1])            ||
      ! G_IS_PARAM_SPEC_DOUBLE (procedure->args[2]))
    {
      return FALSE;
    }

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    {
      ligma_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);
    }

  proc_frame->progress = g_object_new (LIGMA_TYPE_PDB_PROGRESS,
                                       "pdb",           plug_in->manager->ligma->pdb,
                                       "context",       proc_frame->main_context,
                                       "callback-name", progress_callback,
                                       NULL);

  ligma_plug_in_progress_attach (proc_frame->progress);

  return TRUE;
}

gboolean
ligma_plug_in_progress_uninstall (LigmaPlugIn  *plug_in,
                                 const gchar *progress_callback)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (LIGMA_IS_PDB_PROGRESS (proc_frame->progress))
    {
      ligma_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);

      return TRUE;
    }

  return FALSE;
}

gboolean
ligma_plug_in_progress_cancel (LigmaPlugIn  *plug_in,
                              const gchar *progress_callback)
{
  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  return FALSE;
}


/*  private functions  */

static LigmaValueArray *
get_cancel_return_values (LigmaProcedure *procedure)
{
  LigmaValueArray *return_vals;
  GError         *error;

  error = g_error_new_literal (LIGMA_PDB_ERROR, LIGMA_PDB_ERROR_CANCELLED,
                               _("Cancelled"));
  return_vals = ligma_procedure_get_return_values (procedure, FALSE, error);
  g_error_free (error);

  return return_vals;
}

static void
ligma_plug_in_progress_cancel_callback (LigmaProgress *progress,
                                       LigmaPlugIn   *plug_in)
{
  LigmaPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;
  GList               *list;

  if (proc_frame->main_loop)
    {
      proc_frame->return_vals =
        get_cancel_return_values (proc_frame->procedure);
    }

  for (list = plug_in->temp_proc_frames; list; list = g_list_next (list))
    {
      proc_frame = list->data;

      if (proc_frame->main_loop)
        {
          proc_frame->return_vals =
            get_cancel_return_values (proc_frame->procedure);
        }
    }

  ligma_plug_in_close (plug_in, TRUE);
}
