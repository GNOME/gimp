/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-progress.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "plug-in-types.h"

#include "core/gimp.h"
#include "core/gimpparamspecs.h"
#include "core/gimppdbprogress.h"
#include "core/gimpprogress.h"

#include "pdb/gimp-pdb.h"
#include "pdb/gimptemporaryprocedure.h"

#include "plug-in.h"
#include "plug-in-progress.h"


/*  local function prototypes  */

static void   plug_in_progress_cancel_callback (GimpProgress *progress,
                                                PlugIn       *plug_in);


/*  public functions  */

void
plug_in_progress_start (PlugIn      *plug_in,
                        const gchar *message,
                        GimpObject  *display)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (display == NULL || GIMP_IS_OBJECT (display));

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress)
    {
      proc_frame->progress = gimp_new_progress (plug_in->gimp, display);

      if (proc_frame->progress)
        {
          proc_frame->progress_created = TRUE;

          g_object_ref (proc_frame->progress);
        }
    }

  if (proc_frame->progress)
    {
      if (! proc_frame->progress_cancel_id)
        proc_frame->progress_cancel_id =
          g_signal_connect (proc_frame->progress, "cancel",
                            G_CALLBACK (plug_in_progress_cancel_callback),
                            plug_in);

      if (gimp_progress_is_active (proc_frame->progress))
        {
          if (message)
            gimp_progress_set_text (proc_frame->progress, message);

          if (gimp_progress_get_value (proc_frame->progress) > 0.0)
            gimp_progress_set_value (proc_frame->progress, 0.0);
        }
      else
        {
          gimp_progress_start (proc_frame->progress,
                               message ? message : "",
                               TRUE);
        }
    }
}

void
plug_in_progress_end (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    {
      if (proc_frame->progress_cancel_id)
        {
          g_signal_handler_disconnect (proc_frame->progress,
                                       proc_frame->progress_cancel_id);
          proc_frame->progress_cancel_id = 0;
        }

      if (gimp_progress_is_active (proc_frame->progress))
        gimp_progress_end (proc_frame->progress);

      if (proc_frame->progress_created)
        {
          gimp_free_progress (plug_in->gimp, proc_frame->progress);
          g_object_unref (proc_frame->progress);
          proc_frame->progress = NULL;
        }
    }
}

void
plug_in_progress_set_text (PlugIn      *plug_in,
                           const gchar *message)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    gimp_progress_set_text (proc_frame->progress, message);
}

void
plug_in_progress_set_value (PlugIn  *plug_in,
                            gdouble  percentage)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! gimp_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && gimp_progress_is_active (proc_frame->progress))
    gimp_progress_set_value (proc_frame->progress, percentage);
}

void
plug_in_progress_pulse (PlugIn  *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_if_fail (plug_in != NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (! proc_frame->progress                           ||
      ! gimp_progress_is_active (proc_frame->progress) ||
      ! proc_frame->progress_cancel_id)
    {
      plug_in_progress_start (plug_in, NULL, NULL);
    }

  if (proc_frame->progress && gimp_progress_is_active (proc_frame->progress))
    gimp_progress_pulse (proc_frame->progress);
}

guint32
plug_in_progress_get_window (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_val_if_fail (plug_in != NULL, 0);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    return gimp_progress_get_window (proc_frame->progress);

  return 0;
}

gboolean
plug_in_progress_install (PlugIn      *plug_in,
                          const gchar *progress_callback)
{
  PlugInProcFrame *proc_frame;
  GimpProcedure   *procedure;

  g_return_val_if_fail (plug_in != NULL, FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  procedure = gimp_pdb_lookup (plug_in->gimp, progress_callback);

  if (! GIMP_IS_TEMPORARY_PROCEDURE (procedure)                ||
      GIMP_TEMPORARY_PROCEDURE (procedure)->plug_in != plug_in ||
      procedure->num_args                           != 3       ||
      ! GIMP_IS_PARAM_SPEC_INT32 (procedure->args[0])          ||
      ! G_IS_PARAM_SPEC_STRING   (procedure->args[1])          ||
      ! G_IS_PARAM_SPEC_DOUBLE   (procedure->args[2]))
    {
      return FALSE;
    }

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame->progress)
    {
      plug_in_progress_end (plug_in);

      if (proc_frame->progress)
        {
          g_object_unref (proc_frame->progress);
          proc_frame->progress = NULL;
        }
    }

  proc_frame->progress = g_object_new (GIMP_TYPE_PDB_PROGRESS,
                                       "context",       proc_frame->main_context,
                                       "callback-name", progress_callback,
                                       NULL);

  return TRUE;
}

gboolean
plug_in_progress_uninstall (PlugIn      *plug_in,
                            const gchar *progress_callback)
{
  PlugInProcFrame *proc_frame;

  g_return_val_if_fail (plug_in != NULL, FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (GIMP_IS_PDB_PROGRESS (proc_frame->progress))
    {
      plug_in_progress_end (plug_in);
      g_object_unref (proc_frame->progress);
      proc_frame->progress = NULL;

      return TRUE;
    }

  return FALSE;
}

gboolean
plug_in_progress_cancel (PlugIn      *plug_in,
                         const gchar *progress_callback)
{
  g_return_val_if_fail (plug_in != NULL, FALSE);
  g_return_val_if_fail (progress_callback != NULL, FALSE);

  return FALSE;
}

void
plug_in_progress_message (PlugIn      *plug_in,
                          const gchar *message)
{
  PlugInProcFrame *proc_frame;
  gchar           *domain;

  g_return_if_fail (plug_in != NULL);
  g_return_if_fail (message != NULL);

  proc_frame = plug_in_get_proc_frame (plug_in);

  domain = plug_in_get_undo_desc (plug_in);

  if (proc_frame->progress)
    {
      gimp_progress_message (proc_frame->progress,
                             plug_in->gimp, domain, message);
    }
  else
    {
      gimp_message (plug_in->gimp, domain, message);
    }

  g_free (domain);
}


/*  private functions  */

static void
plug_in_progress_cancel_callback (GimpProgress *progress,
                                  PlugIn       *plug_in)
{
  PlugInProcFrame *proc_frame = &plug_in->main_proc_frame;
  GList           *list;

  if (proc_frame->main_loop)
    {
      proc_frame->return_vals = gimp_procedure_get_return_values (NULL,
                                                                  FALSE);

      g_value_set_enum (proc_frame->return_vals->values, GIMP_PDB_CANCEL);
    }

  for (list = plug_in->temp_proc_frames; list; list = g_list_next (list))
    {
      proc_frame = list->data;

      if (proc_frame->main_loop)
        {
          proc_frame->return_vals = gimp_procedure_get_return_values (NULL,
                                                                      FALSE);

          g_value_set_enum (proc_frame->return_vals->values, GIMP_PDB_CANCEL);
        }
    }

  plug_in_close (plug_in, TRUE);
  plug_in_unref (plug_in);
}
