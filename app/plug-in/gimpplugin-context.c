/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmaplugin-context.c
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

#include "core/ligma.h"

#include "pdb/ligmapdbcontext.h"

#include "ligmaplugin.h"
#include "ligmaplugin-context.h"
#include "ligmapluginmanager.h"


gboolean
ligma_plug_in_context_push (LigmaPlugIn *plug_in)
{
  LigmaPlugInProcFrame *proc_frame;
  LigmaContext         *parent;
  LigmaContext         *context;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    parent = proc_frame->context_stack->data;
  else
    parent = proc_frame->main_context;

  context = ligma_pdb_context_new (plug_in->manager->ligma, parent, FALSE);

  proc_frame->context_stack = g_list_prepend (proc_frame->context_stack,
                                              context);

  return TRUE;
}

gboolean
ligma_plug_in_context_pop (LigmaPlugIn *plug_in)
{
  LigmaPlugInProcFrame *proc_frame;

  g_return_val_if_fail (LIGMA_IS_PLUG_IN (plug_in), FALSE);

  proc_frame = ligma_plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    {
      LigmaContext *context = proc_frame->context_stack->data;

      proc_frame->context_stack = g_list_remove (proc_frame->context_stack,
                                                 context);
      g_object_unref (context);

      return TRUE;
    }

  return FALSE;
}
