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

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"

#include "plug-in/plug-in.h"

#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimpitemfactory.h"

#include "file-dialog-utils.h"
#include "file-save-dialog.h"

#include "gimprc.h"
#include "undo.h"

#include "libgimp/gimpintl.h"


typedef struct _OverwriteData OverwriteData;

struct _OverwriteData
{
  gchar *full_filename;
  gchar *raw_filename;
};


static void    file_save_dialog_create      (void);

static void    file_overwrite               (const gchar   *filename,
					     const gchar   *raw_filename);
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
file_save_dialog_menu_init (Gimp *gimp)
{
  GimpItemFactoryEntry  entry;
  PlugInProcDef        *file_proc;
  GSList               *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->save_procs = g_slist_reverse (gimp->save_procs);

  for (list = gimp->save_procs; list; list = g_slist_next (list))
    {
      gchar *basename;
      gchar *filename;
      gchar *page;
      gchar *lowercase_page;

      file_proc = (PlugInProcDef *) list->data;

      basename = g_path_get_basename (file_proc->prog);

      filename = g_strconcat (basename, ".html", NULL);

      g_free (basename);

      page = g_build_filename ("filters", filename, NULL);

      g_free (filename);

      lowercase_page = g_ascii_strdown (page, -1);

      g_free (page);

      entry.entry.path            = file_proc->menu_path;
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_save_type_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.quark_string          = NULL;
      entry.help_page             = lowercase_page;
      entry.description           = NULL;

      gimp_menu_item_create (&entry, NULL, file_proc);
    }
}

void
file_save_dialog_menu_reset (void)
{
  save_file_proc = NULL;
}

void
file_save_dialog_show (GimpImage *gimage)
{
  const gchar *filename;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (! gimp_image_active_drawable (gimage))
    return;

  the_gimage = gimage;

  set_filename = TRUE;

  filename = gimp_object_get_name (GIMP_OBJECT (gimage));

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

  file_dialog_update_menus (gimage->gimp->save_procs,
                            gimp_drawable_type (gimp_image_active_drawable (gimage)));

  file_dialog_show (filesave);
}

void
file_save_a_copy_dialog_show (GimpImage *gimage)
{
  const gchar *filename;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  if (! gimp_image_active_drawable (gimage))
    return;

  the_gimage = gimage;

  set_filename = FALSE;

  filename = gimp_object_get_name (GIMP_OBJECT (gimage));

  if (! filesave)
    file_save_dialog_create ();

  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);
  if (GTK_WIDGET_VISIBLE (filesave))
    return;

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save a Copy of the Image"));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesave),
                                   filename ?
                                   filename :
                                   "." G_DIR_SEPARATOR_S);

  file_dialog_update_menus (gimage->gimp->save_procs,
                            gimp_drawable_type (gimp_image_active_drawable (gimage)));

  file_dialog_show (filesave);
}


/*  private functions  */

static void
file_save_dialog_create (void)
{
  GtkFileSelection *file_sel;

  filesave = gtk_file_selection_new (_("Save Image"));
  gtk_window_set_wmclass (GTK_WINDOW (filesave), "save_image", "Gimp");
  gtk_window_set_position (GTK_WINDOW (filesave), GTK_WIN_POS_MOUSE);

  file_sel = GTK_FILE_SELECTION (filesave);

  gtk_container_set_border_width (GTK_CONTAINER (filesave), 2);
  gtk_container_set_border_width (GTK_CONTAINER (file_sel->button_area), 2);

  g_signal_connect_swapped (G_OBJECT (file_sel->cancel_button), "clicked",
			    G_CALLBACK (file_dialog_hide),
			    filesave);
  g_signal_connect (G_OBJECT (filesave), "delete_event",
		    G_CALLBACK (file_dialog_hide),
		    NULL);

  g_signal_connect (G_OBJECT (file_sel->ok_button), "clicked",
		    G_CALLBACK (file_save_ok_callback),
		    filesave);

  gtk_quit_add_destroy (1, GTK_OBJECT (filesave));

  /*  Connect the "F1" help key  */
  gimp_help_connect (filesave,
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

    save_menu = gtk_item_factory_from_path ("<Save>")->widget;
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

  file_dialog_update_name (proc, GTK_FILE_SELECTION (filesave));

  save_file_proc = proc;
}

static void
file_save_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection *fs;
  const gchar      *filename;
  const gchar      *raw_filename;
  gchar            *dot;
  gint              x;
  struct stat       buf;
  gint              err;

  fs = GTK_FILE_SELECTION (data);

  filename     = gtk_file_selection_get_filename (fs);
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

	  proc_rec = procedural_db_lookup (the_gimage->gimp,
                                           "plug_in_the_slimy_egg");
	  if (!proc_rec)
	    break;

	  file_dialog_hide (filesave);

	  args = g_new (Argument, 3);
	  args[0].arg_type      = GIMP_PDB_INT32;
	  args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
	  args[1].arg_type      = GIMP_PDB_IMAGE;
	  args[1].value.pdb_int = gimp_image_get_ID (the_gimage);
	  args[2].arg_type      = GIMP_PDB_DRAWABLE;
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
            {
              gtk_file_selection_set_filename (fs, filename);
            }
	}
      else
	{
	  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);
	  file_overwrite (filename, raw_filename);
	}
    }
  else
    {
      GimpPDBStatusType status;

      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

      status = file_save (the_gimage,
                          filename,
                          raw_filename,
                          save_file_proc,
                          GIMP_RUN_INTERACTIVE,
                          set_filename);

      if (status != GIMP_PDB_SUCCESS &&
          status != GIMP_PDB_CANCEL)
	{
	  /* Please add error. (: %s) --bex */
          g_message (_("Saving %s failed."), filename);
	}
      else
        {
	  file_dialog_hide (GTK_WIDGET (fs));
        }

      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
    }
}

static void
file_overwrite (const gchar *filename,
		const gchar *raw_filename)
{
  OverwriteData *overwrite_data;
  GtkWidget     *query_box;
  gchar         *overwrite_text;

  overwrite_data = g_new0 (OverwriteData, 1);

  overwrite_data->full_filename = g_strdup (filename);
  overwrite_data->raw_filename  = g_strdup (raw_filename);

  overwrite_text = g_strdup_printf (_("%s exists. Overwrite?"), filename);

  query_box = gimp_query_boolean_box (_("File Exists!"),
				      gimp_standard_help_func,
				      "save/file_exists.html",
				      FALSE,
				      overwrite_text,
				      GTK_STOCK_YES, GTK_STOCK_NO,
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
      GimpPDBStatusType status;

      status = file_save (the_gimage,
                          overwrite_data->full_filename,
                          overwrite_data->raw_filename,
                          save_file_proc,
                          GIMP_RUN_INTERACTIVE,
                          set_filename);

      if (status != GIMP_PDB_SUCCESS &&
          status != GIMP_PDB_CANCEL)
        {
	  /* Another error required. --bex */
          g_message (_("Saving '%s' failed."), overwrite_data->full_filename);
        }
      else
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
