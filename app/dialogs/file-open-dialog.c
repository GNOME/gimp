/* GIMP - The GNU Image Manipulation Program
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
#include "core/gimpimage-undo.h"
#include "core/gimplayer.h"
#include "core/gimpprogress.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"

#include "file-open-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static void       file_open_dialog_response    (GtkWidget           *open_dialog,
                                                gint                 response_id,
                                                Gimp                *gimp);
static gboolean   file_open_dialog_open_image  (GtkWidget           *open_dialog,
                                                Gimp                *gimp,
                                                const gchar         *uri,
                                                const gchar         *entered_filename,
                                                GimpPlugInProcedure *load_proc);
static gboolean   file_open_dialog_open_layers (GtkWidget           *open_dialog,
                                                GimpImage           *image,
                                                const gchar         *uri,
                                                const gchar         *entered_filename,
                                                GimpPlugInProcedure *load_proc);


/*  public functions  */

GtkWidget *
file_open_dialog_new (Gimp *gimp)
{
  GtkWidget *dialog;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  dialog = gimp_file_dialog_new (gimp,
                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                 _("Open Image"), "gimp-file-open",
                                 GTK_STOCK_OPEN,
                                 GIMP_HELP_FILE_OPEN);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_dialog_response),
                    gimp);

  return dialog;
}


/*  private functions  */

static void
file_open_dialog_response (GtkWidget *open_dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GimpFileDialog *dialog  = GIMP_FILE_DIALOG (open_dialog);
  GSList         *uris;
  GSList         *list;
  gboolean        success = FALSE;

  if (response_id != GTK_RESPONSE_OK)
    {
      if (! dialog->busy)
        gtk_widget_hide (open_dialog);

      return;
    }

  uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (open_dialog));

  gimp_file_dialog_set_sensitive (dialog, FALSE);

  for (list = uris; list; list = g_slist_next (list))
    {
      gchar *filename = file_utils_filename_from_uri (list->data);

      if (filename)
        {
          gboolean regular = g_file_test (filename, G_FILE_TEST_IS_REGULAR);

          g_free (filename);

          if (! regular)
            continue;
        }

      if (dialog->image)
        {
          if (file_open_dialog_open_layers (open_dialog,
                                            dialog->image,
                                            list->data,
                                            list->data,
                                            dialog->file_proc))
            {
              success = TRUE;
            }
        }
      else
        {
          if (file_open_dialog_open_image (open_dialog,
                                           gimp,
                                           list->data,
                                           list->data,
                                           dialog->file_proc))
            {
              success = TRUE;

              gdk_window_raise (open_dialog->window);
            }
        }

      if (dialog->canceled)
        break;
    }

  if (success)
    {
      gtk_widget_hide (open_dialog);

      if (dialog->image)
        gimp_image_flush (dialog->image);
    }

  gimp_file_dialog_set_sensitive (dialog, TRUE);

  g_slist_foreach (uris, (GFunc) g_free, NULL);
  g_slist_free (uris);
}

static gboolean
file_open_dialog_open_image (GtkWidget           *open_dialog,
                             Gimp                *gimp,
                             const gchar         *uri,
                             const gchar         *entered_filename,
                             GimpPlugInProcedure *load_proc)
{
  GimpImage         *image;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  image = file_open_with_proc_and_display (gimp,
                                           gimp_get_user_context (gimp),
                                           GIMP_PROGRESS (open_dialog),
                                           uri, entered_filename, FALSE,
                                           load_proc,
                                           &status, &error);

  if (image)
    {
      return TRUE;
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      gchar *filename = file_utils_uri_display_name (uri);

      gimp_message (gimp, G_OBJECT (open_dialog), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"), filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }

  return FALSE;
}

static gboolean
file_open_dialog_open_layers (GtkWidget           *open_dialog,
                              GimpImage           *image,
                              const gchar         *uri,
                              const gchar         *entered_filename,
                              GimpPlugInProcedure *load_proc)
{
  GList             *new_layers;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  new_layers = file_open_layers (image->gimp,
                                 gimp_get_user_context (image->gimp),
                                 GIMP_PROGRESS (open_dialog),
                                 image, FALSE,
                                 uri, GIMP_RUN_INTERACTIVE, load_proc,
                                 &status, &error);

  if (new_layers)
    {
      gimp_image_add_layers (image, new_layers, -1,
                             0, 0,
                             gimp_image_get_width (image),
                             gimp_image_get_height (image),
                             _("Open layers"));

      g_list_free (new_layers);

      gimp_image_flush (image);

      return TRUE;
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      gchar *filename = file_utils_uri_display_name (uri);

      gimp_message (image->gimp, G_OBJECT (open_dialog), GIMP_MESSAGE_ERROR,
                    _("Opening '%s' failed:\n\n%s"), filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }

  return FALSE;
}
