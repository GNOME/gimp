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

#include "plug-in/plug-in-proc.h"
#include "plug-in/plug-in-run.h"

#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"

#include "file-dialog-utils.h"
#include "file-save-dialog.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static GtkWidget * file_save_dialog_create      (Gimp            *gimp,
                                                 GimpMenuFactory *menu_factory);
static void        file_save_ok_callback        (GtkWidget       *widget,
                                                 GtkWidget       *save_dialog);
static void        file_save_overwrite          (GtkWidget       *save_dialog,
                                                 const gchar     *uri,
                                                 const gchar     *raw_filename);
static void        file_save_overwrite_callback (GtkWidget       *widget,
                                                 gboolean         overwrite,
                                                 gpointer         data);
static void        file_save_dialog_save_image  (GtkWidget       *save_dialog,
                                                 GimpImage       *gimage,
                                                 const gchar     *uri,
                                                 const gchar     *raw_filename,
                                                 PlugInProcDef   *save_proc,
                                                 gboolean         set_uri_and_proc);


static GtkWidget     *filesave         = NULL;

static PlugInProcDef *save_file_proc   = NULL;
static GimpImage     *the_gimage       = NULL;
static gboolean       set_uri_and_proc = TRUE;


/*  public functions  */

void
file_save_dialog_set_type (PlugInProcDef *proc)
{
  if (proc)
    file_dialog_update_name (proc, GTK_FILE_SELECTION (filesave));

  save_file_proc = proc;
}

void
file_save_dialog_show (GimpImage       *gimage,
                       GimpMenuFactory *menu_factory)
{
  GimpItemFactory *item_factory;
  gchar           *filename;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));

  if (! gimp_image_active_drawable (gimage))
    return;

  the_gimage       = gimage;
  set_uri_and_proc = TRUE;

  if (! filesave)
    filesave = file_save_dialog_create (gimage->gimp, menu_factory);

  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);

  if (GTK_WIDGET_VISIBLE (filesave))
    {
      gtk_window_present (GTK_WINDOW (filesave));
      return;
    }

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save Image"));

  filename = gimp_image_get_filename (gimage);

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesave),
                                   filename ?
				   filename :
                                   "." G_DIR_SEPARATOR_S);

  g_free (filename);

  item_factory = g_object_get_data (G_OBJECT (filesave), "gimp-item-factory");

  gimp_item_factory_update (item_factory,
                            gimp_image_active_drawable (gimage));

  file_dialog_show (filesave);
}

void
file_save_a_copy_dialog_show (GimpImage       *gimage,
                              GimpMenuFactory *menu_factory)
{
  GimpItemFactory *item_factory;
  const gchar     *uri;
  gchar           *filename = NULL;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));

  if (! gimp_image_active_drawable (gimage))
    return;

  the_gimage       = gimage;
  set_uri_and_proc = FALSE;

  uri = gimp_object_get_name (GIMP_OBJECT (gimage));

  if (uri)
    filename = g_filename_from_uri (uri, NULL, NULL);

  if (! filesave)
    filesave = file_save_dialog_create (gimage->gimp, menu_factory);

  gtk_widget_set_sensitive (GTK_WIDGET (filesave), TRUE);

  if (GTK_WIDGET_VISIBLE (filesave))
    {
      gtk_window_present (GTK_WINDOW (filesave));
      return;
    }

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save a Copy of the Image"));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesave),
                                   filename ?
                                   filename :
                                   "." G_DIR_SEPARATOR_S);

  g_free (filename);

  item_factory = g_object_get_data (G_OBJECT (filesave), "gimp-item-factory");

  gimp_item_factory_update (item_factory,
                            gimp_image_active_drawable (gimage));

  file_dialog_show (filesave);
}


/*  private functions  */

static GtkWidget *
file_save_dialog_create (Gimp            *gimp,
                         GimpMenuFactory *menu_factory)
{
  return file_dialog_new (gimp,
                          menu_factory, "<Save>",
                          _("Save Image"), "save_image",
                          "save/dialogs/file_save.html",
                          G_CALLBACK (file_save_ok_callback));
}

static void
file_save_ok_callback (GtkWidget *widget,
		       GtkWidget *save_dialog)
{
  GtkFileSelection *fs;
  const gchar      *filename;
  const gchar      *raw_filename;
  gchar            *uri;

  fs = GTK_FILE_SELECTION (save_dialog);

  filename     = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));

  g_assert (filename && raw_filename);

  uri = g_filename_to_uri (filename, NULL, NULL);

  {
    gchar *dot;
    gint   x;

    for (dot = strrchr (filename, '.'), x = 0; dot && *(++dot);)
      {
        if (*dot != 'e' || ++x < 0)
          {
            break;
          }
        else if (x > 3 && !strcmp (dot + 1, "k"))
          { 
            ProcRecord   *proc_rec;
            Argument     *args;
            GimpDrawable *drawable;

            file_dialog_hide (save_dialog);

            drawable = gimp_image_active_drawable (the_gimage);
            if (! drawable)
              return;

            proc_rec = procedural_db_lookup (the_gimage->gimp,
                                             "plug_in_the_slimy_egg");
            if (! proc_rec)
              return;

            args = g_new (Argument, 3);
            args[0].arg_type      = GIMP_PDB_INT32;
            args[0].value.pdb_int = GIMP_RUN_INTERACTIVE;
            args[1].arg_type      = GIMP_PDB_IMAGE;
            args[1].value.pdb_int = gimp_image_get_ID (the_gimage);
            args[2].arg_type      = GIMP_PDB_DRAWABLE;
            args[2].value.pdb_int = gimp_item_get_ID (GIMP_ITEM (drawable));

            plug_in_run (the_gimage->gimp, proc_rec, args, 3, FALSE, TRUE, -1);

            g_free (args);

            return;
          }
      }
  }

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      if (g_file_test (filename, G_FILE_TEST_IS_DIR))
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
	  file_save_overwrite (save_dialog, uri, raw_filename);
	}
    }
  else
    {
      gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

      file_save_dialog_save_image (save_dialog,
                                   the_gimage,
                                   uri,
                                   raw_filename,
                                   save_file_proc,
                                   set_uri_and_proc);

      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
    }
}

typedef struct _OverwriteData OverwriteData;

struct _OverwriteData
{
  GtkWidget *save_dialog;
  gchar     *uri;
  gchar     *raw_filename;
};

static void
file_save_overwrite (GtkWidget   *save_dialog,
                     const gchar *uri,
                     const gchar *raw_filename)
{
  OverwriteData *overwrite_data;
  GtkWidget     *query_box;
  gchar         *filename;
  gchar         *message;

  overwrite_data = g_new0 (OverwriteData, 1);

  overwrite_data->save_dialog  = save_dialog;
  overwrite_data->uri          = g_strdup (uri);
  overwrite_data->raw_filename = g_strdup (raw_filename);

  filename = file_utils_uri_to_utf8_filename (uri);

  message = g_strdup_printf (_("File '%s' exists.\n"
                               "Overwrite it?"), filename);

  g_free (filename);

  query_box = gimp_query_boolean_box (_("File Exists!"),
				      gimp_standard_help_func,
				      "save/file_exists.html",
				      GTK_STOCK_DIALOG_QUESTION,
				      message,
				      GTK_STOCK_YES, GTK_STOCK_NO,
				      NULL, NULL,
				      file_save_overwrite_callback,
				      overwrite_data);

  g_free (message);

  gtk_widget_show (query_box);

  gtk_widget_set_sensitive (save_dialog, FALSE);
}

static void
file_save_overwrite_callback (GtkWidget *widget,
                              gboolean   overwrite,
                              gpointer   data)
{
  OverwriteData *overwrite_data;

  overwrite_data = (OverwriteData *) data;

  if (overwrite)
    {
      file_save_dialog_save_image (overwrite_data->save_dialog,
                                   the_gimage,
                                   overwrite_data->uri,
                                   overwrite_data->raw_filename,
                                   save_file_proc,
                                   set_uri_and_proc);
    }

  gtk_widget_set_sensitive (overwrite_data->save_dialog, TRUE);

  g_free (overwrite_data->uri);
  g_free (overwrite_data->raw_filename);
  g_free (overwrite_data);
}

static void
file_save_dialog_save_image (GtkWidget     *save_dialog,
                             GimpImage     *gimage,
                             const gchar   *uri,
                             const gchar   *raw_filename,
                             PlugInProcDef *save_proc,
                             gboolean       set_uri_and_proc)
{
  GimpPDBStatusType  status;
  GError            *error = NULL;

  status = file_save_as (gimage,
                         uri,
                         raw_filename,
                         save_proc,
                         GIMP_RUN_INTERACTIVE,
                         set_uri_and_proc,
                         &error);

  if (status != GIMP_PDB_SUCCESS &&
      status != GIMP_PDB_CANCEL)
    {
      gchar *filename;

      filename = file_utils_uri_to_utf8_filename (uri);

      g_message (_("Saving '%s' failed:\n\n%s"),
                 filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }
  else
    {
      file_dialog_hide (save_dialog);
    }
}
