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

#include "core/core-types.h"

#include "core/gimpimage.h"
#include "core/gimpobject.h"

#include "file-commands.h"
#include "file-new-dialog.h"
#include "file-open-dialog.h"
#include "file-save-dialog.h"
#include "menus.h"

#include "app_procs.h"
#include "file-open.h"
#include "file-save.h"
#include "gdisplay.h"
#include "gdisplay_ops.h"
#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


#define REVERT_DATA_KEY "revert-confirm-dialog"

#define return_if_no_display(gdisp) \
        gdisp = gdisplay_active (); \
        if (!gdisp) return


/*  local function prototypes  */

static void   file_revert_confirm_callback (GtkWidget *widget,
					    gboolean   revert,
					    gpointer   data);


/*  public functions  */

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
file_open_by_extension_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  file_open_dialog_menu_reset ();
}

void
file_open_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  file_open_dialog_show ();
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

  status = file_open_with_display (filename);

  if (status != GIMP_PDB_SUCCESS &&
      status != GIMP_PDB_CANCEL)
    {
      g_message (_("Error opening file: %s\n"), filename);
    }
}

void
file_save_by_extension_cmd_callback (GtkWidget *widget,
				     gpointer   data)
{
  file_save_dialog_menu_reset ();
}

void
file_save_cmd_callback (GtkWidget *widget,
			gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  if (! gimp_image_active_drawable (gdisp->gimage))
    return;

  /*  Only save if the gimage has been modified  */
  if (! gimprc.trust_dirty_flag || gdisp->gimage->dirty != 0)
    {
      gchar *filename;

      filename = g_strdup (gimp_object_get_name (GIMP_OBJECT (gdisp->gimage)));

      if (! filename)
	{
	  file_save_as_cmd_callback (widget, data);
	}
      else
	{
	  const gchar *raw_filename;
	  gint         status;

	  raw_filename = g_basename (filename);
	  
	  status = file_save (gdisp->gimage,
			      filename,
			      raw_filename,
			      RUN_WITH_LAST_VALS,
			      TRUE);

	  if (status != GIMP_PDB_SUCCESS &&
	      status != GIMP_PDB_CANCEL)
	    {
	      g_message (_("Save failed.\n%s"), filename);
	    }
	}

      g_free (filename);
    }
}

void
file_save_as_cmd_callback (GtkWidget *widget,
			   gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  file_save_dialog_show (gdisp->gimage);
}

void
file_save_a_copy_as_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GDisplay *gdisp;
  return_if_no_display (gdisp);

  file_save_a_copy_dialog_show (gdisp->gimage);
}

void
file_revert_cmd_callback (GtkWidget *widget,
			  gpointer   data)
{
  GDisplay    *gdisp;
  GtkWidget   *query_box;
  const gchar *filename;

  return_if_no_display (gdisp);

  filename = gimp_object_get_name (GIMP_OBJECT (gdisp->gimage));

  query_box = g_object_get_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY);

  if (! filename)
    {
      g_message (_("Revert failed.\n"
		   "No filename associated with this image."));
    }
  else if (query_box)
    {
      gdk_window_raise (query_box->window);
    }
  else
    {
      gchar *text;

      text = g_strdup_printf (_("Reverting %s to\n"
				"%s\n\n"
				"(You will lose all your changes\n"
				"including all undo information)"),
			      g_basename (filename),
			      filename);

      query_box = gimp_query_boolean_box (_("Revert Image?"),
					  gimp_standard_help_func,
					  "file/revert.html",
					  FALSE,
					  text,
					  GTK_STOCK_YES, GTK_STOCK_NO,
					  G_OBJECT (gdisp->gimage),
					  "disconnect",
					  file_revert_confirm_callback,
					  gdisp->gimage);

      g_free (text);

      g_object_set_data (G_OBJECT (gdisp->gimage), REVERT_DATA_KEY,
			 query_box);

      gtk_widget_show (query_box);
    }
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


/*  private functions  */

static void
file_revert_confirm_callback (GtkWidget *widget,
			      gboolean   revert,
			      gpointer   data)
{
  GimpImage *old_gimage;

  old_gimage = (GimpImage *) data;

  g_object_set_data (G_OBJECT (old_gimage), REVERT_DATA_KEY, NULL);

  if (revert)
    {
      GimpImage   *new_gimage;
      const gchar *filename;
      gint         status;

      filename = gimp_object_get_name (GIMP_OBJECT (old_gimage));

      new_gimage = file_open_image (old_gimage->gimp,
				    filename, filename,
				    _("Revert"),
				    NULL,
				    RUN_INTERACTIVE,
				    &status);

      if (new_gimage != NULL)
	{
	  undo_free (new_gimage);
	  gdisplays_reconnect (old_gimage, new_gimage);
	  gdisplays_resize_cursor_label (new_gimage);
	  gdisplays_update_full (new_gimage);
	  gdisplays_shrink_wrap (new_gimage);
	  gimp_image_clean_all (new_gimage);
	}
      else if (status != GIMP_PDB_CANCEL)
	{
	  g_message (_("Revert failed.\n%s"), filename);
	}
    }
}
