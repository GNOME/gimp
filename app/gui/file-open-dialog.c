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

#include "config/gimpcoreconfig.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpimagefile.h"

#include "plug-in/plug-in-proc.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpfiledialog.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"

#include "dialogs.h"
#include "file-dialog-utils.h"
#include "file-open-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * file_open_dialog_create     (Gimp            *gimp,
                                                GimpMenuFactory *menu_factory);
static void        file_open_dialog_response   (GtkWidget       *open_dialog,
                                                gint             response_id,
                                                Gimp            *gimp);
static gboolean    file_open_dialog_open_image (GtkWidget       *open_dialog,
                                                Gimp            *gimp,
                                                const gchar     *uri,
                                                const gchar     *entered_filename,
                                                PlugInProcDef   *load_proc);


/*  private variables  */

static GtkWidget *fileload  = NULL;


/*  public functions  */

void
file_open_dialog_show (Gimp            *gimp,
                       GimpImage       *gimage,
                       const gchar     *uri,
                       GimpMenuFactory *menu_factory,
                       GtkWidget       *parent)
{
  gchar *filename = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (gimage == NULL || GIMP_IS_IMAGE (gimage));
  g_return_if_fail (GIMP_IS_MENU_FACTORY (menu_factory));
  g_return_if_fail (parent == NULL || GTK_IS_WIDGET (parent));

  if (! fileload)
    fileload = file_open_dialog_create (gimp, menu_factory);

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);

  if (GTK_WIDGET_VISIBLE (fileload))
    {
      gtk_window_present (GTK_WINDOW (fileload));
      return;
    }

  if (gimage)
    {
      filename = gimp_image_get_filename (gimage);

      if (filename)
        {
          gchar *dirname;

          dirname = g_path_get_dirname (filename);
          g_free (filename);

          filename = g_build_filename (dirname, ".", NULL);
          g_free (dirname);
        }
    }
  else if (uri)
    {
      filename = g_filename_from_uri (uri, NULL, NULL);
    }

  gtk_window_set_title (GTK_WINDOW (fileload), _("Open Image"));

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileload),
				   filename ?
				   filename :
                                   "." G_DIR_SEPARATOR_S);

  g_free (filename);

  file_dialog_show (fileload, parent);
}


/*  private functions  */

static GtkWidget *
file_open_dialog_create (Gimp            *gimp,
                         GimpMenuFactory *menu_factory)
{
  GtkWidget *dialog;

  dialog = gimp_file_dialog_new (gimp, gimp->load_procs,
                                 menu_factory, "<Load>",
                                 _("Open Image"), "gimp-file-open",
                                 GTK_STOCK_OPEN,
                                 GIMP_HELP_FILE_OPEN);

  gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (dialog), TRUE);

  gimp_dialog_factory_add_foreign (global_dialog_factory,
                                   "gimp-file-open-dialog",
                                   dialog);

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
  GtkFileSelection  *fs;
  gchar            **selections;
  gchar             *uri;
  const gchar       *entered_filename;
  gint               i;

  if (response_id != GTK_RESPONSE_OK)
    {
      file_dialog_hide (open_dialog);
      return;
    }

  fs = GTK_FILE_SELECTION (open_dialog);

  selections = gtk_file_selection_get_selections (fs);

  if (selections == NULL)
    return;

  entered_filename = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));

  if (g_file_test (selections[0], G_FILE_TEST_IS_DIR))
    {
      if (selections[0][strlen (selections[0]) - 1] != G_DIR_SEPARATOR)
        {
          gchar *s = g_strconcat (selections[0], G_DIR_SEPARATOR_S, NULL);
          gtk_file_selection_set_filename (fs, s);
          g_free (s);
        }
      else
        {
          gtk_file_selection_set_filename (fs, selections[0]);
        }

      g_strfreev (selections);

      return;
    }

  if (strstr (entered_filename, "://"))
    {
      /* try with the entered filename if it looks like an URI */

      uri = g_strdup (entered_filename);
    }
  else
    {
      uri = g_filename_to_uri (selections[0], NULL, NULL);
    }

  gtk_widget_set_sensitive (open_dialog, FALSE);

  if (file_open_dialog_open_image (open_dialog,
                                   gimp,
                                   uri,
                                   entered_filename,
                                   GIMP_FILE_DIALOG (open_dialog)->file_proc))
    {
      file_dialog_hide (open_dialog);
    }

  g_free (uri);

  /*
   * Now deal with multiple selections from the filesel list
   */

  for (i = 1; selections[i] != NULL; i++)
    {
      if (g_file_test (selections[i], G_FILE_TEST_IS_REGULAR))
	{
          uri = g_filename_to_uri (selections[i], NULL, NULL);

	  if (file_open_dialog_open_image (open_dialog,
                                           gimp,
                                           uri,
                                           uri,
                                           GIMP_FILE_DIALOG (open_dialog)->file_proc))
            {
              file_dialog_hide (open_dialog);
            }

          g_free (uri);
	}
    }

  g_strfreev (selections);

  gtk_widget_set_sensitive (open_dialog, TRUE);
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
