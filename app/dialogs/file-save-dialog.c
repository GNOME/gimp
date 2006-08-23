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

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"

#include "file/file-save.h"
#include "file/file-utils.h"

#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmessagebox.h"
#include "widgets/gimpmessagedialog.h"

#include "file-save-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       file_save_dialog_response    (GtkWidget     *save_dialog,
                                                gint           response_id,
                                                Gimp          *gimp);
static void       file_save_overwrite          (GtkWidget     *save_dialog,
                                                const gchar   *uri,
                                                const gchar   *raw_filename);
static void       file_save_overwrite_response (GtkWidget     *dialog,
                                                gint           response_id,
                                                gpointer       data);
static gboolean   file_save_dialog_save_image  (GtkWidget     *save_dialog,
                                                GimpImage     *gimage,
                                                const gchar   *uri,
                                                const gchar   *raw_filename,
                                                PlugInProcDef *save_proc,
                                                gboolean       save_a_copy);


/*  public functions  */

GtkWidget *
file_save_dialog_new (Gimp *gimp)
{
  GtkWidget   *dialog;
  const gchar *uri;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_file_dialog_new (gimp,
                                 GTK_FILE_CHOOSER_ACTION_SAVE,
                                 _("Save Image"), "gimp-file-save",
                                 GTK_STOCK_SAVE,
                                 GIMP_HELP_FILE_SAVE);

  uri = g_object_get_data (G_OBJECT (gimp), "gimp-file-save-last-uri");

  if (uri)
    {
      gchar *folder_uri = g_path_get_dirname (uri);

#ifdef __GNUC__
#warning: FIXME: should use set_uri() but idle stuff in the file chooser seems to override set_current_name() when called immediately after set_uri()
#endif

      if (folder_uri)
        {
          gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (dialog),
                                                   folder_uri);
          g_free (folder_uri);
        }

      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "");
    }

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_save_dialog_response),
                    gimp);

  return dialog;
}


/*  private functions  */

static void
file_save_dialog_response (GtkWidget *save_dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GimpFileDialog *dialog = GIMP_FILE_DIALOG (save_dialog);
  gchar          *uri;

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! dialog->busy)
        gtk_widget_hide (save_dialog);

      return;
    }

  uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (save_dialog));

  if (uri && strlen (uri))
    {
      gchar *filename = file_utils_filename_from_uri (uri);

      g_return_if_fail (filename != NULL);

      if (g_file_test (filename, G_FILE_TEST_EXISTS))
        {
          file_save_overwrite (save_dialog, uri, uri);
        }
      else
        {
          gulong handler_id;

          gimp_file_dialog_set_sensitive (dialog, FALSE);
          handler_id = g_signal_connect (dialog, "destroy",
                                         G_CALLBACK (gtk_widget_destroyed),
                                         &dialog);

          if (file_save_dialog_save_image (save_dialog,
                                           dialog->gimage,
                                           uri,
                                           uri,
                                           dialog->file_proc,
                                           dialog->save_a_copy))
            {
              if (dialog)
                gtk_widget_hide (save_dialog);
            }

          if (dialog)
            {
              gimp_file_dialog_set_sensitive (dialog, TRUE);
              g_signal_handler_disconnect (dialog, handler_id);
            }
        }

      g_free (filename);
      g_free (uri);
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
  OverwriteData *overwrite_data = g_new0 (OverwriteData, 1);
  GtkWidget     *dialog;
  gchar         *filename;

  overwrite_data->save_dialog  = save_dialog;
  overwrite_data->uri          = g_strdup (uri);
  overwrite_data->raw_filename = g_strdup (raw_filename);

  dialog = gimp_message_dialog_new (_("File exists"), GIMP_STOCK_WARNING,
                                    save_dialog, GTK_DIALOG_DESTROY_WITH_PARENT,
                                    gimp_standard_help_func, NULL,

                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    _("_Replace"),    GTK_RESPONSE_OK,

                                    NULL);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_save_overwrite_response),
                    overwrite_data);

  filename = file_utils_uri_to_utf8_filename (uri);
  gimp_message_box_set_primary_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                                     _("A file named '%s' already exists."),
                                     filename);
  g_free (filename);

  gimp_message_box_set_text (GIMP_MESSAGE_DIALOG (dialog)->box,
                             _("Do you want to replace it with the image "
                               "you are saving?"));

  gimp_file_dialog_set_sensitive (GIMP_FILE_DIALOG (save_dialog), FALSE);
  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, FALSE);

  gtk_widget_show (dialog);
}

static void
file_save_overwrite_response (GtkWidget *dialog,
                              gint       response_id,
                              gpointer   data)
{
  OverwriteData  *overwrite_data = data;
  GimpFileDialog *save_dialog = GIMP_FILE_DIALOG (overwrite_data->save_dialog);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (save_dialog),
                                     GTK_RESPONSE_CANCEL, TRUE);

  gtk_widget_destroy (dialog);

  if (response_id == GTK_RESPONSE_OK)
    {
      if (file_save_dialog_save_image (overwrite_data->save_dialog,
                                       save_dialog->gimage,
                                       overwrite_data->uri,
                                       overwrite_data->raw_filename,
                                       save_dialog->file_proc,
                                       save_dialog->save_a_copy))
        {
          gtk_widget_hide (overwrite_data->save_dialog);
        }
    }

  gimp_file_dialog_set_sensitive (save_dialog, TRUE);

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
                             gboolean       save_a_copy)
{
  GimpPDBStatusType  status;
  GError            *error = NULL;

  g_object_ref (gimage);

  status = file_save_as (gimage,
                         gimp_get_user_context (gimage->gimp),
                         GIMP_PROGRESS (save_dialog),
                         uri,
                         raw_filename,
                         save_proc,
                         GIMP_RUN_INTERACTIVE,
                         save_a_copy,
                         &error);

  if (status == GIMP_PDB_SUCCESS)
    g_object_set_data_full (G_OBJECT (gimage->gimp), "gimp-file-save-last-uri",
                            g_strdup (uri), (GDestroyNotify) g_free);

  g_object_unref (gimage);

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
