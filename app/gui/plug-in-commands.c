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

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "plug-in/plug-in.h"
#include "plug-in/plug-in-proc.h"

#include "display/gimpdisplay.h"

#include "plug-in-commands.h"

#include "app_procs.h"


void
plug_in_run_cmd_callback (GtkWidget *widget,
                          gpointer   data,
                          guint      action)
{
  GimpDisplay *gdisplay;
  ProcRecord  *proc_rec;
  Argument    *args;
  gint         i;
  gint         gdisp_ID = -1;
  gint         argc     = 0; /* calm down a gcc warning.  */

  /* get the active gdisplay */
  gdisplay = gimp_context_get_display (gimp_get_user_context (the_gimp));

  proc_rec = (ProcRecord *) data;

  /* construct the procedures arguments */
  args = g_new0 (Argument, proc_rec->num_args);

  /* initialize the argument types */
  for (i = 0; i < proc_rec->num_args; i++)
    args[i].arg_type = proc_rec->args[i].arg_type;

  switch (proc_rec->proc_type)
    {
    case GIMP_EXTENSION:
      /* initialize the first argument  */
      args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
      argc = 1;
      break;

    case GIMP_PLUGIN:
      if (gdisplay)
	{
	  gdisp_ID = gimp_display_get_ID (gdisplay);

	  /* initialize the first 3 plug-in arguments  */
	  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
	  args[1].value.pdb_int = gimp_image_get_ID (gdisplay->gimage);
	  args[2].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (gimp_image_active_drawable (gdisplay->gimage)));
	  argc = 3;
	}
      else
	{
	  g_warning ("Uh-oh, no active gdisplay for the plug-in!");
	  g_free (args);
	  return;
	}
      break;

    case GIMP_TEMPORARY:
      args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
      argc = 1;
      if (proc_rec->num_args >= 3 &&
	  proc_rec->args[1].arg_type == GIMP_PDB_IMAGE &&
	  proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
	{
	  if (gdisplay)
	    {
	      gdisp_ID = gdisplay->ID;

	      args[1].value.pdb_int = gimp_image_get_ID (gdisplay->gimage);
	      args[2].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (gimp_image_active_drawable (gdisplay->gimage)));
	      argc = 3;
	    }
	  else
	    {
	      g_warning ("Uh-oh, no active gdisplay for the temporary procedure!");
	      g_free (args);
	      return;
	    }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      g_free (args);
      return;
    }

  /* run the plug-in procedure */
  plug_in_run (the_gimp, proc_rec, args, argc, FALSE, TRUE, gdisp_ID);

  if (proc_rec->proc_type == GIMP_PLUGIN)
    last_plug_in = proc_rec;

  g_free (args);
}

void
plug_in_repeat_cmd_callback (GtkWidget *widget,
                             gpointer   data,
                             guint      action)
{
  GimpDisplay  *gdisp;
  GimpDrawable *drawable;
  gboolean      interactive;

  gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data)));

  if (! gdisp)
    return;

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! drawable)
    return;

  interactive = action ? TRUE : FALSE;

  plug_in_repeat (GIMP (data),
                  gimp_display_get_ID (gdisp),
                  gimp_image_get_ID (gdisp->gimage),
                  gimp_item_get_ID (GIMP_ITEM (drawable)),
                  interactive);
}
