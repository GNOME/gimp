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

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpthumb/gimpthumb.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "config/gimpcoreconfig.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimagefile.h"

#include "plug-in/plug-in-proc.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimppreview.h"
#include "widgets/gimpthumbbox.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs.h"
#include "file-dialog-utils.h"
#include "file-open-dialog.h"

#include "gimp-intl.h"


/*  local function prototypes  */

static GtkWidget * file_open_dialog_create       (Gimp             *gimp,
                                                  GimpMenuFactory  *menu_factory);
static void        file_open_selchanged_callback (GtkTreeSelection *sel,
                                                  GtkWidget        *open_dialog);
static void        file_open_response_callback   (GtkWidget        *open_dialog,
                                                  gint              response_id,
                                                  Gimp             *gimp);
static void        file_open_dialog_open_image   (GtkWidget        *open_dialog,
                                                  Gimp             *gimp,
                                                  const gchar      *uri,
                                                  const gchar      *entered_filename,
                                                  PlugInProcDef    *load_proc);


static GtkWidget     *fileload       = NULL;
static GtkWidget     *thumb_box      = NULL;
static PlugInProcDef *load_file_proc = NULL;


/*  public functions  */

void
file_open_dialog_set_type (PlugInProcDef *proc)
{
  /* Don't call file_dialog_update_name() here, see bug #112273.  */

  load_file_proc = proc;
}

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
  GtkWidget        *open_dialog;
  GtkFileSelection *fs;

  open_dialog = file_dialog_new (gimp,
                                 global_dialog_factory,
                                 "gimp-file-open-dialog",
                                 menu_factory, "<Load>",
                                 _("Open Image"), "gimp-file-open",
                                 GIMP_HELP_FILE_OPEN);

  g_signal_connect (open_dialog, "response",
                    G_CALLBACK (file_open_response_callback),
                    gimp);

  fs = GTK_FILE_SELECTION (open_dialog);

  gtk_file_selection_set_select_multiple (fs, TRUE);

  /*  The preview frame  */
  if (gimp->config->thumbnail_size > 0)
    {
      GtkTreeSelection *tree_sel;
      GtkWidget        *hbox;

      tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fs->file_list));

      /* Catch file-list clicks so we can update the preview thumbnail */
      g_signal_connect (tree_sel, "changed",
                        G_CALLBACK (file_open_selchanged_callback),
                        open_dialog);

      /* pack the containing open_options hbox into the open-dialog */
      for (hbox = fs->dir_list; ! GTK_IS_HBOX (hbox); hbox = hbox->parent);

      thumb_box = gimp_thumb_box_new (gimp);
      gtk_box_pack_end (GTK_BOX (hbox), thumb_box, FALSE, FALSE, 0);
      gtk_widget_show (thumb_box);

      gtk_widget_set_sensitive (GTK_WIDGET (thumb_box), FALSE);
    }

  return open_dialog;
}

static void
selchanged_foreach (GtkTreeModel *model,
		    GtkTreePath  *path,
		    GtkTreeIter  *iter,
		    gpointer      data)
{
  gboolean *selected = data;

  *selected = TRUE;
}

static void
file_open_selchanged_callback (GtkTreeSelection *sel,
			       GtkWidget        *open_dialog)
{
  GtkFileSelection *fs;
  Gimp             *gimp;
  const gchar      *fullfname;
  gboolean          selected = FALSE;

  gtk_tree_selection_selected_foreach (sel,
				       selchanged_foreach,
				       &selected);

  fs = GTK_FILE_SELECTION (open_dialog);

  gimp = GIMP (g_object_get_data (G_OBJECT (open_dialog), "gimp"));

  if (selected)
    {
      gchar *uri;

      fullfname = gtk_file_selection_get_filename (fs);

      uri = file_utils_filename_to_uri (gimp->load_procs, fullfname, NULL);
      gimp_thumb_box_set_uri (GIMP_THUMB_BOX (thumb_box), uri);
      g_free (uri);
    }
  else
    {
      gimp_thumb_box_set_uri (GIMP_THUMB_BOX (thumb_box), NULL);
    }

  {
    gchar **selections;
    GSList *uris = NULL;
    gint    i;

    selections = gtk_file_selection_get_selections (fs);

    for (i = 0; selections[i] != NULL; i++)
      uris = g_slist_prepend (uris, g_filename_to_uri (selections[i],
                                                       NULL, NULL));

    g_strfreev (selections);

    uris = g_slist_reverse (uris);

    gimp_thumb_box_set_uris (GIMP_THUMB_BOX (thumb_box), uris);
  }
}

static void
file_open_response_callback (GtkWidget *open_dialog,
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

  file_open_dialog_open_image (open_dialog,
                               gimp,
                               uri,
                               entered_filename,
                               load_file_proc);

  g_free (uri);

  /*
   * Now deal with multiple selections from the filesel list
   */

  for (i = 1; selections[i] != NULL; i++)
    {
      if (g_file_test (selections[i], G_FILE_TEST_IS_REGULAR))
	{
          uri = g_filename_to_uri (selections[i], NULL, NULL);

	  file_open_dialog_open_image (open_dialog,
                                       gimp,
                                       uri,
                                       uri,
                                       load_file_proc);

          g_free (uri);
	}
    }

  g_strfreev (selections);

  gtk_widget_set_sensitive (open_dialog, TRUE);
}

static void
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
      file_dialog_hide (open_dialog);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      gchar *filename;

      filename = file_utils_uri_to_utf8_filename (uri);

      g_message (_("Opening '%s' failed:\n\n%s"),
                 filename, error->message);
      g_clear_error (&error);

      g_free (filename);
    }
}
