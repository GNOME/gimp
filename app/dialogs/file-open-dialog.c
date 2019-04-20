/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"

#include "file/file-open.h"
#include "file/gimp-file.h"

#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpopendialog.h"
#include "widgets/gimpwidgets-utils.h"

#include "file-open-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       file_open_dialog_response    (GtkWidget           *dialog,
                                                gint                 response_id,
                                                Gimp                *gimp);
static GimpImage *file_open_dialog_open_image  (GtkWidget           *dialog,
                                                Gimp                *gimp,
                                                GFile               *file,
                                                GimpPlugInProcedure *load_proc);
static gboolean   file_open_dialog_open_layers (GtkWidget           *dialog,
                                                GimpImage           *image,
                                                GFile               *file,
                                                GimpPlugInProcedure *load_proc);


/*  public functions  */

GtkWidget *
file_open_dialog_new (Gimp *gimp)
{
  GtkWidget           *dialog;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_open_dialog_new (gimp);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  gimp_file_dialog_load_state (GIMP_FILE_DIALOG (dialog),
                               "gimp-file-open-dialog-state");

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_dialog_response),
                    gimp);

  return dialog;
}


/*  private functions  */

static void
file_open_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GimpFileDialog *file_dialog = GIMP_FILE_DIALOG (dialog);
  GimpOpenDialog *open_dialog = GIMP_OPEN_DIALOG (dialog);
  GSList         *files;
  GSList         *list;
  gboolean        success = FALSE;

  gimp_file_dialog_save_state (GIMP_FILE_DIALOG (dialog),
                               "gimp-file-open-dialog-state");

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! file_dialog->busy)
        gtk_widget_destroy (dialog);

      return;
    }

  files = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));

  if (files)
    g_object_set_data_full (G_OBJECT (gimp), GIMP_FILE_OPEN_LAST_FILE_KEY,
                            g_object_ref (files->data),
                            (GDestroyNotify) g_object_unref);

  gimp_file_dialog_set_sensitive (file_dialog, FALSE);

  /* When we are going to open new image windows, unset the transient
   * window. We don't need it since we will use gdk_window_raise() to
   * keep the dialog on top. And if we don't do it, then the dialog
   * will pull the image window it was invoked from on top of all the
   * new opened image windows, and we don't want that to happen.
   */
  if (! open_dialog->open_as_layers)
    gtk_window_set_transient_for (GTK_WINDOW (dialog), NULL);

  if (file_dialog->image)
    g_object_ref (file_dialog->image);

  for (list = files; list; list = g_slist_next (list))
    {
      GFile *file = list->data;

      if (open_dialog->open_as_layers)
        {
          if (! file_dialog->image)
            {
              gimp_open_dialog_set_image (
                open_dialog,
                file_open_dialog_open_image (dialog,
                                             gimp,
                                             file,
                                             file_dialog->file_proc),
                TRUE);

              if (file_dialog->image)
                {
                  g_object_ref (file_dialog->image);
                  success = TRUE;
                }
            }
          else if (file_open_dialog_open_layers (dialog,
                                                 file_dialog->image,
                                                 file,
                                                 file_dialog->file_proc))
            {
              success = TRUE;
            }
        }
      else
        {
          if (file_open_dialog_open_image (dialog,
                                           gimp,
                                           file,
                                           file_dialog->file_proc))
            {
              success = TRUE;

              /* Make the dialog stay on top of all images we open if
               * we open say 10 at once
               */
              gdk_window_raise (gtk_widget_get_window (dialog));
            }
        }

      if (file_dialog->canceled)
        break;
    }

  if (success)
    {
      if (file_dialog->image)
        {
          if (open_dialog->open_as_layers)
            gimp_image_flush (file_dialog->image);

          g_object_unref (file_dialog->image);
        }

      gtk_widget_destroy (dialog);
    }
  else
    {
      if (file_dialog->image)
        g_object_unref (file_dialog->image);

      gimp_file_dialog_set_sensitive (file_dialog, TRUE);
    }

  g_slist_free_full (files, (GDestroyNotify) g_object_unref);
}

static GimpImage *
file_open_dialog_open_image (GtkWidget           *dialog,
                             Gimp                *gimp,
                             GFile               *file,
                             GimpPlugInProcedure *load_proc)
{
  GimpImage         *image;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  image = file_open_with_proc_and_display (gimp,
                                           gimp_get_user_context (gimp),
                                           GIMP_PROGRESS (dialog),
                                           file, file, FALSE,
                                           load_proc,
                                           G_OBJECT (gtk_widget_get_screen (dialog)),
                                           gimp_widget_get_monitor (dialog),
                                           &status, &error);

  if (! image && status != GIMP_PDB_CANCEL)
    {
      gimp_message (gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  return image;
}

static gboolean
file_open_dialog_open_layers (GtkWidget           *dialog,
                              GimpImage           *image,
                              GFile               *file,
                              GimpPlugInProcedure *load_proc)
{
  GList             *new_layers;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  new_layers = file_open_layers (image->gimp,
                                 gimp_get_user_context (image->gimp),
                                 GIMP_PROGRESS (dialog),
                                 image, FALSE,
                                 file, GIMP_RUN_INTERACTIVE, load_proc,
                                 &status, &error);

  if (new_layers)
    {
      gimp_image_add_layers (image, new_layers,
                             GIMP_IMAGE_ACTIVE_PARENT, -1,
                             0, 0,
                             gimp_image_get_width (image),
                             gimp_image_get_height (image),
                             _("Open layers"));

      g_list_free (new_layers);

      return TRUE;
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      gimp_message (image->gimp, G_OBJECT (dialog), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    gimp_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  return FALSE;
}
