/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppluginprocframe.c
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

#include <string.h>

#include <glib-object.h>

#include "plug-in-types.h"

#include "core/gimpcontext.h"
#include "core/gimpprogress.h"

#include "gimpplugin.h"
#include "gimpplugin-cleanup.h"
#include "gimpplugin-progress.h"
#include "gimppluginprocedure.h"


/*  public functions  */

GimpPlugInProcFrame *
gimp_plug_in_proc_frame_new (GimpContext         *context,
                             GimpProgress        *progress,
                             GimpPlugInProcedure *procedure)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), NULL);
  g_return_val_if_fail (GIMP_IS_PLUG_IN_PROCEDURE (procedure), NULL);

  proc_frame = g_slice_new0 (GimpPlugInProcFrame);

  proc_frame->ref_count = 1;

  gimp_plug_in_proc_frame_init (proc_frame, context, progress, procedure);

  return proc_frame;
}

void
gimp_plug_in_proc_frame_init (GimpPlugInProcFrame *proc_frame,
                              GimpContext         *context,
                              GimpProgress        *progress,
                              GimpPlugInProcedure *procedure)
{
  g_return_if_fail (proc_frame != NULL);
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (procedure == NULL ||
                    GIMP_IS_PLUG_IN_PROCEDURE (procedure));

  proc_frame->main_context       = g_object_ref (context);
  proc_frame->context_stack      = NULL;
  proc_frame->procedure          = GIMP_PROCEDURE (procedure);
  proc_frame->main_loop          = NULL;
  proc_frame->return_vals        = NULL;
  proc_frame->progress           = progress ? g_object_ref (progress) : NULL;
  proc_frame->progress_created   = FALSE;
  proc_frame->progress_cancel_id = 0;

  if (progress)
    gimp_plug_in_progress_attach (progress);
}

void
gimp_plug_in_proc_frame_dispose (GimpPlugInProcFrame *proc_frame,
                                 GimpPlugIn          *plug_in)
{
  g_return_if_fail (proc_frame != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  if (proc_frame->progress)
    {
      gimp_plug_in_progress_end (plug_in, proc_frame);

      if (proc_frame->progress)
        {
          g_object_unref (proc_frame->progress);
          proc_frame->progress = NULL;
        }
    }

  if (proc_frame->context_stack)
    {
      g_list_foreach (proc_frame->context_stack, (GFunc) g_object_unref, NULL);
      g_list_free (proc_frame->context_stack);
      proc_frame->context_stack = NULL;
    }

  if (proc_frame->main_context)
    {
      g_object_unref (proc_frame->main_context);
      proc_frame->main_context = NULL;
    }

  if (proc_frame->return_vals)
    {
      g_value_array_free (proc_frame->return_vals);
      proc_frame->return_vals = NULL;
    }

  if (proc_frame->main_loop)
    {
      g_main_loop_unref (proc_frame->main_loop);
      proc_frame->main_loop = NULL;
    }

  if (proc_frame->cleanups)
    gimp_plug_in_cleanup (plug_in, proc_frame);
}

GimpPlugInProcFrame *
gimp_plug_in_proc_frame_ref (GimpPlugInProcFrame *proc_frame)
{
  g_return_val_if_fail (proc_frame != NULL, NULL);

  proc_frame->ref_count++;

  return proc_frame;
}

void
gimp_plug_in_proc_frame_unref (GimpPlugInProcFrame *proc_frame,
                               GimpPlugIn          *plug_in)
{
  g_return_if_fail (proc_frame != NULL);
  g_return_if_fail (GIMP_IS_PLUG_IN (plug_in));

  proc_frame->ref_count--;

  if (proc_frame->ref_count < 1)
    {
      gimp_plug_in_proc_frame_dispose (proc_frame, plug_in);
      g_slice_free (GimpPlugInProcFrame, proc_frame);
    }
}

GValueArray *
gimp_plug_in_proc_frame_get_return_vals (GimpPlugInProcFrame *proc_frame)
{
  GValueArray *return_vals;

  g_return_val_if_fail (proc_frame != NULL, NULL);

  if (proc_frame->return_vals)
    {
      if (proc_frame->return_vals->n_values >=
          proc_frame->procedure->num_values + 1)
        {
          return_vals = proc_frame->return_vals;
        }
      else
        {
          /* Allocate new return values of the correct size. */
          return_vals = gimp_procedure_get_return_values (proc_frame->procedure,
                                                          FALSE);

          /* Copy all of the arguments we can. */
          memcpy (return_vals->values, proc_frame->return_vals->values,
                  sizeof (GValue) * proc_frame->return_vals->n_values);

          /* Free the old arguments. */
          g_free (proc_frame->return_vals->values);
          proc_frame->return_vals->values = 0;
          proc_frame->return_vals->n_values = 0;
          g_value_array_free (proc_frame->return_vals);
        }

      /* We have consumed any saved values, so clear them. */
      proc_frame->return_vals = NULL;
    }
  else
    {
      /* Just return a dummy set of values. */
      return_vals = gimp_procedure_get_return_values (proc_frame->procedure,
                                                      FALSE);
    }

  return return_vals;
}
