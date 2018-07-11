/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpplugin-context.c
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

#include "plug-in-types.h"

#include "core/gimp.h"

#include "pdb/gimppdbcontext.h"

#include "gimpplugin.h"
#include "gimpplugin-context.h"
#include "gimppluginmanager.h"


gboolean
gimp_plug_in_context_push (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;
  GimpContext         *parent;
  GimpContext         *context;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    parent = proc_frame->context_stack->data;
  else
    parent = proc_frame->main_context;

  context = gimp_pdb_context_new (plug_in->manager->gimp, parent, FALSE);

  proc_frame->context_stack = g_list_prepend (proc_frame->context_stack,
                                              context);

  return TRUE;
}

gboolean
gimp_plug_in_context_pop (GimpPlugIn *plug_in)
{
  GimpPlugInProcFrame *proc_frame;

  g_return_val_if_fail (GIMP_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = gimp_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    {
      GimpContext *context = proc_frame->context_stack->data;

      proc_frame->context_stack = g_list_remove (proc_frame->context_stack,
                                                 context);
      g_object_unref (context);

      return TRUE;
    }

  return FALSE;
}
