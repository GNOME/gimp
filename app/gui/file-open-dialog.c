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

#ifdef __GNUC__
#warning GTK_DISABLE_DEPRECATED
#endif
#undef GTK_DISABLE_DEPRECATED

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

static void     file_open_dialog_create          (Gimp             *gimp);
static void     file_open_genbutton_callback     (GtkWidget        *widget,
                                                  gpointer          data);
static void     file_open_selchanged_callback    (GtkTreeSelection *sel,
                                                  gpointer          data);
static void     file_open_ok_callback            (GtkWidget        *widget,
                                                  gpointer          data);
static void     file_open_type_callback          (GtkWidget        *widget,
                                                  gpointer          data);
static void     file_open_imagefile_info_changed (GimpImagefile    *imagefile,
                                                  gpointer          data);



static GtkWidget     *fileload               = NULL;
static GtkWidget     *open_options           = NULL;
static GimpImagefile *open_options_imagefile = NULL;

static GtkWidget     *open_options_label     = NULL;
static GtkWidget     *open_options_frame     = NULL;

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
    file_open_dialog_create (gimp);

  gtk_widget_set_sensitive (GTK_WIDGET (fileload), TRUE);
  if (GTK_WIDGET_VISIBLE (fileload))
    return;

  gtk_file_selection_set_filename (GTK_FILE_SELECTION (fileload),
				   "." G_DIR_SEPARATOR_S);
  gtk_window_set_title (GTK_WINDOW (fileload), _("Open Image"));

  file_dialog_show (fileload);
}


/*  private functions  */

static void
file_open_dialog_create (Gimp *gimp)
{
  GtkFileSelection *file_sel;
  GtkTreeSelection *tree_sel;

  fileload = gtk_file_selection_new (_("Open Image"));

  g_object_set_data (G_OBJECT (fileload), "gimp", gimp);

  gtk_window_set_position (GTK_WINDOW (fileload), GTK_WIN_POS_MOUSE);
  gtk_window_set_wmclass (GTK_WINDOW (fileload), "load_image", "Gimp");

  file_sel = GTK_FILE_SELECTION (fileload);

  gtk_container_set_border_width (GTK_CONTAINER (fileload), 2);
  gtk_container_set_border_width (GTK_CONTAINER (file_sel->button_area), 2);

  g_signal_connect_swapped (G_OBJECT (file_sel->cancel_button), "clicked",
			    G_CALLBACK (file_dialog_hide),
			    fileload);
  g_signal_connect (G_OBJECT (fileload), "delete_event",
		    G_CALLBACK (file_dialog_hide),
		    NULL);

  g_signal_connect (G_OBJECT (file_sel->ok_button), "clicked",
		    G_CALLBACK (file_open_ok_callback),
		    fileload);

  gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (fileload), TRUE);
  tree_sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (GTK_FILE_SELECTION (fileload)->file_list));

  /* Catch file-list clicks so we can update the preview thumbnail */
  g_signal_connect (G_OBJECT (tree_sel), "changed",
		    G_CALLBACK (file_open_selchanged_callback),
		    fileload);

  /*  Connect the "F1" help key  */
  gimp_help_connect (fileload,
		     gimp_standard_help_func,
		     "open/dialogs/file_open.html");

  {
    GimpItemFactory *item_factory;
    GtkWidget       *frame;
    GtkWidget       *vbox;
    GtkWidget       *hbox;
    GtkWidget       *option_menu;
    GtkWidget       *open_options_genbutton;
    GtkWidget       *preview;

    open_options = gtk_hbox_new (TRUE, 4);

    /* format-chooser frame */
    frame = gtk_frame_new (_("Determine File Type"));
    gtk_box_pack_start (GTK_BOX (open_options), frame, TRUE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    option_menu = gtk_option_menu_new ();
    gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
    gtk_widget_show (option_menu);

    item_factory = gimp_item_factory_from_path ("<Load>");
    gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu),
                              GTK_ITEM_FACTORY (item_factory)->widget);

    /* Preview frame */
    open_options_frame = frame = gtk_frame_new (_("Preview"));
    gtk_box_pack_end (GTK_BOX (open_options), frame, TRUE, TRUE, 0);
    gtk_widget_show (frame);

    vbox = gtk_vbox_new (FALSE, 2);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
    gtk_container_add (GTK_CONTAINER (frame), vbox);
    gtk_widget_show (vbox);

    hbox = gtk_hbox_new (TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
    gtk_widget_show (hbox);

    open_options_genbutton = gtk_button_new ();
    gtk_box_pack_start (GTK_BOX (hbox), open_options_genbutton, TRUE, FALSE, 0);
    gtk_widget_show (open_options_genbutton);

    g_signal_connect (G_OBJECT (open_options_genbutton), "clicked",
		      G_CALLBACK (file_open_genbutton_callback),
		      fileload);

    gimp_help_set_help_data (open_options_genbutton,
                             _("Click to create preview"), NULL);

    open_options_imagefile = gimp_imagefile_new (NULL);

    preview = gimp_preview_new (GIMP_VIEWABLE (open_options_imagefile),
                                128, 0, FALSE);
    gtk_container_add (GTK_CONTAINER (open_options_genbutton), preview);
    gtk_widget_show (preview);

    g_signal_connect (G_OBJECT (open_options_imagefile), "info_changed",
                      G_CALLBACK (file_open_imagefile_info_changed),
                      NULL);

    open_options_label = gtk_label_new (_("No Selection"));
    gtk_box_pack_start (GTK_BOX (vbox), open_options_label, FALSE, FALSE, 0);
    gtk_widget_show (open_options_label); 

    /* pack the containing open_options hbox into the open-dialog */
    gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (fileload)->main_vbox),
		      open_options, FALSE, FALSE, 0);
  }

  gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), FALSE);

  gtk_widget_show (open_options);
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
                                  gpointer       data)
{
  const gchar *uri;

  uri = gimp_object_get_name (GIMP_OBJECT (imagefile));

  if (! uri)
    {
      gtk_label_set_text (GTK_LABEL (open_options_label), _("No Selection"));
      gtk_frame_set_label (GTK_FRAME (open_options_frame), _("Preview"));
    }
  else
    {
      gchar *basename;

      basename = file_utils_uri_to_utf8_basename (uri);

      gtk_frame_set_label (GTK_FRAME (open_options_frame), basename);

      g_free (basename);
    }

  switch (imagefile->thumb_state)
    {
    case GIMP_IMAGEFILE_STATE_UNKNOWN:
    case GIMP_IMAGEFILE_STATE_REMOTE:
    case GIMP_IMAGEFILE_STATE_NOT_FOUND:
      gtk_label_set_text (GTK_LABEL (open_options_label),
                          _("No preview available"));
      break;

    case GIMP_IMAGEFILE_STATE_EXISTS:
      if (imagefile->image_mtime > imagefile->thumb_mtime)
        {
          gtk_label_set_text (GTK_LABEL (open_options_label),
                              _("Thumbnail is out of date"));
        }
      else
        {
          gchar *str;

          str = g_strdup_printf (_("(%d x %d)"),
                                 imagefile->width,
                                 imagefile->height);

          gtk_label_set_text (GTK_LABEL (open_options_label), str);
          g_free (str);
        }
      break;

    default:
      break;
    }
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
			       gpointer          data)
{
  GtkFileSelection *fileload;
  Gimp             *gimp;
  const gchar      *fullfname;
  gboolean          selected = FALSE;

  gtk_tree_selection_selected_foreach (sel,
				       selchanged_foreach,
				       &selected);
  if (selected)
    {
      gchar *uri;

      fileload = GTK_FILE_SELECTION (data);

      gimp = GIMP (g_object_get_data (G_OBJECT (fileload), "gimp"));

      fullfname = gtk_file_selection_get_filename (fileload);

      gtk_widget_set_sensitive (GTK_WIDGET (open_options_frame), TRUE);

      uri = file_utils_filename_to_uri (gimp->load_procs, fullfname, NULL);

      gimp_object_set_name (GIMP_OBJECT (open_options_imagefile), uri);

      g_free (uri);
    }
  else
    {
      gimp_object_set_name (GIMP_OBJECT (open_options_imagefile), NULL);
    }

  gimp_imagefile_update (open_options_imagefile);
}

static void
file_open_genbutton_callback (GtkWidget *widget,
			      gpointer   data)
{
  GtkFileSelection  *fs;
  Gimp              *gimp;
  gchar            **selections;
  gint               i;

  fs = GTK_FILE_SELECTION (data);

  gimp = GIMP (g_object_get_data (G_OBJECT (fs), "gimp"));

  gimp_set_busy (gimp);
  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  selections = gtk_file_selection_get_selections (fs);

  for (i = 0; selections[i] != NULL; i++)
    {
      if (g_file_test (selections[i], G_FILE_TEST_IS_REGULAR))
        {
          gchar *uri = g_filename_to_uri (selections[i], NULL, NULL);

          if (gimp->config->write_thumbnails)
            {
              gimp_object_set_name (GIMP_OBJECT (open_options_imagefile),
                                    uri);
              gimp_imagefile_create_thumbnail (open_options_imagefile);
            }

#if 0
            {
              gtk_label_set_text (GTK_LABEL (open_options_label),
                                  _("Failed to generate preview."));
            }
#endif

          g_free (uri);
        }
     }

  g_strfreev (selections);

  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
  gimp_unset_busy (gimp);
}

static void
file_open_ok_callback (GtkWidget *widget,
		       gpointer   data)
{
  GtkFileSelection   *fs;
  Gimp               *gimp;
  gchar              *entered_filename;
  GimpPDBStatusType   status;
  gchar             **selections;
  gint                i;
  gchar              *uri;

  fs = GTK_FILE_SELECTION (data);

  gimp = GIMP (g_object_get_data (G_OBJECT (fs), "gimp"));

  selections = gtk_file_selection_get_selections (fs);

  if (selections == NULL)
    return;

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

  entered_filename =
    g_strdup (gtk_entry_get_text (GTK_ENTRY (fs->selection_entry)));

  gtk_widget_set_sensitive (GTK_WIDGET (fs), FALSE);

  if (! g_file_test (selections[0], G_FILE_TEST_EXISTS))
    { 
      /* try with the entered filename in case the user typed an URI */

      uri = g_strdup (entered_filename);
    }
  else
    {
      uri = g_filename_to_uri (selections[0], NULL, NULL);
    }

  status = file_open_with_proc_and_display (gimp,
                                            uri,
                                            entered_filename, 
                                            load_file_proc);

  g_free (entered_filename);
  g_free (uri);

  if (status == GIMP_PDB_SUCCESS)
    {
      file_dialog_hide (data);
    }
  else if (status != GIMP_PDB_CANCEL)
    {
      /* Hackery required. Please add error message. --bex */
      g_message (_("Opening '%s' failed."), uri);
    }

  /*
   * Now deal with multiple selections from the filesel list
   */

  for (i = 1; selections[i] != NULL; i++)
    {
      if (g_file_test (selections[i], G_FILE_TEST_IS_REGULAR))
	{
          uri = g_filename_to_uri (selections[i], NULL, NULL);

	  status = file_open_with_proc_and_display (gimp,
						    uri,
						    uri,
						    load_file_proc);

	  if (status == GIMP_PDB_SUCCESS)
	    {
	      file_dialog_hide (data);
	    }
	  else if (status != GIMP_PDB_CANCEL)
	    {
	      /* same as previous. --bex */
	      g_message (_("Opening '%s' failed."), uri);
	    }

          g_free (uri);
	}
    }

  g_strfreev (selections);
    
  gtk_widget_set_sensitive (GTK_WIDGET (fs), TRUE);
}
