/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-progress.c
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
#include "core/gimpdisplay.h"
#include "core/gimpparamspecs.h"
#include "core/gimppdbprogress.h"
#include "core/gimpprogress.h"

#include "pdb/gimppdb.h"
#include "pdb/gimppdberror.h"

#include "gimpplugin.h"
#include "gimpplugin-progress.h"
#include "gimppluginmanager.h"
#include "gimptemporaryprocedure.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void   gimp_plug_in_progress_cancel_callback (GimpProgress *progress,
                                                     GimpPlugIn   *plug_in);


/*  public functions  */

gint
gimp_plug_in_progress_attach (GimpProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count++;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

gint
gimp_plug_in_progress_detach (GimpProgress *progress)
{
  gint attach_count;

  g_return_val_if_fail (GIMP_IS_PROGRESS (progress), 0);

  attach_count =
    GPOINTER_TO_INT (g_object_get_data (G_OBJECT (progress),
                                        "plug-in-progress-attach-count"));

  attach_count--;

  g_object_set_data (G_OBJECT (progress), "plug-in-progress-attach-count",
                     GINT_TO_POINTER (attach_count));

  return attach_count;
}

void
gimp_plug_in_progress_start (GimpPlugIn  *plug_in,
                             const gchar *message,
                             GimpDisplay *display)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
  g_return_if_fail (display == NULL || GIMP_IS_DISPLAY (display));

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress)
    {
      proc_frame->progress = gimp_new_progress (plug_in->manager->gimp,
                                                display);

      if (proc_frame->progress)
        {
          proc_frame->progress_created = TRUE;

          g_object_ref (proc_frame->progress);

          gimp_plug_in_progress_attach (proc_frame->progress);
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
                              G_CALLBACK (gimp_plug_in_progress_cancel_callback),
                              plug_in);
        }

      if (gimp_progress_is_active (proc_frame->progress))
        {
          if (message)
            gimp_progress_set_text_literal (proc_frame->progress, message);

          if (gimp_progress_get_value (proc_frame->progress) > 0.0)
            gimp_progress_set_value (proc_frame->progress, 0.0);
        }
      else
        {
          gimp_progress_start (proc_frame->progress, TRUE,
                               "%s", message ? message : "");
        }
    }
}

void
gimp_plug_in_progress_end (GimpPlugIn          *plug_in,
                           GimpPlugInProcFrame *proc_frame)
{
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));
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

      if (gimp_plug_in_progress_detach (proc_frame->progress) < 1 &&
          gimp_progress_is_active (proc_frame->progress))
        {
          gimp_progress_end (proc_frame->progress);
        }

      if (proc_frame->progress_created)
        {
          gimp_free_progress (plug_in->manager->gimp, proc_frame->progress);
          g_clear_object (&proc_frame->progress);
        }
    }
}

void
gimp_plug_in_progress_set_text (GimpPlugIn  *plug_in,
                                const gchar *message)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    gimp_progress_set_text_literal (proc_frame->progress, message);
}

void
gimp_plug_in_progress_set_value (GimpPlugIn *plug_in,
                                 gdouble     percentage)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! gimp_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      gimp_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && gimp_progress_is_active (proc_frame->progress))
    gimp_progress_set_value (proc_frame->progress, percentage);
}

void
gimp_plug_in_progress_pulse (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! gimp_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      gimp_plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && gimp_progress_is_active (proc_frame->progress))
    gimp_progress_pulse (proc_frame->progress);
}

GBytes *
gimp_plug_in_progress_get_window_id (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), 0);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    return gimp_progress_get_window_id (proc_frame->progress);
  else if (plug_in->display)
    return gimp_get_display_window_id (plug_in->manager->gimp, plug_in->display);

  return 0;
}

gboolean
gimp_plug_in_progress_install (GimpPlugIn  *plug_in,
                               const gchar *progress_callback)
{
  GimpPlugInProcFrame *proc_frame;
  GimpProcedure       *procedure;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  procedure = gimp_pdb_lookup_procedure (plug_in->manager->gimp->pdb,
                                         progress_callback);

  if (! GIMP_IS_TEMPORARY_PROCEDURE (procedure)                ||
      GIMP_TEMPORARY_PROCEDURE (procedure)->plug_in != plug_in ||
      procedure->num_args                           != 3       ||
      ! G_IS_PARAM_SPEC_INT    (procedure->args[0])            ||
      ! G_IS_PARAM_SPEC_STRING (procedure->args[1])            ||
      ! G_IS_PARAM_SPEC_DOUBLE (procedure->args[2]))
    {
      return FALSE;
    }

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    {
      gimp_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);
    }

  proc_frame->progress = g_object_new (GIMP_TYPE_PDB_PROGRESS,
                                       "pdb",           plug_in->manager->gimp->pdb,
                                       "context",       proc_frame->main_context,
                                       "callback-name", progress_callback,
                                       NULL);

  gimp_plug_in_progress_attach (proc_frame->progress);

  return TRUE;
}

gboolean
gimp_plug_in_progress_uninstall (GimpPlugIn  *plug_in,
                                 const gchar *progress_callback)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (GIMP_IS_PDB_PROGRESS (proc_frame->progress))
    {
      gimp_plug_in_progress_end (plug_in, proc_frame);

      g_clear_object (&proc_frame->progress);

      return TRUE;
    }

  return FALSE;
}

gboolean
gimp_plug_in_progress_cancel (GimpPlugIn  *plug_in,
                              const gchar *progress_callback)
{
  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  return FALSE;
}


/*  private functions  */

static GimpValueArray *
get_cancel_return_values (GimpProcedure *procedure)
{
  GimpValueArray *return_vals;
  GError         *error;

  error = g_error_new_literal (GIMP_PDB_ERROR, GIMP_PDB_ERROR_CANCELLED,
                               _("Cancelled"));
  return_vals = gimp_procedure_get_return_values (procedure, FALSE, error);
  g_error_free (error);

  return return_vals;
}

static void
gimp_plug_in_progress_cancel_callback (GimpProgress *progress,
                                       GimpPlugIn   *plug_in)
{
  GimpPlugInProcFrame *proc_frame = &plug_in->main_proc_frame;
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

  gimp_plug_in_close (plug_in, TRUE);
}
