/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
#include "core/gimpcontext.h"

#include "plug-in.h"
#include "plug-in-context.h"


gboolean
plug_in_context_push (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;
  GimpContext     *parent;
  GimpContext     *context;

  g_return_val_if_fail (plug_in != NULL, FALSE);

  proc_frame = plug_in_get_proc_frame (plug_in);

  if (proc_frame->context_stack)
    parent = proc_frame->context_stack->data;
  else
    parent = proc_frame->main_context;

  context = gimp_context_new (plug_in->gimp, "plug-in context", NULL);
  gimp_context_copy_properties (parent, context, GIMP_CONTEXT_ALL_PROPS_MASK);

  proc_frame->context_stack = g_list_prepend (proc_frame->context_stack,
                                              context);

  return TRUE;
}

gboolean
plug_in_context_pop (PlugIn *plug_in)
{
  PlugInProcFrame *proc_frame;

  g_return_val_if_fail (plug_in != NULL, FALSE);

  proc_frame = plug_in_get_proc_frame (plug_in);

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
