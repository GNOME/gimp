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
#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpcoreconfig.h"
#include "core/gimpdocuments.h"
#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimpimagefile.h"

#include "plug-in/plug-in-types.h"
#include "plug-in/plug-in-proc.h"

#include "file/file-open.h"
#include "file/file-utils.h"

#include "widgets/gimpitemfactory.h"
#include "widgets/gimppreview.h"

#include "file-dialog-utils.h"
#include "file-open-dialog.h"

#include "undo.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static GtkWidget * file_open_dialog_create          (Gimp             *gimp);
static void        file_open_type_callback          (GtkWidget        *widget,
                                                     gpointer          data);
static void        file_open_selchanged_callback    (GtkTreeSelection *sel,
                                                     GtkWidget        *open_dialog);
static void        file_open_imagefile_info_changed (GimpImagefile    *imagefile,
                                                     GtkLabel         *label);
static gboolean    file_open_thumbnail_button_press (GtkWidget        *widget,
                                                     GdkEventButton   *bevent,
                                                     GtkWidget        *open_dialog);
static void        file_open_thumbnail_clicked      (GtkWidget        *widget,
                                                     GtkWidget        *open_dialog);
static void        file_open_ok_callback            (GtkWidget        *widget,
                                                     GtkWidget        *open_dialog);
static void        file_open_dialog_open_image      (GtkWidget        *open_dialog,
                                                     Gimp             *gimp,
                                                     const gchar      *uri,
                                                     const gchar      *entered_filename,
                                                     PlugInProcDef    *load_proc);


static GtkWidget     *fileload               = NULL;

static GtkWidget     *open_options_frame     = NULL;
static GtkWidget     *open_options_title     = NULL;
static GimpImagefile *open_options_imagefile = NULL;
static GtkWidget     *open_options_preview   = NULL;

static PlugInProcDef *load_file_proc = NULL;


/*  public functions  */

void
file_open_dialog_menu_init (Gimp            *gimp,
                            GimpItemFactory *item_factory)
{
  GimpItemFactoryEntry  entry;
  PlugInProcDef        *file_proc;
  GSList               *list;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GIMP_IS_ITEM_FACTORY (item_factory));

  gimp->load_procs = g_slist_reverse (gimp->load_procs);

  for (list = gimp->load_procs; list; list = g_slist_next (list))
    {
      gchar *basename;
      gchar *lowercase_basename;
      gchar *help_page;

      file_proc = (PlugInProcDef *) list->data;

      basename = g_path_get_basename (file_proc->prog);

      lowercase_basename = g_ascii_strdown (basename, -1);

      g_free (basename);

      help_page = g_strconcat ("filters/",
			       lowercase_basename,
			       ".html",
			       NULL);

      g_free (lowercase_basename);

      entry.entry.path            = strstr (file_proc->menu_path, "/");
      entry.entry.accelerator     = NULL;
      entry.entry.callback        = file_open_type_callback;
      entry.entry.callback_action = 0;
      entry.entry.item_type       = NULL;
      entry.quark_string          = NULL;
      entry.help_page             = help_page;
      entry.description           = NULL;

      gimp_item_factory_create_item (item_factory,
                                     &entry,
                                     file_proc, 2,
                                     TRUE, FALSE);
    }
}

void
file_open_dialog_menu_reset (void)
{
  load_file_proc = NULL;
}

void
file_open_dialog_show (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! fileload)
    fileload = file_open_dialog_create (gimp);

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  if (GTK_WIDGET_VISIBLE (fileload))
    return;

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileload),
				   "." G_DIR_SEPARATOR_S);
  gtk_window_set_title (GTK_WINDOW (fileload), _("Open Image"));

  file_dialog_show (fileload);
}


/*  private functions  */

static GtkWidget *
file_open_dialog_create (Gimp *gimp)
{
  GtkWidget        *open_dialog;
  GtkFileSelection *fs;
  GtkTreeSelection *tree_sel;

  open_dialog = file_dialog_new (gimp,
                                 gimp_item_factory_from_path ("<Load>"),
                                 _("Open Image"), "open_image",
                                 "open/dialogs/file_open.html",
                                 G_CALLBACK (file_open_ok_callback));

  fs = GTK_FILE_SELECTION (open_dialog);

  gtk_file_selection_set_select_multiple (fs, TRUE);
  tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (fs->file_list));

  /* Catch file-list clicks so we can update the preview thumbnail */
  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (file_open_selchanged_callback),
                    open_dialog);

  /*  The preview frame  */
  {
    GtkWidget *vbox;
    GtkWidget *ebox;
    GtkWidget *hbox;
    GtkWidget *button;
    GtkWidget *label;
    GtkStyle  *style;

    open_options_frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (open_options_frame), GTK_SHADOW_IN);

    ebox = gtk_event_box_new ();

    gtk_widget_ensure_style (ebox);
    style = gtk_widget_get_style (ebox);
    gtk_widget_modify_bg (ebox, GTK_STATE_NORMAL,
                          &style->base[GTK_STATE_NORMAL]);
    gtk_widget_modify_bg (ebox, GTK_STATE_INSENSITIVE,
                          &style->base[GTK_STATE_NORMAL]);

    gtk_container_add (GTK_CONTAINER (open_options_frame), ebox);
    gtk_widget_show (ebox);

    g_signal_connect (G_OBJECT (ebox), "button_press_event",
		      G_CALLBACK (file_open_thumbnail_button_press),
                      open_dialog);

    gimp_help_set_help_data (ebox, _("Click to create preview"), NULL);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_add (GTK_CONTAINER (ebox), vbox);
    gtk_widget_show (vbox);

    button = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    label = gtk_label_new_with_mnemonic (_("_Preview"));
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_container_add (GTK_CONTAINER (button), label);
    gtk_widget_show (label);

    g_signal_connect (G_OBJECT (button), "button_press_event",
                      G_CALLBACK (gtk_true),
                      NULL);
    g_signal_connect (G_OBJECT (button), "button_release_event",
                      G_CALLBACK (gtk_true),
                      NULL);
    g_signal_connect (G_OBJECT (button), "enter_notify_event",
                      G_CALLBACK (gtk_true),
                      NULL);
    g_signal_connect (G_OBJECT (button), "leave_notify_event",
                      G_CALLBACK (gtk_true),
                      NULL);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    open_options_imagefile = gimp_imagefile_new (NULL);

    open_options_preview =
      gimp_preview_new (GIMP_VIEWABLE (open_options_imagefile),
                        GIMP_IMAGEFILE_THUMB_SIZE_NORMAL, 0, FALSE);

    gtk_widget_ensure_style (open_options_preview);
    style = gtk_widget_get_style (open_options_preview);
    gtk_widget_modify_bg (open_options_preview, GTK_STATE_NORMAL,
                          &style->base[GTK_STATE_NORMAL]);
    gtk_widget_modify_bg (open_options_preview, GTK_STATE_INSENSITIVE,
                          &style->base[GTK_STATE_NORMAL]);

    gtk_box_pack_start (GTK_BOX (hbox), open_options_preview, TRUE, FALSE, 4);
    gtk_widget_show (open_options_preview);

    gtk_label_set_mnemonic_widget (GTK_LABEL (label), open_options_preview);
    g_signal_connect (G_OBJECT (open_options_preview), "clicked",
		      G_CALLBACK (file_open_thumbnail_clicked),
                      open_dialog);

    open_options_title = gtk_label_new (_("No Selection"));
    gtk_box_pack_start (GTK_BOX (vbox), open_options_title, FALSE, FALSE, 0);
    gtk_widget_show (open_options_title);

    label = gtk_label_new (" \n ");
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    g_signal_connect (G_OBJECT (open_options_imagefile), "info_changed",
                      G_CALLBACK (file_open_imagefile_info_changed),
                      label);

    /* pack the containing open_options hbox into the open-dialog */
    for (hbox = fs->dir_list; ! GTK_IS_HBOX (hbox); hbox = hbox->parent);

    gtk_box_pack_end (GTK_BOX (hbox), open_options_frame, FALSE, FALSE, 0);
    gtk_widget_show (open_options_frame);

    gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), FALSE);
  }

  return open_dialog;
}

static void
file_open_type_callback (GtkWidget *widget,
			 gpointer   data)
{
  PlugInProcDef *proc = (PlugInProcDef *) data;

  file_dialog_update_name (proc, GTK_FILE_SELECTION (fileload));

  load_file_proc = proc;
}

static void
file_open_imagefile_info_changed (GimpImagefile *imagefile,
                                  GtkLabel      *label)
{
  gtk_label_set_text (label, gimp_imagefile_get_description (imagefile));
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
  if (selected)
    {
      gchar *uri;
      gchar *basename;

      fs = GTK_FILE_SELECTION (open_dialog);

      gimp = GIMP (g_object_get_data (G_OBJECT (open_dialog), "gimp"));

      fullfname = gtk_file_selection_get_filename (fs);

      uri      = file_utils_filename_to_uri (gimp->load_procs, fullfname, NULL);
      basename = file_utils_uri_to_utf8_basename (uri);

      gimp_object_set_name (GIMP_OBJECT (open_options_imagefile), uri);
      gtk_label_set_text (GTK_LABEL (open_options_title), basename);

      g_free (uri);
      g_free (basename);
    }
  else
    {
      gimp_object_set_name (GIMP_OBJECT (open_options_imagefile), NULL);
      gtk_label_set_text (GTK_LABEL (open_options_title), _("No Selection"));
    }

  gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), selected);
  gimp_imagefile_update (open_options_imagefile);
}

static void
file_open_create_thumbnail (const gchar *filename)
{
  if (g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      GimpImagefile *imagefile;
      gchar         *basename;
      gchar         *uri;

      uri = g_filename_to_uri (filename, NULL, NULL);

      imagefile = gimp_imagefile_new (uri);
      gimp_imagefile_create_thumbnail (imagefile);
      g_object_unref (G_OBJECT (imagefile));

      basename = file_utils_uri_to_utf8_basename (uri);
      gtk_label_set_text (GTK_LABEL (open_options_title), basename);
      g_free (basename);

      gimp_object_set_name (GIMP_OBJECT (open_options_imagefile), uri);
      gimp_imagefile_update (open_options_imagefile);

      g_free (uri);
    }
}

static gboolean
file_open_thumbnail_button_press (GtkWidget      *widget,
                                  GdkEventButton *bevent,
                                  GtkWidget      *open_dialog)
{
  file_open_thumbnail_clicked (widget, open_dialog);

  return TRUE;
}

static void
file_open_thumbnail_clicked (GtkWidget *widget,
                             GtkWidget *open_dialog)
{
  GtkFileSelection  *fs;
  Gimp              *gimp;

  fs = GTK_FILE_SELECTION (open_dialog);

  gimp = GIMP (g_object_get_data (G_OBJECT (fs), "gimp"));

  if (gimp->config->write_thumbnails)
    {
      gchar **selections;
      gint    i;

      gimp_set_busy (gimp);
      gtk_widget_set_sensitive (open_dialog, FALSE);

      selections = gtk_file_selection_get_selections (fs);

      for (i = 1; selections[i] != NULL; i++)
        file_open_create_thumbnail (selections[i]);

      if (selections[0])
        file_open_create_thumbnail (selections[0]);

      g_strfreev (selections);

      gtk_widget_set_sensitive (open_dialog, TRUE);
      gimp_unset_busy (gimp);
    }
}

static void
file_open_ok_callback (GtkWidget *widget,
		       GtkWidget *open_dialog)
{
  GtkFileSelection  *fs;
  Gimp              *gimp;
  gchar            **selections;
  gchar             *uri;
  const gchar       *entered_filename;
  gint               i;

  fs = GTK_FILE_SELECTION (open_dialog);

  gimp = GIMP (g_object_get_data (G_OBJECT (open_dialog), "gimp"));

  selections = gtk_file_selection_get_selections (fs);

  if (selections == NULL)
    return;

  entered_filename = gtk_entry_get_text (GTK_ENTRY (fs->selection_entry));

  if (g_file_test (selections[0], G_FILE_TEST_EXISTS))
    {
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

      uri = g_filename_to_uri (selections[0], NULL, NULL);
    }
  else
    {
      /* try with the entered filename in case the user typed an URI */

      uri = g_strdup (entered_filename);
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
  GimpPDBStatusType status;

  status = file_open_with_proc_and_display (gimp,
                                            uri,
                                            entered_filename, 
                                            load_proc);

  if (status == GIMP_PDB_SUCCESS)
    {
      file_dialog_hide (open_dialog);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      /* Hackery required. Please add error message. --bex */
      g_message (_("Opening '%s' failed."), uri);
    }
}
