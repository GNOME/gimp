/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * gimppatheditor.c
 * Copyright (C) 1999-2003 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include "gimpwidgetstypes.h"

#include "gimppatheditor.h"
#include "gimpfileentry.h"


enum
{
  PATH_CHANGED,
  LAST_SIGNAL
};


static void     gimp_path_editor_class_init         (GimpPathEditorClass *klass);
static void     gimp_path_editor_init               (GimpPathEditor      *gpe);

static void     gimp_path_editor_new_clicked        (GtkWidget           *widget,
                                                     GimpPathEditor      *gpe);
static void     gimp_path_editor_move_clicked       (GtkWidget           *widget,
                                                     GimpPathEditor      *gpe);
static void     gimp_path_editor_delete_clicked     (GtkWidget           *widget,
                                                     GimpPathEditor      *gpe);
static void     gimp_path_editor_file_entry_changed (GtkWidget           *widget,
                                                     GimpPathEditor      *gpe);
static void     gimp_path_editor_selection_changed  (GtkTreeSelection    *sel,
                                                     GimpPathEditor      *gpe);

static gboolean build_path                          (GtkTreeModel        *model,
                                                     GtkTreePath         *tpath,
                                                     GtkTreeIter         *iter,
                                                     gpointer             data);


static guint gimp_path_editor_signals[LAST_SIGNAL] = { 0 };

static GtkVBoxClass *parent_class = NULL;


GType
gimp_path_editor_get_type (void)
{
  static GType gpe_type = 0;

  if (! gpe_type)
    {
      static const GTypeInfo gpe_info =
      {
        sizeof (GimpPathEditorClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) gimp_path_editor_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data     */
	sizeof (GimpPathEditor),
	0,              /* n_preallocs    */
	(GInstanceInitFunc) gimp_path_editor_init,
      };

      gpe_type = g_type_register_static (GTK_TYPE_VBOX,
                                         "GimpPathEditor",
                                         &gpe_info, 0);
    }

  return gpe_type;
}

static void
gimp_path_editor_class_init (GimpPathEditorClass *klass)
{
  parent_class = g_type_class_peek_parent (klass);

  gimp_path_editor_signals[PATH_CHANGED] =
    g_signal_new ("path_changed",
		  G_TYPE_FROM_CLASS (klass),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GimpPathEditorClass, path_changed),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  klass->path_changed = NULL;
}

static void
gimp_path_editor_init (GimpPathEditor *gpe)
{
  GtkWidget *button_box;
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *scrolled_window;
  GtkWidget *tv;

  gpe->file_entry = NULL;
  gpe->sel_path   = NULL;
  gpe->num_items  = 0;

  gpe->upper_hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (gpe), gpe->upper_hbox, FALSE, TRUE, 0);
  gtk_widget_show (gpe->upper_hbox);

  button_box = gtk_hbox_new (TRUE, 0);
  gtk_box_pack_start (GTK_BOX (gpe->upper_hbox), button_box, FALSE, TRUE, 0);
  gtk_widget_show (button_box);

  gpe->new_button = button = gtk_button_new ();
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_NEW, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_path_editor_new_clicked),
		    gpe);

  gpe->up_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_UP, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_path_editor_move_clicked),
		    gpe);

  gpe->down_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_path_editor_move_clicked),
		    gpe);

  gpe->delete_button = button = gtk_button_new ();
  gtk_widget_set_sensitive (button, FALSE);
  gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  image = gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (gimp_path_editor_delete_clicked),
		    gpe);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
				       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_ALWAYS);
  gtk_box_pack_start (GTK_BOX (gpe), scrolled_window, TRUE, TRUE, 2);
  gtk_widget_show (scrolled_window);

  gpe->dir_list = gtk_list_store_new (1, G_TYPE_STRING);
  tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (gpe->dir_list));
  g_object_unref (gpe->dir_list);

  gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (tv),
					       -1, NULL,
					       gtk_cell_renderer_text_new (),
					       "text", 0,
					       NULL);

  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tv), FALSE);

  gtk_container_add (GTK_CONTAINER (scrolled_window), tv);
  gtk_widget_show (tv);

  gpe->sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
  g_signal_connect (gpe->sel, "changed",
		    G_CALLBACK (gimp_path_editor_selection_changed),
		    gpe);
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
 **/
GtkWidget *
gimp_path_editor_new (const gchar *filesel_title,
		      const gchar *path)
{
  GimpPathEditor *gpe;

  g_return_val_if_fail (filesel_title != NULL, NULL);

  gpe = g_object_new (GIMP_TYPE_PATH_EDITOR, NULL);

  gpe->file_entry = gimp_file_entry_new (filesel_title, "", TRUE, TRUE);
  gtk_widget_set_sensitive (gpe->file_entry, FALSE);
  gtk_box_pack_start (GTK_BOX (gpe->upper_hbox), gpe->file_entry,
                      TRUE, TRUE, 0);
  gtk_widget_show (gpe->file_entry);

  g_signal_connect (gpe->file_entry, "filename_changed",
                    G_CALLBACK (gimp_path_editor_file_entry_changed),
		    gpe);

  if (path)
    gimp_path_editor_set_path (gpe, path);

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
 **/
gchar *
gimp_path_editor_get_path (GimpPathEditor *gpe)
{
  GString *path;

  g_return_val_if_fail (GIMP_IS_PATH_EDITOR (gpe), g_strdup (""));

  path = g_string_new ("");

  gtk_tree_model_foreach (GTK_TREE_MODEL (gpe->dir_list), build_path, path);

  return g_string_free (path, FALSE);
}

/**
 * gimp_path_editor_set_path:
 * @gpe:  The path editor you want to set the search path from.
 * @path: The new path to set.
 *
 * The elements of the initial search path must be separated with the
 * #G_SEARCHPATH_SEPARATOR character.
 **/
void
gimp_path_editor_set_path (GimpPathEditor *gpe,
                           const gchar    *path)
{
  gchar       *directory;
  gchar       *mypath;
  GtkTreeIter  iter;

  g_return_if_fail (GIMP_IS_PATH_EDITOR (gpe));

  directory = mypath = g_strdup (path);

  gtk_list_store_clear (gpe->dir_list);

  /*  split up the path  */
  while (strlen (directory))
    {
      gchar *next_separator;

      next_separator = strchr (directory, G_SEARCHPATH_SEPARATOR);
      if (next_separator != NULL)
	*next_separator = '\0';

      gtk_list_store_append (gpe->dir_list, &iter);
      gtk_list_store_set (gpe->dir_list, &iter, 0, directory, -1);

      gpe->num_items++;

      if (next_separator != NULL)
	directory = next_separator + 1;
      else
	break;
    }

  g_free (mypath);
}


/*  private functions  */

static void
gimp_path_editor_new_clicked (GtkWidget      *widget,
                              GimpPathEditor *gpe)
{
  if (gpe->sel_path)
    {
      g_signal_handlers_block_by_func (gpe->sel,
				       gimp_path_editor_selection_changed,
                                       gpe);

      gtk_tree_selection_unselect_path (gpe->sel, gpe->sel_path);

      g_signal_handlers_unblock_by_func (gpe->sel,
					 gimp_path_editor_selection_changed,
                                         gpe);

      gtk_tree_path_free (gpe->sel_path);
      gpe->sel_path = NULL;
    }

  gtk_widget_set_sensitive (gpe->delete_button, FALSE);
  gtk_widget_set_sensitive (gpe->up_button, FALSE);
  gtk_widget_set_sensitive (gpe->down_button, FALSE);
  gtk_widget_set_sensitive (gpe->file_entry, TRUE);

  gtk_editable_set_position
    (GTK_EDITABLE (GIMP_FILE_ENTRY (gpe->file_entry)->entry), -1);
  gtk_widget_grab_focus
    (GTK_WIDGET (GIMP_FILE_ENTRY (gpe->file_entry)->entry));
}

static void
gimp_path_editor_move_clicked (GtkWidget      *widget,
                               GimpPathEditor *gpe)
{
  GtkTreePath  *path;
  GtkTreeModel *model;
  GtkTreeIter   iter1, iter2;
  gchar        *dir1, *dir2;

  if (gpe->sel_path == NULL)
    return;

  path = gtk_tree_path_copy (gpe->sel_path);

  if (widget == gpe->up_button)
    gtk_tree_path_prev (path);
  else
    gtk_tree_path_next (path);

  model = GTK_TREE_MODEL (gpe->dir_list);

  gtk_tree_model_get_iter (model, &iter1, gpe->sel_path);
  gtk_tree_model_get_iter (model, &iter2, path);

  gtk_tree_model_get (model, &iter1, 0, &dir1, -1);
  gtk_tree_model_get (model, &iter2, 0, &dir2, -1);

  gtk_list_store_set (gpe->dir_list, &iter1, 0, dir2, -1);
  gtk_list_store_set (gpe->dir_list, &iter2, 0, dir1, -1);

  g_free (dir2);
  g_free (dir1);

  gtk_tree_selection_select_iter (gpe->sel, &iter2);

  g_signal_emit (gpe, gimp_path_editor_signals[PATH_CHANGED], 0);
}

static void
gimp_path_editor_delete_clicked (GtkWidget      *widget,
                                 GimpPathEditor *gpe)
{
  GtkTreeIter  iter;
  gint        *indices;

  if (gpe->sel_path == NULL)
    return;

  gtk_tree_model_get_iter (GTK_TREE_MODEL (gpe->dir_list), &iter,
			   gpe->sel_path);
  gtk_list_store_remove (gpe->dir_list, &iter);

  gpe->num_items--;

  if (gpe->num_items == 0)
    {
      gtk_tree_path_free (gpe->sel_path);
      gpe->sel_path = NULL;

      g_signal_handlers_block_by_func (gpe->file_entry,
                                       gimp_path_editor_file_entry_changed,
                                       gpe);

      gimp_file_entry_set_filename (GIMP_FILE_ENTRY (gpe->file_entry), "");

      g_signal_handlers_unblock_by_func (gpe->file_entry,
                                         gimp_path_editor_file_entry_changed,
                                         gpe);

      gtk_widget_set_sensitive (gpe->delete_button, FALSE);
      gtk_widget_set_sensitive (gpe->up_button,     FALSE);
      gtk_widget_set_sensitive (gpe->down_button,   FALSE);
      gtk_widget_set_sensitive (gpe->file_entry,    FALSE);

      return;
    }

  indices = gtk_tree_path_get_indices (gpe->sel_path);
  if ((indices[0] == gpe->num_items) && (indices[0] > 0))
    gtk_tree_path_prev (gpe->sel_path);

  gtk_tree_selection_select_path (gpe->sel, gpe->sel_path);

  g_signal_emit (gpe, gimp_path_editor_signals[PATH_CHANGED], 0);
}

static void
gimp_path_editor_file_entry_changed (GtkWidget      *widget,
                                     GimpPathEditor *gpe)
{
  gchar       *directory;
  GtkTreeIter  iter;

  directory = gimp_file_entry_get_filename (GIMP_FILE_ENTRY (widget));
  if (strcmp (directory, "") == 0)
    {
      g_free (directory);
      return;
    }

  if (gpe->sel_path == NULL)
    {
      gtk_list_store_append (gpe->dir_list, &iter);
      gtk_list_store_set (gpe->dir_list, &iter, 0, directory, -1);
      gpe->num_items++;

      gtk_tree_selection_select_iter (gpe->sel, &iter);
    }
  else
    {
      gtk_tree_model_get_iter (GTK_TREE_MODEL (gpe->dir_list), &iter,
			       gpe->sel_path);
      gtk_list_store_set (gpe->dir_list, &iter, 0, directory, -1);
    }

  g_free (directory);

  g_signal_emit (gpe, gimp_path_editor_signals[PATH_CHANGED], 0);
}

static void
gimp_path_editor_selection_changed (GtkTreeSelection *sel,
                                    GimpPathEditor   *gpe)
{
  GtkTreeIter  iter;
  gchar       *directory;
  gint        *indices;

  if (gtk_tree_selection_get_selected (sel, NULL, &iter))
    {
      gtk_tree_model_get (GTK_TREE_MODEL (gpe->dir_list), &iter,
			  0, &directory,
			  -1);

      g_signal_handlers_block_by_func (gpe->file_entry,
				       gimp_path_editor_file_entry_changed,
				       gpe);

      gimp_file_entry_set_filename (GIMP_FILE_ENTRY (gpe->file_entry),
                                    directory);

      g_signal_handlers_unblock_by_func (gpe->file_entry,
					 gimp_path_editor_file_entry_changed,
					 gpe);

      g_free (directory);

      if (gpe->sel_path)
	gtk_tree_path_free (gpe->sel_path);

      gpe->sel_path = gtk_tree_model_get_path (GTK_TREE_MODEL (gpe->dir_list),
					       &iter);

      indices = gtk_tree_path_get_indices (gpe->sel_path);

      gtk_widget_set_sensitive (gpe->delete_button, TRUE);
      gtk_widget_set_sensitive (gpe->up_button, (indices[0] > 0));
      gtk_widget_set_sensitive (gpe->down_button,
				(indices[0] < (gpe->num_items - 1)));
      gtk_widget_set_sensitive (gpe->file_entry, TRUE);
    }
  else
    {
      g_signal_handlers_block_by_func (sel,
				       gimp_path_editor_selection_changed,
				       gpe);

      gtk_tree_selection_select_path (gpe->sel, gpe->sel_path);

      g_signal_handlers_unblock_by_func (sel,
					 gimp_path_editor_selection_changed,
					 gpe);
    }
}

static gboolean
build_path (GtkTreeModel *model,
	    GtkTreePath  *tpath,
	    GtkTreeIter  *iter,
	    gpointer      data)
{
  gchar   *buf;
  GString *path = data;

  gtk_tree_model_get (model, iter, 0, &buf, -1);

  if (path->len > 0)
    g_string_append_c (path, G_SEARCHPATH_SEPARATOR);

  g_string_append (path, buf);

  g_free (buf);

  return FALSE;
}
