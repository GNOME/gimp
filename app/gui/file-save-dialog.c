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

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpitemfactory.h"
#include "widgets/gimpmenufactory.h"

#include "dialogs.h"
#include "file-dialog-utils.h"
#include "file-save-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * file_save_dialog_create      (Gimp            *gimp,
                                                 GimpMenuFactory *menu_factory);
static void        file_save_response_callback  (GtkWidget       *save_dialog,
                                                 gint             response_id,
                                                 Gimp            *gimp);
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
                                                 gboolean         set_uri_and_proc,
                                                 gboolean         set_image_clean);


static GtkWidget     *filesave         = NULL;

static PlugInProcDef *save_file_proc   = NULL;
static GimpImage     *the_gimage       = NULL;
static gboolean       set_uri_and_proc = TRUE;
static gboolean       set_image_clean  = TRUE;


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
                       GimpMenuFactory *menu_factory,
                       GtkWidget       *parent)
{
  GimpItemFactory *item_factory;
  gchar           *filename;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));

  if (! gimp_image_active_drawable (gimage))
    return;

  the_gimage       = gimage;
  set_uri_and_proc = TRUE;
  set_image_clean  = TRUE;

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

  file_dialog_show (filesave, parent);
}

void
file_save_a_copy_dialog_show (GimpImage       *gimage,
                              GimpMenuFactory *menu_factory,
                              GtkWidget       *parent)
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
  set_image_clean  = FALSE;

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

  file_dialog_show (filesave, parent);
}


/*  private functions  */

static GtkWidget *
file_save_dialog_create (Gimp            *gimp,
                         GimpMenuFactory *menu_factory)
{
  GtkWidget *save_dialog;

  save_dialog = file_dialog_new (gimp,
                                 global_dialog_factory,
                                 "gimp-file-save-dialog",
                                 menu_factory, "<Save>",
                                 _("Save Image"), "gimp-file-save",
                                 GIMP_HELP_FILE_SAVE);

  g_signal_connect (save_dialog, "response",
                    G_CALLBACK (file_save_response_callback),
                    gimp);

  return save_dialog;
}

static void
file_save_response_callback (GtkWidget *save_dialog,
                             gint       response_id,
                             Gimp      *gimp)
{
  GtkFileSelection *fs;
  const gchar      *filename;
  const gchar      *raw_filename;
  gchar            *uri;

  if (response_id != GTK_RESPONSE_OK)
    {
      file_dialog_hide (save_dialog);
      return;
    }

  fs = GTK_FILE_SELECTION (save_dialog);

  filename     = gtk_file_selection_get_filename (fs);
  raw_filename = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));

  g_assert (filename && raw_filename);

  uri = g_filename_to_uri (filename, NULL, NULL);

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
                                   set_uri_and_proc,
                                   set_image_clean);

      gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
    }

  g_free (uri);
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
                                      save_dialog,
				      gimp_standard_help_func,
				      GIMP_HELP_FILE_SAVE_OVERWRITE,
				      GIMP_STOCK_QUESTION,
				      message,
				      GTK_STOCK_YES, GTK_STOCK_NO,
				      NULL, NULL,
				      file_save_overwrite_callback,
				      overwrite_data);

  g_free (message);

  gtk_window_set_transient_for (GTK_WINDOW (query_box),
				GTK_WINDOW (save_dialog));

  gtk_widget_set_sensitive (save_dialog, FALSE);

  gtk_widget_show (query_box);
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
                                   set_uri_and_proc,
                                   set_image_clean);
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
                             gboolean       set_uri_and_proc,
                             gboolean       set_image_clean)
{
  GimpPDBStatusType  status;
  GError            *error = NULL;

  status = file_save_as (gimage,
                         uri,
                         raw_filename,
                         save_proc,
                         GIMP_RUN_INTERACTIVE,
                         set_uri_and_proc,
                         set_image_clean,
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
