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

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"

#include "dialogs.h"
#include "file-open-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * file_open_dialog_create     (Gimp          *gimp);
static void        file_open_dialog_response   (GtkWidget     *open_dialog,
                                                gint           response_id,
                                                Gimp          *gimp);
static gboolean    file_open_dialog_open_image (GtkWidget     *open_dialog,
                                                Gimp          *gimp,
                                                const gchar   *uri,
                                                const gchar   *entered_filename,
                                                PlugInProcDef *load_proc);


/*  private variables  */

static GtkWidget *fileload  = NULL;


/*  public functions  */

void
file_open_dialog_show (Gimp        *gimp,
                       GimpImage   *gimage,
                       const gchar *uri,
                       GtkWidget   *parent)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  if (! fileload)
    fileload = file_open_dialog_create (gimp);

  gimp_file_dialog_set_uri (GIMP_FILE_DIALOG (fileload), gimage, uri);

  if (parent)
    gtk_window_set_screen (GTK_WINDOW (fileload),
                           gtk_widget_get_screen (parent));

  gtk_window_present (GTK_WINDOW (fileload));
}


/*  private functions  */

static GtkWidget *
file_open_dialog_create (Gimp *gimp)
{
  GtkWidget *dialog;

  dialog = gimp_file_dialog_new (gimp,
                                 GTK_FILE_CHOOSER_ACTION_OPEN,
                                 _("Open Image"), "gimp-file-open",
                                 GTK_STOCK_OPEN,
                                 GIMP_HELP_FILE_OPEN);

  gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);

  gimp_dialog_factory_add_foreign (global_dialog_factory,
                                   "gimp-file-open-dialog", dialog);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (file_open_dialog_response),
                    gimp);

  return dialog;
}

static void
file_open_dialog_response (GtkWidget *open_dialog,
                           gint       response_id,
                           Gimp      *gimp)
{
  GSList   *uris;
  GSList   *list;

  if (response_id != GTK_RESPONSE_OK)
    {
      gtk_widget_hide (open_dialog);
      return;
    }

  uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (open_dialog));

  gtk_widget_set_sensitive (open_dialog, FALSE);

  for (list = uris; list; list = g_slist_next (list))
    {
      gchar *filename = g_filename_from_uri (list->data, NULL, NULL);

      if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
        {
          if (file_open_dialog_open_image (open_dialog,
                                           gimp,
                                           list->data,
                                           list->data,
                                           GIMP_FILE_DIALOG (open_dialog)->file_proc))
            {
              gtk_widget_hide (open_dialog);
            }
        }

      g_free (filename);
    }

  gtk_widget_set_sensitive (open_dialog, TRUE);

  g_slist_foreach (uris, (GFunc) g_free, NULL);
  g_slist_free (uris);
}

static gboolean
file_open_dialog_open_image (GtkWidget     *open_dialog,
                             Gimp          *gimp,
                             const gchar   *uri,
                             const gchar   *entered_filename,
                             PlugInProcDef *load_proc)
{
  GimpImage         *gimage;
  GimpPDBStatusType  status;
  GError            *error = NULL;

  gimage = file_open_with_proc_and_display (gimp,
                                            gimp_get_user_context (gimp),
                                            uri,
                                            entered_filename,
                                            load_proc,
                                            &status,
                                            &error);

  if (gimage)
    {
      return TRUE;
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      gchar *filename = file_utils_uri_to_utf8_filename (uri);

      g_message (_("Opening '%s' failed:\n\n%s"),
                 filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }

  return FALSE;
}
