/* LIGMA - The GNU Image Manipulation Program
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

#include "libligmabase/ligmabase.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-undo.h"
#include "core/ligmalayer.h"
#include "core/ligmaprogress.h"

#include "file/file-open.h"
#include "file/ligma-file.h"

#include "widgets/ligmafiledialog.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmaopendialog.h"
#include "widgets/ligmawidgets-utils.h"

#include "file-open-dialog.h"

#include "ligma-intl.h"


/*  local function prototypes  */

static void       file_open_dialog_response    (GtkWidget           *dialog,
                                                gint                 response_id,
                                                Ligma                *ligma);
static LigmaImage *file_open_dialog_open_image  (GtkWidget           *dialog,
                                                Ligma                *ligma,
                                                GFile               *file,
                                                LigmaPlugInProcedure *load_proc);
static gboolean   file_open_dialog_open_layers (GtkWidget           *dialog,
                                                LigmaImage           *image,
                                                GFile               *file,
                                                LigmaPlugInProcedure *load_proc);


/*  public functions  */

GtkWidget *
file_open_dialog_new (Ligma *ligma)
{
  GtkWidget           *dialog;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), NULL);

  dialog = ligma_open_dialog_new (ligma);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  ligma_file_dialog_load_state (LIGMA_FILE_DIALOG (dialog),
                               "ligma-file-open-dialog-state");

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_dialog_response),
                    ligma);

  return dialog;
}


/*  private functions  */

static void
file_open_dialog_response (GtkWidget *dialog,
                           gint       response_id,
                           Ligma      *ligma)
{
  LigmaFileDialog *file_dialog = LIGMA_FILE_DIALOG (dialog);
  LigmaOpenDialog *open_dialog = LIGMA_OPEN_DIALOG (dialog);
  GSList         *files;
  GSList         *list;
  gboolean        success = FALSE;

  ligma_file_dialog_save_state (LIGMA_FILE_DIALOG (dialog),
                               "ligma-file-open-dialog-state");

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! file_dialog->busy && response_id != GTK_RESPONSE_HELP)
        gtk_widget_destroy (dialog);

      return;
    }

  files = gtk_file_chooser_get_files (GTK_FILE_CHOOSER (dialog));

  if (files)
    g_object_set_data_full (G_OBJECT (ligma), LIGMA_FILE_OPEN_LAST_FILE_KEY,
                            g_object_ref (files->data),
                            (GDestroyNotify) g_object_unref);

  ligma_file_dialog_set_sensitive (file_dialog, FALSE);

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
              ligma_open_dialog_set_image (
                open_dialog,
                file_open_dialog_open_image (dialog,
                                             ligma,
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
                                           ligma,
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
            ligma_image_flush (file_dialog->image);

          g_object_unref (file_dialog->image);
        }

      gtk_widget_destroy (dialog);
    }
  else
    {
      if (file_dialog->image)
        g_object_unref (file_dialog->image);

      ligma_file_dialog_set_sensitive (file_dialog, TRUE);
    }

  g_slist_free_full (files, (GDestroyNotify) g_object_unref);
}

static LigmaImage *
file_open_dialog_open_image (GtkWidget           *dialog,
                             Ligma                *ligma,
                             GFile               *file,
                             LigmaPlugInProcedure *load_proc)
{
  LigmaImage         *image;
  LigmaPDBStatusType  status;
  GError            *error = NULL;

  image = file_open_with_proc_and_display (ligma,
                                           ligma_get_user_context (ligma),
                                           LIGMA_PROGRESS (dialog),
                                           file, FALSE,
                                           load_proc,
                                           G_OBJECT (ligma_widget_get_monitor (dialog)),
                                           &status, &error);

  if (! image && status != LIGMA_PDB_SUCCESS && status != LIGMA_PDB_CANCEL)
    {
      ligma_message (ligma, G_OBJECT (dialog), LIGMA_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  return image;
}

static gboolean
file_open_dialog_open_layers (GtkWidget           *dialog,
                              LigmaImage           *image,
                              GFile               *file,
                              LigmaPlugInProcedure *load_proc)
{
  GList             *new_layers;
  LigmaPDBStatusType  status;
  GError            *error = NULL;

  new_layers = file_open_layers (image->ligma,
                                 ligma_get_user_context (image->ligma),
                                 LIGMA_PROGRESS (dialog),
                                 image, FALSE,
                                 file, LIGMA_RUN_INTERACTIVE, load_proc,
                                 &status, &error);

  if (new_layers)
    {
      ligma_image_add_layers (image, new_layers,
                             LIGMA_IMAGE_ACTIVE_PARENT, -1,
                             0, 0,
                             ligma_image_get_width (image),
                             ligma_image_get_height (image),
                             _("Open layers"));

      g_list_free (new_layers);

      return TRUE;
    }
  else if (status != LIGMA_PDB_CANCEL)
    {
      ligma_message (image->ligma, G_OBJECT (dialog), LIGMA_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"),
                    ligma_file_get_utf8_name (file), error->message);
      g_clear_error (&error);
    }

  return FALSE;
}
