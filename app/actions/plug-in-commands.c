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

#include "actions-types.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpitem.h"

#include "plug-in/plug-in-run.h"
#include "plug-in/plug-in-proc.h"

#include "widgets/gimpdock.h"
#include "widgets/gimpitemfactory.h"

#include "display/gimpdisplay.h"

#include "gui/plug-in-menus.h"

#include "plug-in-commands.h"


#define return_if_no_display(gdisp,data) \
  if (GIMP_IS_DISPLAY (data)) \
    gdisp = data; \
  else if (GIMP_IS_GIMP (data)) \
    gdisp = gimp_context_get_display (gimp_get_user_context (GIMP (data))); \
  else if (GIMP_IS_DOCK (data)) \
    gdisp = gimp_context_get_display (((GimpDock *) data)->context); \
  else \
    gdisp = NULL; \
  if (! gdisp) \
    return


void
plug_in_run_cmd_callback (GtkWidget *widget,
                          gpointer   data,
                          guint      action)
{
  GtkItemFactory *item_factory;
  Gimp           *gimp;
  ProcRecord     *proc_rec;
  Argument       *args;
  gint            gdisp_ID = -1;
  gint            i;
  gint            argc;
  GimpImageType   drawable_type = GIMP_RGB_IMAGE;

  item_factory = gtk_item_factory_from_widget (widget);

  gimp = GIMP_ITEM_FACTORY (item_factory)->gimp;

  proc_rec = (ProcRecord *) data;

  /* construct the procedures arguments */
  args = g_new0 (Argument, proc_rec->num_args);

  /* initialize the argument types */
  for (i = 0; i < proc_rec->num_args; i++)
    args[i].arg_type = proc_rec->args[i].arg_type;

  /* initialize the first argument  */
  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
  argc = 1;

  switch (proc_rec->proc_type)
    {
    case GIMP_EXTENSION:
      break;

    case GIMP_PLUGIN:
    case GIMP_TEMPORARY:
      if (proc_rec->num_args >= 2 &&
          proc_rec->args[1].arg_type == GIMP_PDB_IMAGE)
        {
          GimpDisplay *gdisplay;

          gdisplay = gimp_context_get_display (gimp_get_user_context (gimp));

          if (gdisplay)
            {
              gdisp_ID = gimp_display_get_ID (gdisplay);

              args[1].value.pdb_int = gimp_image_get_ID (gdisplay->gimage);
              argc++;

              if (proc_rec->num_args >= 2 &&
                  proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
                {
                  GimpDrawable *drawable;

                  drawable = gimp_image_active_drawable (gdisplay->gimage);

                  if (drawable)
                    {
                      drawable_type = gimp_drawable_type (drawable);

                      args[2].value.pdb_int =
                        gimp_item_get_ID (GIMP_ITEM (drawable));
                      argc++;
                    }
                  else
                    {
                      g_warning ("Uh-oh, no active drawable for the plug-in!");
                      g_free (args);
                      return;
                    }
                }
            }
	}
      break;

    default:
      g_error ("Unknown procedure type.");
      g_free (args);
      return;
    }

  /* run the plug-in procedure */
  plug_in_run (gimp, gimp_get_user_context (gimp),
               proc_rec, args, argc, FALSE, TRUE, gdisp_ID);

  /* remember only "standard" plug-ins */
  if (proc_rec->proc_type == GIMP_PLUGIN           &&
      proc_rec->num_args >= 2                      &&
      proc_rec->args[1].arg_type == GIMP_PDB_IMAGE &&
      proc_rec->args[2].arg_type == GIMP_PDB_DRAWABLE)
    {
      gimp->last_plug_in = proc_rec;
      plug_in_menus_update (GIMP_ITEM_FACTORY (item_factory), drawable_type);
    }

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
  return_if_no_display (gdisp, data);

  drawable = gimp_image_active_drawable (gdisp->gimage);

  if (! drawable)
    return;

  interactive = action ? TRUE : FALSE;

  plug_in_repeat (gdisp->gimage->gimp,
                  gimp_get_user_context (gdisp->gimage->gimp),
                  gimp_display_get_ID (gdisp),
                  gimp_image_get_ID (gdisp->gimage),
                  gimp_item_get_ID (GIMP_ITEM (drawable)),
                  interactive);
}
