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

#include "core/core-types.h"

#include "file-commands.h"
#include "file-new-dialog.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"

#include "app_procs.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "menus.h"

#include "libgimp/gimpintl.h"


#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


void
file_new_cmd_callback (GtkWidget *widget,
		       gpointer   data,
		       guint      action)
{
  GDisplay  *gdisp;
  GimpImage *gimage = NULL;

  /*  Before we try to determine the responsible gdisplay,
   *  make sure this wasn't called from the toolbox
   */
  if (action)
    {
      gdisp = gdisplay_active ();

      if (gdisp)
        gimage = gdisp->gimage;
    }

  file_new_dialog_create (gimage);
}

void
file_open_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  file_open_callback (widget, data);
}

void
file_last_opened_cmd_callback (GtkWidget *widget,
			       gpointer   data,
			       guint      action)
{
  gchar *filename;
  guint  num_entries;
  gint   status;

  num_entries = g_slist_length (last_opened_raw_filenames); 

  if (action >= num_entries)
    return;

  filename =
    ((GString *) g_slist_nth_data (last_opened_raw_filenames, action))->str;

  status = file_open_with_display (filename, filename);

  if (status != GIMP_PDB_SUCCESS &&
      status != GIMP_PDB_CANCEL)
    {
      g_message (_("Error opening file: %s\n"), filename);
    }
}

void
file_save_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  file_save_callback (widget, data);
}

void
file_save_as_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  file_save_as_callback (widget, data);
}

void
file_save_a_copy_as_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  file_save_a_copy_as_callback (widget, data);
}

void
file_revert_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  file_revert_callback (widget, data);
}

void
file_close_cmd_callback (GtkWidget *widget,
			 gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  gdisplay_close_window (gdisp, FALSE);
}

void
file_quit_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  app_exit (FALSE);
}
