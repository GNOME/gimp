/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "apptypes.h"

#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "cursorutil.h"
#include "dialog_handler.h"
#include "docindex.h"
#include "file-dialog-utils.h"
#include "file-save-dialog.h"
#include "gdisplay.h"
#include "gimpui.h"
#include "menus.h"

#include "gimprc.h"
#include "file-save.h"
#include "file-utils.h"
#include "plug_in.h"
#include "temp_buf.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _OverwriteData OverwriteData;

struct _OverwriteData
{
  gchar *full_filename;
  gchar *raw_filename;
};


static void    file_save_dialog_create      (void);

static void    file_overwrite               (gchar         *filename,
					     gchar         *raw_filename);
static void    file_overwrite_callback      (GtkWidget     *widget,
					     gboolean       overwrite,
					     gpointer       data);

static void    file_save_ok_callback        (GtkWidget     *widget,
					     gpointer       data);

static void    file_save_type_callback      (GtkWidget     *widget,
					     gpointer       data);


static GtkWidget  *filesave     = NULL;
static GtkWidget  *save_options = NULL;

static PlugInProcDef *save_file_proc = NULL;

static GimpImage *the_gimage   = NULL;
static gboolean   set_filename = TRUE;


/*  public functions  */

void
file_save_menu_init (void)
{
  GimpItemFactoryEntry  entry;
  PlugInProcDef        *file_proc;
  GSList               *list;

  save_procs = g_slist_reverse (save_procs);

  for (list = save_procs; list; list = g_slist_next (list))
    {
      gchar *help_page;

      file_proc = (PlugInProcDef *) list->data;

      help_page = g_strconcat ("filters/",
			       g_basename (file_proc->prog),
			       ".html",
			       NULL);
      g_strdown (help_page);

      entry.entry.path            = file_proc->menu_path;
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_save_type_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.help_page             = help_page;
      entry.description           = NULL;

      menus_create_item_from_full_path (&entry, NULL, file_proc);
    }
}

void
file_save_callback (GtkWidget *widget,
		    gpointer   data)
{
  GDisplay *gdisplay;

  gdisplay = gdisplay_active ();
  if (! gdisplay)
    return;

  if (! gimp_image_active_drawable (gdisplay->gimage))
    return;

  /*  Only save if the gimage has been modified  */
  if (!trust_dirty_flag || gdisplay->gimage->dirty != 0)
    {
      gchar *filename;

      filename =
	g_strdup (gimp_object_get_name (GIMP_OBJECT (gdisplay->gimage)));

      if (! filename)
	{
	  file_save_as_callback (widget, data);
	}
      else
	{
	  gchar *raw_filename;
	  gint   status;

	  raw_filename = g_basename (filename);
	  
	  status = file_save (gdisplay->gimage,
			      filename,
			      raw_filename,
			      RUN_WITH_LAST_VALS,
			      TRUE);

	  if (status != PDB_SUCCESS &&
	      status != PDB_CANCEL)
	    {
	      g_message (_("Save failed.\n%s"), filename);
	    }

	}

      g_free (filename);
    }
}

void
file_save_as_callback (GtkWidget *widget,
		       gpointer   data)
{
  GDisplay    *gdisplay;
  const gchar *filename;

  gdisplay = gdisplay_active ();
  if (! gdisplay)
    return;

  if (! gimp_image_active_drawable (gdisplay->gimage))
    return;

  the_gimage = gdisplay->gimage;

  set_filename = TRUE;

  filename = gimp_object_get_name (GIMP_OBJECT (the_gimage));

  if (! filesave)
    file_save_dialog_create ();

  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
  if (GTK_WIDGET_VISIBLE (filesave))
    return;

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save Image"));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesave),
                                   filename ?
				   filename :
                                   "." G_DIR_SEPARATOR_S);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisplay->gimage)))
    {
    case RGB_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_RGB_IMAGE);
      break;
    case RGBA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_RGBA_IMAGE);
      break;
    case GRAY_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_GRAY_IMAGE);
      break;
    case GRAYA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_GRAYA_IMAGE);
      break;
    case INDEXED_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_INDEXED_IMAGE);
      break;
    case INDEXEDA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_INDEXEDA_IMAGE);
      break;
    }

  file_dialog_show (filesave);
}

void
file_save_a_copy_as_callback (GtkWidget *widget,
			      gpointer   data)
{
  GDisplay    *gdisplay;
  const gchar *filename;

  gdisplay = gdisplay_active ();
  if (! gdisplay)
    return;

  if (! gimp_image_active_drawable (gdisplay->gimage))
    return;

  the_gimage = gdisplay->gimage;

  set_filename = FALSE;

  filename = gimp_object_get_name (GIMP_OBJECT (the_gimage));

  if (!filesave)
    file_save_dialog_create ();

  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
  if (GTK_WIDGET_VISIBLE (filesave))
    return;

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save a Copy of the Image"));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesave),
                                   filename ?
                                   filename :
                                   "." G_DIR_SEPARATOR_S);

  switch (gimp_drawable_type (gimp_image_active_drawable (gdisplay->gimage)))
    {
    case RGB_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_RGB_IMAGE);
      break;
    case RGBA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_RGBA_IMAGE);
      break;
    case GRAY_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_GRAY_IMAGE);
      break;
    case GRAYA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_GRAYA_IMAGE);
      break;
    case INDEXED_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_INDEXED_IMAGE);
      break;
    case INDEXEDA_GIMAGE:
      file_dialog_update_menus (save_procs, PLUG_IN_INDEXEDA_IMAGE);
      break;
    }

  file_dialog_show (filesave);
}

void
file_save_by_extension_callback (GtkWidget *widget,
				 gpointer   data)
{
  save_file_proc = NULL;
}


/*  private functions  */

static void
file_save_dialog_create (void)
{
  filesave = gtk_file_selection_new (_("Save Image"));
  gtk_window_set_wmclass (GTK_WINDOW (filesave), "save_image", "Gimp");
  gtk_window_set_position (GTK_WINDOW (filesave), GTK_WIN_POS_MOUSE);

  gtk_container_set_border_width (GTK_CONTAINER (filesave), 2);
  gtk_container_set_border_width 
    (GTK_CONTAINER (GTK_FILE_SELECTION (filesave)->button_area), 2);

  gtk_signal_connect_object
    (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->cancel_button), "clicked",
     GTK_SIGNAL_FUNC (file_dialog_hide),
     GTK_OBJECT (filesave));
  gtk_signal_connect (GTK_OBJECT (filesave), "delete_event",
		      GTK_SIGNAL_FUNC (file_dialog_hide),
		      NULL);
  gtk_signal_connect
    (GTK_OBJECT (GTK_FILE_SELECTION (filesave)->ok_button), "clicked",
     GTK_SIGNAL_FUNC (file_save_ok_callback),
     filesave);
  gtk_quit_add_destroy (1, GTK_OBJECT (filesave));

  /*  Connect the "F1" help key  */
  gimp_help_connect_help_accel (filesave,
				gimp_standard_help_func,
				"save/dialogs/file_save.html");

  {
    GtkWidget *frame;
    GtkWidget *hbox;
    GtkWidget *label;
    GtkWidget *option_menu;
    GtkWidget *save_menu;

    save_options = gtk_hbox_new (TRUE, 1);

    frame = gtk_frame_new (_("Save Options"));
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
    gtk_box_pack_start (GTK_BOX (save_options), frame, TRUE, TRUE, 4);

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), hbox);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Determine File Type:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, TRUE, 0);
    gtk_widget_show (option_menu);

    save_menu = menus_get_save_factory ()->widget;
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), save_menu);

    gtk_widget_show (frame);

    /* pack the containing save_options hbox into the save-dialog */
    gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (filesave)->main_vbox),
		      save_options, FALSE, FALSE, 0);
  }

  gtk_widget_show (save_options);
}

static void
file_save_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  PlugInProcDef *proc = (PlugInProcDef *) data;

  file_dialog_update_name (proc, filesave);

  save_file_proc = proc;
}

static void
file_save_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection *fs;
  gchar            *filename;
  gchar            *raw_filename;
  gchar            *dot;
  gint              x;
  struct stat       buf;
  gint              err;

  fs = GTK_FILE_SELECTION (data);
  filename = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));

  g_assert (filename && raw_filename);

  for (dot = strrchr (filename, '.'), x = 0; dot && *(++dot);)
    {
      if (*dot != 'e' || ++x < 0)
	break;
      else if (x > 3 && !strcmp (dot + 1, "k"))
	{ 
	  ProcRecord   *proc_rec;
	  Argument     *args;
	  GimpDrawable *the_drawable;

	  the_drawable = gimp_image_active_drawable (the_gimage);
	  if (!the_drawable)
	    return;

	  proc_rec = procedural_db_lookup ("plug_in_the_slimy_egg");
	  if (!proc_rec)
	    break;

	  file_dialog_hide (filesave);

	  args = g_new (Argument, 3);
	  args[0].arg_type      = PDB_INT32;
	  args[0].value.pdb_int = RUN_INTERACTIVE;
	  args[1].arg_type      = PDB_IMAGE;
	  args[1].value.pdb_int = gimp_image_get_ID (the_gimage);
	  args[2].arg_type      = PDB_DRAWABLE;
	  args[2].value.pdb_int = gimp_drawable_get_ID (the_drawable);

	  plug_in_run (proc_rec, args, 3, FALSE, TRUE, 0);

	  g_free (args);
	  
	  return;
	}
   }

  err = stat (filename, &buf);

  if (err == 0)
    {
      if (buf.st_mode & S_IFDIR)
	{
	  if (filename[strlen (filename) - 1] != G_DIR_SEPARATOR)
	    {
	      gchar *s = g_strconcat (filename, G_DIR_SEPARATOR_S, NULL);
	      gtk_file_selection_set_filename (fs, s);
	      g_free (s);
	    }
	  else
	    gtk_file_selection_set_filename (fs, filename);
	}
      else
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
	  file_overwrite (g_strdup (filename), g_strdup (raw_filename));
	}
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

      if (file_save_with_proc (the_gimage,
			       filename, raw_filename,
			       save_file_proc,
			       set_filename))
	{
	  file_dialog_hide (GTK_WIDGET (fs));
	}

      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
    }
}

static void
file_overwrite (gchar *filename,
		gchar *raw_filename)
{
  OverwriteData *overwrite_data;
  GtkWidget *query_box;
  gchar     *overwrite_text;

  overwrite_data = g_new (OverwriteData, 1);
  overwrite_data->full_filename = filename;
  overwrite_data->raw_filename  = raw_filename;

  overwrite_text = g_strdup_printf (_("%s exists, overwrite?"), filename);

  query_box = gimp_query_boolean_box (_("File Exists!"),
				      gimp_standard_help_func,
				      "save/file_exists.html",
				      FALSE,
				      overwrite_text,
				      _("Yes"), _("No"),
				      NULL, NULL,
				      file_overwrite_callback,
				      overwrite_data);

  g_free (overwrite_text);

  gtk_widget_show (query_box);
}

static void
file_overwrite_callback (GtkWidget *widget,
			 gboolean   overwrite,
			 gpointer   data)
{
  OverwriteData *overwrite_data;

  overwrite_data = (OverwriteData *) data;

  if (overwrite)
    {
      if (file_save_with_proc (the_gimage,
			       overwrite_data->full_filename,
			       overwrite_data->raw_filename,
			       save_file_proc,
			       set_filename))
	{
	  file_dialog_hide (GTK_WIDGET (filesave));
	}
    }

  /* always make file save dialog sensitive */
  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);

  g_free (overwrite_data->full_filename);
  g_free (overwrite_data->raw_filename);
  g_free (overwrite_data);
}
