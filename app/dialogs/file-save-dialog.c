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

#include <string.h>

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"

#include "dialogs.h"
#include "file-dialog-utils.h"
#include "file-save-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * file_save_dialog_create      (Gimp          *gimp);
static void        file_save_dialog_response    (GtkWidget     *save_dialog,
                                                 gint           response_id,
                                                 Gimp          *gimp);
static void        file_save_overwrite          (GtkWidget     *save_dialog,
                                                 const gchar   *uri,
                                                 const gchar   *raw_filename);
static void        file_save_overwrite_callback (GtkWidget     *widget,
                                                 gboolean       overwrite,
                                                 gpointer       data);
static gboolean    file_save_dialog_save_image  (GtkWidget     *save_dialog,
                                                 GimpImage     *gimage,
                                                 const gchar   *uri,
                                                 const gchar   *raw_filename,
                                                 PlugInProcDef *save_proc,
                                                 gboolean       set_uri_and_proc,
                                                 gboolean       set_image_clean);


/*  private variables  */

static GtkWidget *filesave = NULL;


/*  public functions  */

void
file_save_dialog_show (GimpImage *gimage,
                       GtkWidget *parent)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  if (! gimp_image_active_drawable (gimage))
    return;

  if (! filesave)
    filesave = file_save_dialog_create (gimage->gimp);

  gimp_file_dialog_set_sensitive (GIMP_FILE_DIALOG (filesave), TRUE);

  if (GTK_WIDGET_VISIBLE (filesave))
    {
      gtk_window_present (GTK_WINDOW (filesave));
      return;
    }

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save Image"));

  gimp_file_dialog_set_image (GIMP_FILE_DIALOG (filesave),
                              gimage, TRUE, TRUE);

  file_dialog_show (filesave, parent);
}

void
file_save_a_copy_dialog_show (GimpImage *gimage,
                              GtkWidget *parent)
{
  g_return_if_fail (GIMP_IS_IMAGE (gimage));
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  if (! gimp_image_active_drawable (gimage))
    return;

  if (! filesave)
    filesave = file_save_dialog_create (gimage->gimp);

  gimp_file_dialog_set_sensitive (GIMP_FILE_DIALOG (filesave), TRUE);

  if (GTK_WIDGET_VISIBLE (filesave))
    {
      gtk_window_present (GTK_WINDOW (filesave));
      return;
    }

  gtk_window_set_title (GTK_WINDOW (filesave), _("Save a Copy of the Image"));

  gimp_file_dialog_set_image (GIMP_FILE_DIALOG (filesave),
                              gimage, FALSE, FALSE);

  file_dialog_show (filesave, parent);
}


/*  private functions  */

static GtkWidget *
file_save_dialog_create (Gimp *gimp)
{
  GtkWidget *dialog;

  dialog = gimp_file_dialog_new (gimp,
                                 GTK_FILE_CHOOSER_ACTION_SAVE,
                                 _("Save Image"), "gimp-file-save",
                                 GTK_STOCK_SAVE,
                                 GIMP_HELP_FILE_SAVE);

  gimp_dialog_factory_add_foreign (global_dialog_factory,
                                   "gimp-file-save-dialog", dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_save_dialog_response),
                    gimp);

  return dialog;
}

static void
file_save_dialog_response (GtkWidget *save_dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (save_dialog);
  gchar          *uri;
  gchar          *filename;

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! dialog->busy)
        file_dialog_hide (save_dialog);

      return;
    }

  uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (save_dialog));

  filename = g_filename_from_uri (uri, NULL, NULL);

  if (g_file_test (filename, G_FILE_TEST_EXISTS))
    {
      file_save_overwrite (save_dialog, uri, uri);
    }
  else
    {
      gimp_file_dialog_set_sensitive (dialog, FALSE);

      if (file_save_dialog_save_image (save_dialog,
                                       dialog->gimage,
                                       uri,
                                       uri,
                                       dialog->file_proc,
                                       dialog->set_uri_and_proc,
                                       dialog->set_image_clean))
        {
          file_dialog_hide (save_dialog);
        }

      gimp_file_dialog_set_sensitive (dialog, TRUE);
    }

  g_free (uri);
  g_free (filename);
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
  message = g_strdup_printf (_("A file named '%s' already exists.\n\n"
                               "Do you want to replace it with the image "
                               "you are saving?"), filename);
  g_free (filename);

  query_box = gimp_query_boolean_box (_("File exists!"),
                                      save_dialog,
                                      gimp_standard_help_func,
                                      GIMP_HELP_FILE_SAVE_OVERWRITE,
                                      GIMP_STOCK_QUESTION,
                                      message,
                                      _("Replace"), GTK_STOCK_CANCEL,
                                      NULL, NULL,
                                      file_save_overwrite_callback,
                                      overwrite_data);

  g_free (message);

  gtk_window_set_transient_for (GTK_WINDOW (query_box),
                                GTK_WINDOW (save_dialog));

  gimp_file_dialog_set_sensitive (GIMP_FILE_DIALOG (save_dialog), FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, FALSE);

  gtk_widget_show (query_box);
}

static void
file_save_overwrite_callback (GtkWidget *widget,
                              gboolean   overwrite,
                              gpointer   data)
{
  OverwriteData  *overwrite_data = data;
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (overwrite_data->save_dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
                                     GTK_RESPONSE_CANCEL, TRUE);

  if (overwrite)
    {
      gtk_widget_hide (widget);

      if (file_save_dialog_save_image (overwrite_data->save_dialog,
                                       dialog->gimage,
                                       overwrite_data->uri,
                                       overwrite_data->raw_filename,
                                       dialog->file_proc,
                                       dialog->set_uri_and_proc,
                                       dialog->set_image_clean))
        {
          file_dialog_hide (overwrite_data->save_dialog);
        }

    }

  gimp_file_dialog_set_sensitive (dialog, TRUE);

  g_free (overwrite_data->uri);
  g_free (overwrite_data->raw_filename);
  g_free (overwrite_data);
}

static gboolean
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
                         gimp_get_user_context (gimage->gimp),
                         GIMP_PROGRESS (save_dialog),
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
      gchar *filename = file_utils_uri_to_utf8_filename (uri);

      g_message (_("Saving '%s' failed:\n\n%s"),
                 filename, error->message);
      g_clear_error (&error);

      g_free (filename);

      return FALSE;
    }

  return TRUE;
}
