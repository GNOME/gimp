/* LIBGIMP - The GIMP Library 
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball                
 *
 * gimppatheditor.c
 * Copyright (C) 1999 Michael Natterer <mitch@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <string.h>

#include "gimppatheditor.h"

#include "libgimp/gimpfileselection.h"
#include "libgimp/gimpwidgets.h"

#include "pixmaps/new.xpm"
#include "pixmaps/delete.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/lower.xpm"

/*  forward declaration  */
static void gimp_path_editor_select_callback   (GtkWidget *widget,
						gpointer   data);
static void gimp_path_editor_deselect_callback (GtkWidget *widget,
						gpointer   data);
static void gimp_path_editor_new_callback      (GtkWidget *widget,
						gpointer   data);
static void gimp_path_editor_move_callback     (GtkWidget *widget,
						gpointer   data);
static void gimp_path_editor_filesel_callback  (GtkWidget *widget,
						gpointer   data);
static void gimp_path_editor_delete_callback   (GtkWidget *widget,
						gpointer   data);

enum
{
  PATH_CHANGED,
  LAST_SIGNAL
};

static guint gimp_path_editor_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;

static void
gimp_path_editor_class_init (GimpPathEditorClass *class)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass *) class;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  gimp_path_editor_signals[PATH_CHANGED] = 
    gtk_signal_new ("path_changed",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GimpPathEditorClass,
				       path_changed),
		    gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, gimp_path_editor_signals, 
				LAST_SIGNAL);

  class->path_changed = NULL;
}

static void
gimp_path_editor_init (GimpPathEditor *gpe)
{
  GtkWidget *button_box;
  GtkWidget *button;
  GtkWidget *scrolled_window;

  gpe->file_selection = NULL;
  gpe->selected_item = NULL;
  gpe->number_of_items = 0;

  gpe->upper_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (gpe), gpe->upper_hbox, FALSE, TRUE, 0);
  gtk_widget_show (gpe->upper_hbox);

  button_box = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gpe->upper_hbox), button_box, FALSE, TRUE, 0);
  gtk_widget_show (button_box);

  gpe->new_button = button = gimp_pixmap_button_new (new_xpm, NULL);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_path_editor_new_callback),
		      gpe);
  gtk_widget_show (button);

  gpe->up_button = button = gimp_pixmap_button_new (raise_xpm, NULL);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_path_editor_move_callback),
		      gpe);
  gtk_widget_show (button);

  gpe->down_button = button = gimp_pixmap_button_new (lower_xpm, NULL);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_path_editor_move_callback),
		      gpe);
  gtk_widget_show (button);

  gpe->delete_button = button = gimp_pixmap_button_new (delete_xpm, NULL);
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (gimp_path_editor_delete_callback),
		      gpe);
  gtk_widget_show (button);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (gpe), scrolled_window, TRUE, TRUE, 2);
  gtk_widget_show (scrolled_window);

  gpe->dir_list = gtk_list_new ();
  gtk_list_set_selection_mode (GTK_LIST (gpe->dir_list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window),
					 gpe->dir_list);
  gtk_widget_show (gpe->dir_list);
}

GtkType
gimp_path_editor_get_type (void)
{
  static GtkType gpe_type = 0;

  if (!gpe_type)
    {
      GtkTypeInfo gpe_info =
      {
	"GimpPathEditor",
	sizeof (GimpPathEditor),
	sizeof (GimpPathEditorClass),
	(GtkClassInitFunc) gimp_path_editor_class_init,
	(GtkObjectInitFunc) gimp_path_editor_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL
      };

      gpe_type = gtk_type_unique (gtk_vbox_get_type (), &gpe_info);
    }
  
  return gpe_type;
}

/**
 * gimp_path_editor_new:
 * @filesel_title: The title of the #GtkFileSelection dialog which can be
 *                 popped up by the attached #GimpFileSelection.
 * @path: The initial search path.
 *
 * Creates a new #GimpPathEditor widget.
 *
 * The elements of the initial search path must be separated with the
 * #G_SEARCHPATH_SEPARATOR character.
 *
 * Returns: A pointer to the new #GimpPathEditor widget.
 *
 */
GtkWidget *
gimp_path_editor_new (gchar *filesel_title,
		      gchar *path)
{
  GimpPathEditor *gpe;
  GtkWidget      *list_item;
  GList          *directory_list;
  gchar          *directory;

  g_return_val_if_fail ((filesel_title != NULL), NULL);
  g_return_val_if_fail ((path != NULL), NULL);

  gpe = gtk_type_new (gimp_path_editor_get_type ());

  gpe->file_selection = gimp_file_selection_new (filesel_title, "", TRUE, TRUE);
  gtk_widget_set_sensitive (gpe->file_selection, FALSE);
  gtk_box_pack_start (GTK_BOX (gpe->upper_hbox), gpe->file_selection,
		      TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (gpe->file_selection), "filename_changed",
		      GTK_SIGNAL_FUNC (gimp_path_editor_filesel_callback),
		      gpe);
  gtk_widget_show (gpe->file_selection);

  directory_list = NULL;
  directory = path = g_strdup (path);

  /*  split up the path  */
  while (strlen (directory))
    {
      gchar *current_dir;
      gchar *next_separator;

      next_separator = strchr (directory, G_SEARCHPATH_SEPARATOR);
      if (next_separator != NULL)
	*next_separator = '\0';

      current_dir = g_strdup (directory);

      list_item = gtk_list_item_new_with_label (current_dir);
      gtk_object_set_data_full (GTK_OBJECT (list_item), "gimp_path_editor",
				current_dir,
				(GtkDestroyNotify) g_free);
      directory_list = g_list_append (directory_list, list_item);
      gtk_signal_connect (GTK_OBJECT (list_item), "select",
			  GTK_SIGNAL_FUNC (gimp_path_editor_select_callback),
			  gpe);
      gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
			  GTK_SIGNAL_FUNC (gimp_path_editor_deselect_callback),
			  gpe);
      gtk_widget_show (list_item);
      gpe->number_of_items++;

      if (next_separator != NULL)
	directory = next_separator + 1;
      else
	break;
    }

  g_free (path);

  if (directory_list)
    gtk_list_append_items (GTK_LIST (gpe->dir_list), directory_list);

  return GTK_WIDGET (gpe);
}

/**
 * gimp_path_editor_get_path:
 * @gpe: The path editor you want to get the search path from.
 *
 * The elements of the returned search path string are separated with the
 * #G_SEARCHPATH_SEPARATOR character.
 *
 * Note that you have to g_free() the returned string.
 *
 * Returns: The search path the user has selected in the path editor.
 *
 */
gchar *
gimp_path_editor_get_path (GimpPathEditor *gpe)
{
  GList *list;
  gchar *path = NULL;

  g_return_val_if_fail (gpe != NULL, g_strdup (""));
  g_return_val_if_fail (GIMP_IS_PATH_EDITOR (gpe), g_strdup (""));

  for (list = GTK_LIST (gpe->dir_list)->children; list; list = list->next)
    {
      if (path == NULL)
	{
	  path =
	    g_strdup ((gchar *) gtk_object_get_data (GTK_OBJECT (list->data),
						     "gimp_path_editor"));
	}
      else
	{
	  gchar *newpath;

	  newpath =
	    g_strconcat (path,
			 G_SEARCHPATH_SEPARATOR_S,
			 (gchar *) gtk_object_get_data (GTK_OBJECT (list->data),
							"gimp_path_editor"),
			 NULL);

	  g_free (path);
	  path = newpath;
	}
    }

  return path;
}

static void
gimp_path_editor_select_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpPathEditor *gpe;
  gint            pos;
  gchar          *directory;

  gpe = GIMP_PATH_EDITOR (data);
  directory = (gchar *) gtk_object_get_data (GTK_OBJECT (widget),
					     "gimp_path_editor");

  gtk_signal_handler_block_by_data (GTK_OBJECT (gpe->file_selection), gpe);
  gimp_file_selection_set_filename (GIMP_FILE_SELECTION (gpe->file_selection),
				    directory);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gpe->file_selection), gpe);
  gpe->selected_item = widget;

  pos = gtk_list_child_position (GTK_LIST (gpe->dir_list), gpe->selected_item);

  gtk_widget_set_sensitive (gpe->delete_button, TRUE);
  gtk_widget_set_sensitive (gpe->up_button, (pos > 0));
  gtk_widget_set_sensitive (gpe->down_button,
			    (pos < (gpe->number_of_items - 1)));
  gtk_widget_set_sensitive (gpe->file_selection, TRUE);
}

/*  the selected directory may never be deselected except by the 'new'
 *  button, so catch the "deselect" signal and reselect it
 */
static void
gimp_path_editor_deselect_callback (GtkWidget *widget,
				    gpointer   data)
{
  GimpPathEditor *gpe;

  gpe = GIMP_PATH_EDITOR (data);

  if (widget != gpe->selected_item)
    return;

  gtk_signal_handler_block_by_data (GTK_OBJECT (gpe->selected_item), gpe);
  gtk_list_select_child (GTK_LIST (gpe->dir_list), gpe->selected_item);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gpe->selected_item), gpe);
}

static void
gimp_path_editor_new_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpPathEditor *gpe;

  gpe = GIMP_PATH_EDITOR (data);

  if (gpe->selected_item)
    {
      gtk_signal_handler_block_by_data (GTK_OBJECT (gpe->selected_item), gpe);
      gtk_list_unselect_child (GTK_LIST (gpe->dir_list), gpe->selected_item);
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gpe->selected_item), gpe);
    }
  gpe->selected_item = NULL;

  gtk_widget_set_sensitive (gpe->delete_button, FALSE);
  gtk_widget_set_sensitive (gpe->up_button, FALSE);
  gtk_widget_set_sensitive (gpe->down_button, FALSE);
  gtk_widget_set_sensitive (gpe->file_selection, TRUE);

  gtk_editable_set_position
    (GTK_EDITABLE (GIMP_FILE_SELECTION (gpe->file_selection)->entry), -1);
  gtk_widget_grab_focus
    (GTK_WIDGET (GIMP_FILE_SELECTION (gpe->file_selection)->entry));
}

static void
gimp_path_editor_move_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpPathEditor *gpe = GIMP_PATH_EDITOR (data);
  GList          *move_list = NULL;
  gint            pos;
  gint            distance;

  if (gpe->selected_item == NULL)
    return;

  pos = gtk_list_child_position (GTK_LIST (gpe->dir_list), gpe->selected_item);
  distance = (widget == gpe->up_button) ? - 1 : 1;
  move_list = g_list_append (move_list, gpe->selected_item);

  gtk_signal_handler_block_by_data (GTK_OBJECT (gpe->selected_item), gpe);
  gtk_list_remove_items_no_unref (GTK_LIST (gpe->dir_list), move_list);
  gtk_signal_handler_unblock_by_data (GTK_OBJECT (gpe->selected_item), gpe);
  gtk_list_insert_items (GTK_LIST (gpe->dir_list), move_list, pos + distance);
  gtk_list_select_item (GTK_LIST (gpe->dir_list), pos + distance);

  gtk_signal_emit (GTK_OBJECT (gpe), gimp_path_editor_signals[PATH_CHANGED]);
}

static void
gimp_path_editor_delete_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpPathEditor *gpe = GIMP_PATH_EDITOR (data);
  GList          *delete_list = NULL;
  gint            pos;

  if (gpe->selected_item == NULL)
    return;

  pos = gtk_list_child_position (GTK_LIST (gpe->dir_list), gpe->selected_item);
  delete_list = g_list_append (delete_list, gpe->selected_item);

  gtk_list_remove_items (GTK_LIST (gpe->dir_list), delete_list);
  gpe->number_of_items--;

  if (gpe->number_of_items == 0)
    {
      gpe->selected_item = NULL;
      gtk_signal_handler_block_by_data (GTK_OBJECT (gpe->file_selection), gpe);
      gimp_file_selection_set_filename (GIMP_FILE_SELECTION (gpe->file_selection), "");
      gtk_signal_handler_unblock_by_data (GTK_OBJECT (gpe->file_selection), gpe);
      gtk_widget_set_sensitive (gpe->delete_button, FALSE);
      gtk_widget_set_sensitive (gpe->file_selection, FALSE);

      return;
    }

  if ((pos == gpe->number_of_items) && (pos > 0))
    pos--;
  gtk_list_select_item (GTK_LIST (gpe->dir_list), pos);

  gtk_signal_emit (GTK_OBJECT (gpe), gimp_path_editor_signals[PATH_CHANGED]);
}

static void
gimp_path_editor_filesel_callback (GtkWidget *widget,
				   gpointer   data)
{
  GimpPathEditor *gpe = GIMP_PATH_EDITOR (data);
  GList          *append_list = NULL;
  GtkWidget      *list_item = NULL;
  gchar          *directory;

  directory = gimp_file_selection_get_filename (GIMP_FILE_SELECTION (widget));
  if (strcmp (directory, "") == 0)
    return;

  if (gpe->selected_item == NULL)
    {
      list_item = gtk_list_item_new_with_label (directory);
      gtk_object_set_data_full (GTK_OBJECT (list_item), "gimp_path_editor",
				directory,
				(GtkDestroyNotify) g_free);
      append_list = g_list_append (append_list, list_item);
      gtk_signal_connect (GTK_OBJECT (list_item), "select",
			  GTK_SIGNAL_FUNC (gimp_path_editor_select_callback),
			  gpe);
      gtk_signal_connect (GTK_OBJECT (list_item), "deselect",
			  GTK_SIGNAL_FUNC (gimp_path_editor_deselect_callback),
			  gpe);
      gtk_widget_show (list_item);
      gpe->number_of_items++;
      gtk_list_append_items (GTK_LIST (gpe->dir_list), append_list);
      gtk_list_select_item (GTK_LIST (gpe->dir_list), gpe->number_of_items - 1);
    }
  else
    {
      gtk_label_set_text (GTK_LABEL (GTK_BIN (gpe->selected_item)->child),
			  directory);
      gtk_object_set_data_full (GTK_OBJECT (gpe->selected_item),
				"gimp_path_editor",
				directory,
				(GtkDestroyNotify) g_free);
    }

  gtk_signal_emit (GTK_OBJECT (gpe), gimp_path_editor_signals[PATH_CHANGED]);
}
